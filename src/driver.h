#ifndef SEARCH_DRIVER_H
#define SEARCH_DRIVER_H

/** @file
 * Functions implementing search algorithm as PDES in ROSS.
 */

/** Setting grid map file for the simulation. */
void driver_config(const char *grid_map_file);

/** Initialize the driver (parse grid file). */
int driver_init(void);

/** Clean up driver resources. */
void driver_finalize(void);

/** Write final results to output file. */
void write_final_output(void);


#endif /* SEARCH_DRIVER_H */
