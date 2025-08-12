#include "state.h"
#include <stdio.h>
#include <string.h>

// ================================= Global variables ================================

// Grid dimensions and positions
int g_grid_width = 0;
int g_grid_height = 0;
int g_start_x = -1, g_start_y = -1;
int g_goal_x = -1, g_goal_y = -1;

// Global grid arrays
enum CellType *g_initial_grid = NULL;
bool *g_visited_grid = NULL;
enum Direction *g_exit_dirs = NULL;

// ================================= Helper functions ================================

static void get_neighbors(int x, int y, int neighbors[4][2], bool valid[4]) {
    // North, South, East, West
    int dx[] = {0, 0, 1, -1};
    int dy[] = {-1, 1, 0, 0};

    for (int i = 0; i < 4; i++) {
        neighbors[i][0] = x + dx[i];
        neighbors[i][1] = y + dy[i];
        valid[i] = is_valid_position(neighbors[i][0], neighbors[i][1]) &&
                   g_initial_grid[grid_index(neighbors[i][0], neighbors[i][1])] != CELL_OBSTACLE;
    }
}

static enum Direction opposite_direction(enum Direction dir) {
    switch (dir) {
        case DIR_NORTH: return DIR_SOUTH;
        case DIR_SOUTH: return DIR_NORTH;
        case DIR_EAST: return DIR_WEST;
        case DIR_WEST: return DIR_EAST;
        default: return DIR_NONE;
    }
}

static void send_agent_move(tw_lp *lp, int x, int y, enum Direction direction) {
    // North, South, East, West
    int dx[] = {0, 0, 1, -1};
    int dy[] = {-1, 1, 0, 0};

    tw_lpid target_gid = grid_index(x + dx[direction], y + dy[direction]);
    tw_event *e = tw_event_new(target_gid, 1.0, lp);
    struct SearchMessage *msg = tw_event_data(e);
    msg->type = MESSAGE_TYPE_agent_move;
    msg->sender = lp->gid;
    tw_event_send(e);
}

static void send_cell_unavailable(tw_lp *lp, int x, int y, enum Direction direction) {
    // North, South, East, West
    int dx[] = {0, 0, 1, -1};
    int dy[] = {-1, 1, 0, 0};

    tw_lpid target_gid = grid_index(x + dx[direction], y + dy[direction]);
    tw_event *e = tw_event_new(target_gid, 0.5, lp);
    struct SearchMessage *msg = tw_event_data(e);
    msg->type = MESSAGE_TYPE_cell_unavailable;
    msg->sender = lp->gid;
    msg->from_dir = opposite_direction(direction);
    tw_event_send(e);
}

// ================================= ROSS LP functions ===============================

void search_lp_init(struct SearchCellState *state, tw_lp *lp) {
    // Initialize from global grid
    if (!g_initial_grid) {
        fprintf(stderr, "Error: Grid not loaded!\n");
        return;
    }

    state->y = lp->gid / g_grid_width;
    state->x = lp->gid % g_grid_width;
    state->cell_type = g_initial_grid[grid_index(state->x, state->y)];
    state->was_visited = false;
    state->exit_dir = DIR_NONE;

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
        msg->from_dir = DIR_NONE;
        tw_event_send(e);
    }

    assert_valid_SearchCellState(state);
}

void search_lp_event_handler(struct SearchCellState *state, tw_bf *bf, struct SearchMessage *msg, tw_lp *lp) {
    memset(bf, 0, sizeof(*bf));
    switch (msg->type) {
        case MESSAGE_TYPE_agent_move:
            // Agent arrives at this cell
            state->was_visited = true;

            // If this is the goal, we're done!
            if (state->cell_type == CELL_GOAL) {
                bf->c0 = 1;
                return;
            }

            // Try to move to next cell
            enum Direction available_moves[4];
            int num_moves = 0;

            for (int i = 0; i < 4; i++) {
                if (state->available_dirs[i]) {
                    available_moves[num_moves++] = i;
                }
            }

            if (num_moves > 0) {
                bf->c1 = 1;
                // Pick random direction
                int choice = tw_rand_integer(lp->rng, 0, num_moves - 1);
                enum Direction dir = available_moves[choice];

                state->exit_dir = dir;

                // Send agent to next cell
                send_agent_move(lp, state->x, state->y, dir);
                // Informing cell is no longer available
                for (int i = 0; i < num_moves; i++) {
                    send_cell_unavailable(lp, state->x, state->y, available_moves[i]);
                }
            } else {
                bf->c2 = 1;
                // No moves available - agent is stuck
                state->exit_dir = DIR_NONE;
            }
            break;

        case MESSAGE_TYPE_cell_unavailable:
            bf->c3 = state->available_dirs[msg->from_dir];
            state->available_dirs[msg->from_dir] = false;
            break;
    }

    assert_valid_SearchMessage(msg);
}

void search_lp_event_rev_handler(struct SearchCellState *state, tw_bf *bf, struct SearchMessage *msg, tw_lp *lp) {
    switch (msg->type) {
        case MESSAGE_TYPE_agent_move:
            state->was_visited = false;
            state->exit_dir = DIR_NONE;
            if (bf->c1) {
                tw_rand_reverse_unif(lp->rng);
            }
            break;
        case MESSAGE_TYPE_cell_unavailable:
            state->available_dirs[msg->from_dir] = bf->c3;
            break;
    }
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
