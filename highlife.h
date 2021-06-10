// Header file template for HighLife model
//
// This file includes:
// - the state and message structs
// - extern'ed command line arguments
// - custom mapping function prototypes (if needed)
// - any other needed structs, enums, unions, or #defines

#ifndef _highlife_h
#define _highlife_h

#include "ross.h"

// HighLife Parameters
#define W_WIDTH 20
#define W_HEIGHT 20

// Message enums and structs

typedef enum {
  STEP,
  ROW_UPDATE,
} message_type;

typedef enum {
  UP_ROW,
  DOWN_ROW,
} row_direction;

// Message struct: this contains all data sent in an event
typedef struct {
  message_type type;
  tw_lpid sender;
  row_direction dir;
  //unsigned char row[W_WIDTH];
  unsigned char rev_state[W_WIDTH * W_HEIGHT]; // Storing the previous state to be recovered by the reverse message
} message;

// State struct: this defines the state of each LP
typedef struct {
  int steps;
  unsigned char grid[W_WIDTH * W_HEIGHT];
} state;

// Command Line Argument declarations
extern unsigned int init_pattern;
//extern unsigned int world_width;
//extern unsigned int world_height;

// Global variables by main
// - this defines the LP types
//extern tw_lptype model_lps[];

// Function Declarations
// defined in model_driver.c:
extern void highlife_init(state *s, tw_lp *lp);
extern void highlife_event(state *s, tw_bf *bf, message *in_msg, tw_lp *lp);
extern void highlife_event_reverse(state *s, tw_bf *bf, message *in_msg, tw_lp *lp);
extern void highlife_final(state *s, tw_lp *lp);
// defined in highlife_map.c:
extern tw_peid highlife_map(tw_lpid gid);

/*
//Custom mapping prototypes
void model_cutom_mapping(void);
tw_lp * model_mapping_to_lp(tw_lpid lpid);
tw_peid model_map(tw_lpid gid);
*/

#endif  // _highlife_h
