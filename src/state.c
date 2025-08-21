#include "state.h"
#include "director.h"
#include "ross-kernel-inline.h"
#include <stdio.h>
#include <string.h>

// ================================= Global variables ================================

#define PROB_CLONING 1.0

// Grid dimensions and positions
int g_grid_width = 0;
int g_grid_height = 0;
int g_start_x = -1, g_start_y = -1;
int g_goal_x = -1, g_goal_y = -1;

// Global grid arrays
enum CELL_TYPE *g_initial_grid = NULL;
bool *g_visited_grid = NULL;
enum DIRECTION *g_exit_dirs = NULL;

// ================================= Helper functions ================================

static void get_neighbors(int x, int y, int neighbors[4][2], bool valid[4]) {
    // North, South, East, West
    int dx[] = {0, 0, 1, -1};
    int dy[] = {-1, 1, 0, 0};

    for (int i = 0; i < 4; i++) {
        neighbors[i][0] = x + dx[i];
        neighbors[i][1] = y + dy[i];
        valid[i] = is_valid_position(neighbors[i][0], neighbors[i][1]) &&
                   g_initial_grid[grid_index(neighbors[i][0], neighbors[i][1])] != CELL_TYPE_obstacle;
    }
}

static enum DIRECTION opposite_direction(enum DIRECTION dir) {
    switch (dir) {
        case DIRECTION_north: return DIRECTION_south;
        case DIRECTION_south: return DIRECTION_north;
        case DIRECTION_east:  return DIRECTION_west;
        case DIRECTION_west:  return DIRECTION_east;
        default: return DIRECTION_none;
    }
}

void send_agent_move(tw_lp *lp, int x, int y, enum DIRECTION direction, double at) {
    // North, South, East, West
    int dx[] = {0, 0, 1, -1};
    int dy[] = {-1, 1, 0, 0};

    double const offset = at - tw_now(lp);
    tw_lpid const target_gid = g_tw_lp_offset + grid_index(x + dx[direction], y + dy[direction]);

    tw_event *e = tw_event_new(target_gid, offset, lp);
    struct SearchMessage *msg = tw_event_data(e);
    msg->type = MESSAGE_TYPE_agent_move;
    msg->sender = lp->gid;
    tw_event_send(e);
}

static void send_agent_move_cloning(tw_lp *lp, int x, int y, enum DIRECTION option1, enum DIRECTION option2) {
    director_store_decision(x, y, option1, option2, tw_now(lp));
    tw_trigger_gvt_hook_now(lp);
}

static void send_agent_move_cloning_rev(tw_lp *lp) {
    tw_trigger_gvt_hook_now_rev(lp);
}

static void send_cell_unavailable(tw_lp *lp, int x, int y, enum DIRECTION direction) {
    // North, South, East, West
    int dx[] = {0, 0, 1, -1};
    int dy[] = {-1, 1, 0, 0};

    tw_lpid const target_gid = g_tw_lp_offset + grid_index(x + dx[direction], y + dy[direction]);
    tw_event *e = tw_event_new(target_gid, 0.5, lp);
    struct SearchMessage *msg = tw_event_data(e);
    msg->type = MESSAGE_TYPE_cell_unavailable;
    msg->sender = lp->gid;
    msg->from_dir = opposite_direction(direction);
    tw_event_send(e);
}

// ================================= Message handlers ================================

static void handle_agent_move(struct SearchCellState *state, tw_bf *bf, struct SearchMessage *msg, tw_lp *lp) {
    // Agent arrives at this cell
    state->was_visited = true;

    // If this is the goal, we're done!
    if (state->cell_type == CELL_TYPE_goal) {
        bf->c0 = 1;
        return;
    }

    // Try to move to next cell
    enum DIRECTION available_moves[4];
    int num_moves = 0;

    for (int i = 0; i < 4; i++) {
        if (state->available_dirs[i]) {
            available_moves[num_moves++] = i;
        }
    }

    if (num_moves == 1) {
        // Only one choice - no random number needed
        enum DIRECTION dir = available_moves[0];
        state->exit_dir = dir;

        // Send agent to next cell
        send_agent_move(lp, state->x, state->y, dir, tw_now(lp) + 1.0);
        // Informing cell is no longer available
        send_cell_unavailable(lp, state->x, state->y, dir);
    } else if (num_moves > 1) {
        bf->c1 = 1;
        // Pick random direction from multiple options
        int const choice = tw_rand_integer(lp->rng, 0, num_moves - 1);
        enum DIRECTION const dir = available_moves[choice];
        state->exit_dir = dir;

        double const p = tw_rand_unif(lp->rng);
        if (p < PROB_CLONING) {
            bf->c4 = 1;

            int choice_2nd = tw_rand_integer(lp->rng, 0, num_moves - 2);
            if (choice <= choice_2nd) { choice_2nd += 1; }
            enum DIRECTION const dir_2nd = available_moves[choice_2nd];

            send_agent_move_cloning(lp, state->x, state->y, dir, dir_2nd);
        } else {
            send_agent_move(lp, state->x, state->y, dir, tw_now(lp) + 1.0);
        }

        // Telling neighbors, this cell is no longer available
        for (int i = 0; i < num_moves; i++) {
            send_cell_unavailable(lp, state->x, state->y, available_moves[i]);
        }
    } else {
        bf->c2 = 1;
        // No moves available - agent is stuck
        state->exit_dir = DIRECTION_none;
    }
}

static void handle_cell_unavailable(struct SearchCellState *state, tw_bf *bf, struct SearchMessage *msg, tw_lp *lp) {
    bf->c3 = state->available_dirs[msg->from_dir];
    state->available_dirs[msg->from_dir] = false;
}

// ================================= ROSS LP functions ===============================

void search_lp_init(struct SearchCellState *state, tw_lp *lp) {
    // Initialize from global grid
    if (!g_initial_grid) {
        fprintf(stderr, "Error: Grid not loaded!\n");
        return;
    }

    state->y = lp->id / g_grid_width;
    state->x = lp->id % g_grid_width;
    state->cell_type = g_initial_grid[grid_index(state->x, state->y)];
    state->was_visited = false;
    state->exit_dir = DIRECTION_none;

    // Initialize available directions based on neighbors
    int neighbors[4][2];
    bool valid[4];
    get_neighbors(state->x, state->y, neighbors, valid);

    for (int i = 0; i < 4; i++) {
        state->available_dirs[i] = valid[i];
    }

    // If this is the start cell, place the agent here
    if (state->x == g_start_x && state->y == g_start_y) {
        // Schedule first move after a small delay
        tw_event *e = tw_event_new(lp->gid, 1.0, lp);
        struct SearchMessage *msg = tw_event_data(e);
        msg->type = MESSAGE_TYPE_agent_move;
        msg->sender = lp->gid;
        msg->from_dir = DIRECTION_none;
        tw_event_send(e);
    }

    assert_valid_SearchCellState(state);
}

void search_lp_event_handler(struct SearchCellState *state, tw_bf *bf, struct SearchMessage *msg, tw_lp *lp) {
    memset(bf, 0, sizeof(*bf));
    switch (msg->type) {
        case MESSAGE_TYPE_agent_move:
            handle_agent_move(state, bf, msg, lp);
            break;
        case MESSAGE_TYPE_cell_unavailable:
            handle_cell_unavailable(state, bf, msg, lp);
            break;
    }

    assert_valid_SearchCellState(state);
    assert_valid_SearchMessage(msg);
}

void search_lp_event_rev_handler(struct SearchCellState *state, tw_bf *bf, struct SearchMessage *msg, tw_lp *lp) {
    switch (msg->type) {
        case MESSAGE_TYPE_agent_move:
            state->was_visited = false;
            state->exit_dir = DIRECTION_none;
            if (bf->c1) {
                tw_rand_reverse_unif(lp->rng);
                tw_rand_reverse_unif(lp->rng);
                if (bf->c4) {
                    tw_rand_reverse_unif(lp->rng);
                    send_agent_move_cloning_rev(lp);
                }
            }
            break;
        case MESSAGE_TYPE_cell_unavailable:
            state->available_dirs[msg->from_dir] = bf->c3;
            break;
    }
    assert_valid_SearchCellState(state);
}

void search_lp_event_commit(struct SearchCellState *state, tw_bf *bf, struct SearchMessage *msg, tw_lp *lp) {
    switch (msg->type) {
        case MESSAGE_TYPE_agent_move:
            if (bf->c0) {
                printf("Goal found at (%d,%d) at time %.2f!\n", state->x, state->y, tw_now(lp));
            }
            if (bf->c2) {
                printf("Agent stuck at (%d,%d) at time %.2f\n", state->x, state->y, tw_now(lp));
            }
            break;
        default:
            break;
    }
}

void search_lp_final(struct SearchCellState *state, tw_lp *lp) {
    // Write final state to global grid
    int idx = grid_index(state->x, state->y);
    g_visited_grid[idx] = state->was_visited;
    g_exit_dirs[idx] = state->exit_dir;

    assert_valid_SearchCellState(state);
}
