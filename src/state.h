#ifndef SEARCH_STATE_H
#define SEARCH_STATE_H

/** @file
 * Definition of messages and LP states for search algorithm simulation.
 */

#include <ross.h>
#include <stdbool.h>
#include <assert.h>

/** Maximum dimensions for the search grid */
#define MAX_GRID_WIDTH 100
#define MAX_GRID_HEIGHT 100

// ================================ Enums ===================================

/** Cell types in the search grid */
enum CELL_TYPE {
  CELL_TYPE_free = 0,     /**< Free space, agent can move here */
  CELL_TYPE_obstacle = 1, /**< Obstacle, agent cannot move here */
  CELL_TYPE_start = 2,    /**< Starting position */
  CELL_TYPE_goal = 3      /**< Goal position */
};

/** Direction enumeration */
enum DIRECTION {
  DIRECTION_north = 0,  /**< North (up) */
  DIRECTION_south = 1,  /**< South (down) */
  DIRECTION_east = 2,   /**< East (right) */
  DIRECTION_west = 3,   /**< West (left) */
  DIRECTION_none = 4    /**< No direction / not set */
};

// ================================ Global variables ===============================

/** Global grid dimensions and data (set at runtime) */
extern int g_grid_width;
extern int g_grid_height;
extern int g_start_x, g_start_y;
extern int g_goal_x, g_goal_y;

/** Global grid arrays for initial state and final results (shared per PE) */
extern enum CELL_TYPE *g_initial_grid;  /**< Initial grid layout (read at init) */
extern bool *g_visited_grid;           /**< Final: which cells were visited (written at finalize) */
extern enum DIRECTION *g_exit_dirs;    /**< Final: exit direction from each cell (written at finalize) */

// ================================ State struct ===============================

/** State for each cell LP in the search simulation.
 * Each LP represents one cell in the grid.
 */
struct SearchCellState {
  int x, y;                    /**< Cell coordinates */
  enum CELL_TYPE cell_type;     /**< Type of this cell */
  bool available_dirs[4];      /**< Which directions are available (N,S,E,W) */
  bool was_visited;            /**< Whether agent has visited this cell */
  enum DIRECTION exit_dir;     /**< Direction agent exited (for path reconstruction) */
};

/** Helper functions for global grid access */
static inline int grid_index(int x, int y) {
    return y * g_grid_width + x;
}

static inline bool is_valid_position(int x, int y) {
    return x >= 0 && x < g_grid_width && y >= 0 && y < g_grid_height;
}

static inline bool is_valid_SearchCellState(struct SearchCellState *s) {
    return s->x >= 0 && s->x < g_grid_width &&
           s->y >= 0 && s->y < g_grid_height &&
           s->cell_type >= CELL_TYPE_free && s->cell_type <= CELL_TYPE_goal &&
           s->exit_dir >= DIRECTION_north && s->exit_dir <= DIRECTION_none;
}

static inline void assert_valid_SearchCellState(struct SearchCellState *s) {
#ifndef NDEBUG
    assert(s->x >= 0 && s->x < g_grid_width);
    assert(s->y >= 0 && s->y < g_grid_height);
    assert(s->cell_type >= CELL_TYPE_free && s->cell_type <= CELL_TYPE_goal);
    assert(s->exit_dir >= DIRECTION_north && s->exit_dir <= DIRECTION_none);
#endif
}

// ========================= Message enums and structs =========================

/** Types of messages in the search simulation */
enum MESSAGE_TYPE {
  MESSAGE_TYPE_agent_move,      /**< Agent moves to this cell */
  MESSAGE_TYPE_cell_unavailable /**< Notification that a neighbor cell is unavailable */
};

/** Message data for search simulation events */
struct SearchMessage {
  enum MESSAGE_TYPE type;
  tw_lpid sender;

  union {
    struct { // message type = cell_unavailable
      enum DIRECTION from_dir;     /**< Direction the notification came from */
    };
  };
};

static inline bool is_valid_SearchMessage(struct SearchMessage *msg) {
    if (msg->type != MESSAGE_TYPE_agent_move && msg->type != MESSAGE_TYPE_cell_unavailable) {
        return false;
    }
    if (msg->type == MESSAGE_TYPE_agent_move) {
        return msg->from_dir >= DIRECTION_north && msg->from_dir <= DIRECTION_none;
    }
    return true;
}

static inline void assert_valid_SearchMessage(struct SearchMessage *msg) {
#ifndef NDEBUG
    assert(msg->type == MESSAGE_TYPE_agent_move || msg->type == MESSAGE_TYPE_cell_unavailable);
    if (msg->type == MESSAGE_TYPE_agent_move) {
        assert(msg->from_dir >= DIRECTION_north && msg->from_dir <= DIRECTION_none);
    }
#endif
}

// ================================= LP function declarations ===============================

/** Cell initialization. */
void search_lp_init(struct SearchCellState *s, struct tw_lp *lp);

/** Forward event handler. */
void search_lp_event_handler(
        struct SearchCellState *s,
        struct tw_bf *bf,
        struct SearchMessage *in_msg,
        struct tw_lp *lp);

/** Reverse event handler. */
void search_lp_event_rev_handler(
        struct SearchCellState *s,
        struct tw_bf *bf,
        struct SearchMessage *in_msg,
        struct tw_lp *lp);

/** Commit event handler. */
void search_lp_event_commit(
        struct SearchCellState *s,
        struct tw_bf *bf,
        struct SearchMessage *in_msg,
        struct tw_lp *lp);

/** Cell finalization. */
void search_lp_final(struct SearchCellState *s, struct tw_lp *lp);

/** Exporting function to the director to schedule agent movement, to choose a path */
void send_agent_move(tw_lp *lp, int x, int y, enum DIRECTION direction, double at);

#endif /* SEARCH_STATE_H */
