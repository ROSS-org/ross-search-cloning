#include "director.h"
#include "ross-extern.h"
#include "state.h"
#include <stdio.h>

static inline void synch_lp_to_gvt(tw_pe *pe, tw_lp *grid_lp, tw_event_sig *gvt_sig) {
    grid_lp->kp->last_sig = *gvt_sig;
    pe->cur_event = pe->abort_event;
    pe->cur_event->caused_by_me = NULL;
    pe->cur_event->sig = pe->GVT_sig;
}

/** Structure to track decision information for GVT hook */
struct DecisionInfo {
    int x, y;                    /**< Position where decision was made */
    enum DIRECTION chosen_dir;   /**< First direction chosen */
    enum DIRECTION second_dir;   /**< Second direction option */
    tw_stime timestamp;          /**< When the decision was made */
};

static inline void assert_valid_DecisionInfo(struct DecisionInfo *di) {
#ifndef NDEBUG
    assert(di != NULL);
    assert(di->x >= 0 && di->x < g_grid_width);
    assert(di->y >= 0 && di->y < g_grid_height);
    assert(di->chosen_dir >= DIRECTION_north && di->chosen_dir <= DIRECTION_west);
    assert(di->second_dir >= DIRECTION_north && di->second_dir <= DIRECTION_west);
    assert(di->timestamp >= 0.0);
#endif
}

enum OPTION {
    OPTION_first_branch = 0,
    OPTION_second_branch
};

// Static storage for current decision information
static struct DecisionInfo current_decision;
static bool did_this_pe_trigger = false;

// Generates a non-valid current decision position, because current_decision should never be used if did_this_pe_trigger == false
static void clean_current_decision(void) {
    current_decision.x = -1;
    current_decision.y = -1;
    current_decision.chosen_dir = DIRECTION_none;
    current_decision.second_dir = DIRECTION_none;
    current_decision.timestamp = -1;

    did_this_pe_trigger = false;
}

void director_init(void) {
    clean_current_decision();
}

struct SerializableEvent {
    tw_lpid local_lpid;
    tw_stime recv_ts;
    tw_stime prio;
    struct SearchMessage msg;
};

// BUG: We are not copying the tie-breaker signature for each event, which means that tied events will be pontentially rescheduled at the destination on a different order to that of the source simulation. Copying the precise tie-breaker signature is not a solution, because we don't want TWO different events with the same precise signature (leads to weird collisions and conflicts with the assumptions of cloning, where only ONE PE calls the director at the time)
static void clone_events(tw_pe *pe, tw_peid source, tw_peid dest) {
    assert(g_tw_mynode == source || g_tw_mynode == dest);

    if (g_tw_mynode == source) {
        tw_event_sig gvt_sig = pe->GVT_sig;

        int event_count = 0;
        struct SerializableEvent *events = NULL;

        tw_event *next_event = tw_pq_dequeue(pe->pq);
        tw_event *dequeued_events = NULL;

        while (next_event) {
            assert(tw_event_sig_compare_ptr(&next_event->sig, &gvt_sig) >= 0);

            if (next_event->event_id && next_event->state.remote) {
                tw_hash_remove(pe->hash_t, next_event, next_event->send_pe);
            }

            events = realloc(events, (event_count + 1) * sizeof(struct SerializableEvent));
            events[event_count].local_lpid = next_event->dest_lpid - g_tw_lp_offset;
            events[event_count].recv_ts = next_event->recv_ts;
            events[event_count].prio = next_event->sig.priority;

            void *msg_data = tw_event_data(next_event);
            events[event_count].msg = *(struct SearchMessage*)msg_data;

            event_count++;

            next_event->prev = dequeued_events;
            dequeued_events = next_event;
            next_event = tw_pq_dequeue(pe->pq);
        }

        while (dequeued_events) {
            tw_event *prev_event = dequeued_events;
            dequeued_events = dequeued_events->prev;
            prev_event->prev = NULL;
            tw_pq_enqueue(pe->pq, prev_event);

            if (prev_event->event_id && prev_event->state.remote) {
                tw_hash_insert(pe->hash_t, prev_event, prev_event->send_pe);
            }
        }

        MPI_Send(&event_count, 1, MPI_INT, dest, 0, MPI_COMM_ROSS);
        if (event_count > 0) {
            MPI_Send(events, event_count * sizeof(struct SerializableEvent),
                     MPI_BYTE, dest, 0, MPI_COMM_ROSS);
        }
        free(events);

    } else if (g_tw_mynode == dest) {
        tw_event_sig gvt_sig = pe->GVT_sig;
        tw_stime gvt = gvt_sig.recv_ts;

        int event_count;
        MPI_Status status;

        MPI_Recv(&event_count, 1, MPI_INT, source, 0, MPI_COMM_ROSS, &status);

        if (event_count > 0) {
            struct SerializableEvent *events = malloc(event_count * sizeof(struct SerializableEvent));
            MPI_Recv(events, event_count * sizeof(struct SerializableEvent),
                     MPI_BYTE, source, 0, MPI_COMM_ROSS, &status);

            for (int i = 0; i < event_count; i++) {
                tw_lpid local_lpid = events[i].local_lpid;
                if (local_lpid >= 0 && local_lpid < g_tw_nlp) {
                    tw_lp *dest_lp = g_tw_lp[local_lpid];
                    synch_lp_to_gvt(pe, dest_lp, &gvt_sig);

                    // Scheduling event from itself
                    tw_event *new_event = tw_event_new_user_prio(dest_lp->gid, events[i].recv_ts - gvt, dest_lp, events[i].prio);
                    struct SearchMessage *msg = (struct SearchMessage*)tw_event_data(new_event);
                    *msg = events[i].msg;

                    tw_event_send(new_event);
                }
            }
            free(events);
        }
    }
}

static void clone_lp_states(tw_pe *pe, tw_peid source, tw_peid dest) {
    assert(g_tw_mynode == source || g_tw_mynode == dest);

    int total_lps = g_grid_width * g_grid_height;
    assert(total_lps == (int) g_tw_nlp);

    MPI_Request *requests = malloc(g_tw_nlp * sizeof(MPI_Request));

    for (tw_lpid local_lpid = 0; local_lpid < g_tw_nlp; local_lpid++) {
        tw_lp *lp = g_tw_lp[local_lpid];
        struct SearchCellState *state = (struct SearchCellState *)lp->cur_state;

        if (g_tw_mynode == 0) {
            MPI_Isend(state, sizeof(struct SearchCellState), MPI_BYTE, dest,
                      0, MPI_COMM_ROSS, &requests[local_lpid]);
        } else {
            MPI_Irecv(state, sizeof(struct SearchCellState), MPI_BYTE, source,
                      0, MPI_COMM_ROSS, &requests[local_lpid]);
        }
    }

    MPI_Waitall(g_tw_nlp, requests, MPI_STATUSES_IGNORE);
    free(requests);
}

void director_store_decision(int x, int y, enum DIRECTION chosen_dir, enum DIRECTION second_dir, tw_stime timestamp) {
    // Store the decision
    current_decision.x = x;
    current_decision.y = y;
    current_decision.chosen_dir = chosen_dir;
    current_decision.second_dir = second_dir;
    current_decision.timestamp = timestamp;

    did_this_pe_trigger = true;
}

void director_store_decision_rev(int x, int y) {
    clean_current_decision();
}

void advance_to_direction(tw_pe *pe, enum OPTION opt) {
    tw_event_sig gvt_sig = pe->GVT_sig;
    tw_stime gvt = gvt_sig.recv_ts;

    enum DIRECTION dir;
    switch (opt) {
        case OPTION_first_branch:
            dir = current_decision.chosen_dir;
        break;
        case OPTION_second_branch:
            dir = current_decision.second_dir;
        break;
    }

    const char* direction_names[] = {"NORTH", "SOUTH", "EAST", "WEST"};
    printf("PE %d (GVT time: %f) - Position (%d,%d) scheduled at time %.2f: chose %s (options = [%s, %s])\n",
           (int) g_tw_mynode, gvt,
           current_decision.x, current_decision.y,
           current_decision.timestamp,
           direction_names[dir],
           direction_names[current_decision.chosen_dir],
           direction_names[current_decision.second_dir]);

    // Finding LP
    tw_lpid local_lpid = grid_index(current_decision.x, current_decision.y);
    tw_lp * grid_lp = g_tw_lp[local_lpid];
    synch_lp_to_gvt(pe, grid_lp, &gvt_sig);

    send_agent_move(grid_lp, current_decision.x, current_decision.y, dir, current_decision.timestamp + 1.0);
}

void clone_branch_and_advance(tw_pe *pe, tw_peid source, tw_peid dest) {
    if (g_tw_mynode != source && g_tw_mynode != dest) {
        return;
    }

    clone_lp_states(pe, source, dest);
    clone_events(pe, source, dest);

    if (did_this_pe_trigger) {
        assert(source == g_tw_mynode);
        MPI_Send(&current_decision, sizeof(struct DecisionInfo), MPI_BYTE, dest,
                 0, MPI_COMM_ROSS);
        advance_to_direction(pe, OPTION_first_branch);
        did_this_pe_trigger = false;
    } else {
        MPI_Status status;
        MPI_Recv(&current_decision, sizeof(struct DecisionInfo), MPI_BYTE, source,
                 0, MPI_COMM_ROSS, &status);
        advance_to_direction(pe, OPTION_second_branch);
    }
}

void clone_director_gvt_hook(tw_pe *pe, bool past_end_time) {
    (void)past_end_time; // unused parameter
    tw_scheduler_rollback_and_cancel_events_pe(pe);

    // For testing purposes, it will force cloning and branching only on the 4 GVT hook call, otherwise it will advance the simulation on the PE that triggered the call only
    static int num_triggers = 0;
    num_triggers++;
    if (num_triggers != 4) {
        if (did_this_pe_trigger) {
            assert_valid_DecisionInfo(&current_decision);
            advance_to_direction(pe, OPTION_first_branch);
            did_this_pe_trigger = false;
        }
        return;
    }

    // TODO: figure out logic to determine source and destination PEs
    clone_branch_and_advance(pe, 0, 1);
}

void director_finalize(void) {
}
