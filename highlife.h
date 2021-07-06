// Header file template for HighLife model
//
// This file includes:
// - The state and message structs
// - Extern'ed command line arguments
// - Custom mapping function prototypes (if needed)
// - Any other needed structs, enums, unions, or #defines

#ifndef _highlife_h
#define _highlife_h

#include "ross.h"

// The world for a HighLife simulation is composed of stacked mini-worlds, one per LP.
// The size of each mini-world is `W_WIDTH x W_HEIGHT`
#define W_WIDTH 20
#define W_HEIGHT (20 + 2)

// =================================== State struct ==================================

// State struct: this defines the state of each LP
typedef struct {
  int steps;            //< Number of HighLife steps executed (if a mini-world stays the
                        // same after one step of HighLife on its grid, it is not a step
                        // and it won't alter/bother neighbour)
  int next_beat_sent;   //< This boolean is used to determine whether the next heartbeat
                        // has already been produced by another event or it should be
                        // created by the current event being processed
  unsigned char *grid;  //< Pointer to the mini-world (grid)
  FILE *fp;             //< The file in which to save some stats and the grid at the start
                        // and end of simulation
} state;

// ============================ Message enums and structs ============================

// There are two types of messages
typedef enum {
  STEP,        //< This is a heartbeat. It asks the LP to perform a step of HighLife in
               // its grid
  ROW_UPDATE,  //< This tells the LP to replace its old row (either top or bottom) for the
               // row contained in the body
} message_type;

// A ROW_UPDATE message can be of two kinds
typedef enum {
  UP_ROW,    //< The row in the message should replace the top row in the grid
  DOWN_ROW,  //< The row in the message should replace the bottom row in the grid
} row_direction;

// Message struct: this contains all data sent in an event
// There is only one type of message, so we define all the space needed to store any of
// the reversible states as well as the data for the new update
typedef struct {
  message_type type;
  tw_lpid sender;              //< Unique identifier of the LP who sent the message
  row_direction dir;           //< In case `type` is ROW_UPDATE, this indicates the direction
  unsigned char row[W_WIDTH];  //< In case `type` is ROW_UPDATE, this contains the new row values
  unsigned char *rev_state;    //< Storing the previous state to be recovered by the reverse
                               // message handler
} message;

// ======================== Command Line Argument declarations =======================
extern unsigned int init_pattern;

// ================================ Global variables =================================
// This defines the LP types
//extern tw_lptype model_lps[];

// ============================== Function Declarations ==============================
// defined in highlife_driver.c:

/** Grid initialization and first heartbeat */
extern void highlife_init(state *s, tw_lp *lp);

/** Forward event handler */
extern void highlife_event(state *s, tw_bf *bf, message *in_msg, tw_lp *lp);

/** Reverse event handler */
extern void highlife_event_reverse(state *s, tw_bf *bf, message *in_msg, tw_lp *lp);

/** Commit event handler */
extern void highlife_event_commit(state *s, tw_bf *bf, message *in_msg, tw_lp *lp);

/** Cleaning and printing info before shut down */
extern void highlife_final(state *s, tw_lp *lp);

// defined in highlife_map.c:
/** Mapping of LPs to SEs */
extern tw_peid highlife_map(tw_lpid gid);

// ============================ Custom mapping prototypes ============================
/*
void model_cutom_mapping(void);
tw_lp * model_mapping_to_lp(tw_lpid lpid);
tw_peid model_map(tw_lpid gid);
*/

#endif  // _highlife_h
