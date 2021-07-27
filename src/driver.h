#ifndef HIGHLIFE_DRIVER_H
#define HIGHLIFE_DRIVER_H

/** @file
 * Functions implementing Highlife as PDES in ROSS.
 */

#include "state.h"

/** Setting global variables to by the simulation. */
void driver_config(int init_pattern);

/** Grid initialization and first heartbeat. */
void highlife_init(state *s, tw_lp *lp);

/** Forward event handler. */
void highlife_event(state *s, tw_bf *bf, message *in_msg, tw_lp *lp);

/** Reverse event handler. */
void highlife_event_reverse(state *s, tw_bf *bf, message *in_msg, tw_lp *lp);

/** Commit event handler. */
void highlife_event_commit(state *s, tw_bf *bf, message *in_msg, tw_lp *lp);

/** Cleaning and printing info before shut down. */
void highlife_final(state *s, tw_lp *lp);

#endif /* end of include guard */
