#ifndef SEARCH_DIRECTOR_H
#define SEARCH_DIRECTOR_H

/** @file
 * Director module for handling GVT hooks and decision tracking.
 */

#include <ross.h>
#include <stdbool.h>
#include "state.h"

/** GVT hook function - called when triggered from model */
void clone_director_gvt_hook(tw_pe *pe, bool past_end_time);

/** Initialize the director module */
void director_init(void);

/** Store decision information for later printing via GVT hook */
void director_store_decision(int x, int y, enum Direction chosen_dir, enum Direction second_dir, tw_stime timestamp);

/** Cleanup the director module */
void director_finalize(void);

#endif /* SEARCH_DIRECTOR_H */
