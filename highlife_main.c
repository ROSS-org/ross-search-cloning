// The C main file for a ROSS model
// This file includes:
// - definition of the LP types
// - command line argument setup
// - a main function

// includes
#include "highlife.h"
#include "ross.h"

// Define LP types
//   these are the functions called by ROSS for each LP
//   multiple sets can be defined (for multiple LP types)
tw_lptype model_lps[] = {
    {(init_f)    highlife_init,
     (pre_run_f) NULL,
     (event_f)   highlife_event,
     (revent_f)  highlife_event_reverse,
     (commit_f)  NULL,
     (final_f)   highlife_final,
     (map_f)     highlife_map,
     sizeof(state)},
    {0},
};

// Define command line arguments default values
unsigned int init_pattern = 0;
/*unsigned int world_width = 20;*/
/*unsigned int world_height = 20;*/

// add your command line opts
const tw_optdef model_opts[] = {
    TWOPT_GROUP("HighLife"),
    TWOPT_UINT("pattern", init_pattern, "initial pattern for HighLife world"),
    /*TWOPT_UINT("width", world_width, "world width for LP"),*/
    /*TWOPT_UINT("height", world_height, "world height for LP"),*/
    TWOPT_END(),
};

// for doxygen
#define model_main main

int model_main(int argc, char *argv[]) {
  tw_opt_add(model_opts);
  tw_init(&argc, &argv);

  // Do some error checking?
  // Print out some settings?

  // Custom Mapping
  /*
  g_tw_mapping = CUSTOM;
  g_tw_custom_initial_mapping = &model_custom_mapping;
  g_tw_custom_lp_global_to_local_map = &model_mapping_to_lp;
  */

  // Useful ROSS variables and functions
  // tw_nnodes() : number of nodes/processors defined
  // g_tw_mynode : my node/processor id (mpi rank)

  // Useful ROSS variables (set from command line)
  // g_tw_events_per_pe
  // g_tw_lookahead
  // g_tw_nlp
  // g_tw_nkp
  // g_tw_synchronization_protocol
  // g_tw_total_lps

  // assume 1 lp in this node
  int num_lps_in_pe = 1;

  // set up LPs within ROSS
  tw_define_lps(num_lps_in_pe, sizeof(message));
  // note that g_tw_nlp gets set here by tw_define_lps

  // IF there are multiple LP types
  //    you should define the mapping of GID -> lptype index
  // g_tw_lp_typemap = &model_typemap;

  // set the global variable and initialize each LP's type
  g_tw_lp_types = model_lps;
  tw_lp_setup_types();

  // Do some file I/O here? on a per-node (not per-LP) basis

  tw_run();

  tw_end();

  return 0;
}
