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

enum OPTION {
    OPTION_first_branch = 0,
    OPTION_second_branch
};

// Static storage for current decision information
static struct DecisionInfo current_decision;
static bool did_this_pe_trigger = false;

void director_init(void) {
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

void clone_director_gvt_hook(tw_pe *pe, bool past_end_time) {
    (void)past_end_time; // unused parameter

    if (!did_this_pe_trigger) {
        return;
    }

    tw_event_sig gvt_sig = pe->GVT_sig;
    tw_stime gvt = gvt_sig.recv_ts;

    const char* direction_names[] = {"NORTH", "SOUTH", "EAST", "WEST"};
    printf("PE %d (GVT time: %f) - Position (%d,%d) scheduled at time %.2f: chose %s (2nd choice: %s)\n",
           (int) g_tw_mynode, gvt,
           current_decision.x, current_decision.y,
           current_decision.timestamp,
           direction_names[current_decision.chosen_dir],
           direction_names[current_decision.second_dir]);

    // Finding LP
    tw_lpid local_lpid = grid_index(current_decision.x, current_decision.y);
    tw_lp * grid_lp = g_tw_lp[local_lpid];
    synch_lp_to_gvt(pe, grid_lp, &gvt_sig);

    send_agent_move(grid_lp, current_decision.x, current_decision.y, current_decision.chosen_dir, current_decision.timestamp + 1.0);

    did_this_pe_trigger = false;
}

void director_finalize(void) {
}
