#ifndef HIGHLIFE_STATE_H
#define HIGHLIFE_STATE_H

/** @file
 * Definition of messages and LP states for this simulation.
 */

#include <ross.h>

/** The world for a HighLife simulation is composed of stacked mini-worlds, one
 * per LP. The size of each mini-world is `W_WIDTH x W_HEIGHT`.
 */
#define W_WIDTH 20
#define W_HEIGHT (20 + 2)

// ================================ State struct ===============================

/** This defines the state for all LPs. **/
struct HighlifeState {
  int steps;            /**< Number of HighLife steps executed by LP. If a
                          mini-world stays the same after one step of HighLife
                          on its grid, it is not a step and it won't
                          alter/bother neighbour. */
  int next_beat_sent;   /**< This boolean is used to determine whether the next
                          heartbeat. has already been produced by another event
                          or it should be created by the current event being
                          processed. */
  unsigned char *grid;  /**< Pointer to the mini-world (grid). */
  FILE *fp;             /**< The file in which to save some stats and the grid
                          at the start and end of simulation. */
};

// ========================= Message enums and structs =========================

/** There are two types of messages. */
enum MESSAGE_TYPE {
  MESSAGE_TYPE_step,        /**< This is a heartbeat. It asks the LP to perform
                              a step of HighLife in its grid. */
  MESSAGE_TYPE_row_update,  /**< This tells the LP to replace its old row
                              (either top or bottom) for the row contained in
                              the body. */
};

/** A MESSAGE_TYPE_row_update message can be of two kinds. */
enum ROW_DIRECTION {
  ROW_DIRECTION_up_row,    /**< The row in the message should replace the top
                             row in the grid. */
  ROW_DIRECTION_down_row,  /**< The row in the message should replace the
                             bottom row in the grid. */
};

/** This contains all data sent in an event.
 * There is only one type of message, so we define all the space needed to
 * store any of the reversible states as well as the data for the new update.
 */
struct Message {
  enum MESSAGE_TYPE type;
  tw_lpid sender;              /**< Unique identifier of the LP who sent the
                                 message. */
  union {
    struct { // message type = step
      unsigned char *rev_state;    /**< Storing the previous state to be
                                     recovered by the reverse message handler.
                                     */
    };
    struct { // message type = row_update
      enum ROW_DIRECTION dir;      /**< In case `type` is ROW_UPDATE, this
                                     indicates the direction. */
      unsigned char row[W_WIDTH];  /**< In case `type` is ROW_UPDATE, this
                                     contains the new row values. */
    };
  };
};

#endif /* end of include guard */
