#include "driver.h"
#include "state.h"
#include "mapping.h"
#include "director.h"
#include <search_config.h>

/** Defining LP types.
 * - These are the functions called by ROSS for each LP
 * - Multiple sets can be defined (for multiple LP types)
 */
tw_lptype model_lps[] = {
    {(init_f)    search_lp_init,
     (pre_run_f) NULL,
     (event_f)   search_lp_event_handler,
     (revent_f)  search_lp_event_rev_handler,
     (commit_f)  search_lp_event_commit,
     (final_f)   search_lp_final,
     (map_f)     search_lp_map,
     sizeof(struct SearchCellState)},
    {0},
};

/** Define command line arguments default values. */
static char grid_map_file[128] = {'\0'};

/** Custom search algorithm command line options. */
static tw_optdef const model_opts[] = {
    TWOPT_GROUP("Search Algorithm"),
    TWOPT_CHAR("grid-map", grid_map_file, "grid map file path"),
    TWOPT_END(),
};

int main(int argc, char *argv[]) {
    tw_opt_add(model_opts);
    tw_init(&argc, &argv);

    // Check if required grid map file was provided
    if (grid_map_file[0] == '\0') {
        if (g_tw_mynode == 0) {
            fprintf(stderr, "Error: --grid-map option is required\n");
            fprintf(stderr, "Usage: %s --grid-map=path/to/grid.txt [other options]\n", argv[0]);
        }
        tw_end();
        return -1;
    }

    // Configure driver with grid map file
    driver_config(grid_map_file);

    // Initialize the grid (parse file, allocate memory)
    if (driver_init() != 0) {
        if (g_tw_mynode == 0) {
            fprintf(stderr, "Error: Failed to initialize grid\n");
        }
        tw_end();
        return -1;
    }

    // Print version info
    if (g_tw_mynode == 0) {
        printf("Search algorithm git version: " MODEL_VERSION "\n");
    }

    // Initialize director module for decision tracking
    director_init();

    // Set up GVT hook for decision tracking
    g_tw_gvt_hook = clone_director_gvt_hook;
    tw_trigger_gvt_hook_when_model_calls();

    // Calculate number of LPs needed (one per grid cell)
    int total_lps = g_grid_width * g_grid_height;

    // ROSS expects us to set g_tw_nlp (number of LPs per PE)
    // For simplicity, we'll put all LPs on one PE
    g_tw_nlp = total_lps;

    // Set up LPs within ROSS
    tw_define_lps(g_tw_nlp, sizeof(struct SearchMessage));

    // Set the global variable and initialize each LP's type
    g_tw_lp_types = model_lps;
    tw_lp_setup_types();

    // Run the simulation
    tw_run();

    // Write final output (called after all LPs have finished)
    write_final_output();

    // Clean up
    driver_finalize();
    director_finalize();
    tw_end();

    return 0;
}
