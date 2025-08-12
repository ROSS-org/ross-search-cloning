#include "driver.h"
#include "state.h"
#include <stdbool.h>
#include <ross.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ================================= Local variables ================================

static char *g_grid_map_file = NULL;

void driver_config(const char *grid_map_file) {
    if (g_grid_map_file) {
        free(g_grid_map_file);
    }
    g_grid_map_file = strdup(grid_map_file);
}


// ================================= Grid file parsing ===============================

static int parse_grid_file(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        fprintf(stderr, "Error: Cannot open grid file '%s'\n", filename);
        return -1;
    }

    char line[256];

    // Skip comments and find dimensions
    while (fgets(line, sizeof(line), fp)) {
        if (line[0] == '/' && line[1] == '/') continue;
        if (sscanf(line, "%d %d", &g_grid_width, &g_grid_height) == 2) {
            break;
        }
    }

    if (g_grid_width <= 0 || g_grid_height <= 0 ||
        g_grid_width > MAX_GRID_WIDTH || g_grid_height > MAX_GRID_HEIGHT) {
        fprintf(stderr, "Error: Invalid grid dimensions %dx%d\n", g_grid_width, g_grid_height);
        fclose(fp);
        return -1;
    }

    // Allocate global grids
    int total_cells = g_grid_width * g_grid_height;
    g_initial_grid = calloc(total_cells, sizeof(enum CellType));
    g_visited_grid = calloc(total_cells, sizeof(bool));
    g_exit_dirs = calloc(total_cells, sizeof(enum Direction));

    if (!g_initial_grid || !g_visited_grid || !g_exit_dirs) {
        fprintf(stderr, "Error: Failed to allocate grid memory\n");
        fclose(fp);
        return -1;
    }

    // Initialize exit directions to DIR_NONE
    for (int i = 0; i < total_cells; i++) {
        g_exit_dirs[i] = DIR_NONE;
    }

    // Parse grid content
    int y = 0;
    while (fgets(line, sizeof(line), fp) && y < g_grid_height) {
        if (line[0] == '/' && line[1] == '/') continue;

        int x = 0;
        for (char *p = line; *p && x < g_grid_width; p++) {
            if (*p == ' ' || *p == '\t') continue;

            enum CellType cell_type = CELL_FREE;
            switch (*p) {
                case '.': cell_type = CELL_FREE; break;
                case '#': cell_type = CELL_OBSTACLE; break;
                case 'S':
                    cell_type = CELL_START;
                    g_start_x = x;
                    g_start_y = y;
                    break;
                case 'G':
                    cell_type = CELL_GOAL;
                    g_goal_x = x;
                    g_goal_y = y;
                    break;
                default:
                    if (*p != '\n' && *p != '\r') {
                        fprintf(stderr, "Warning: Unknown character '%c' at (%d,%d), treating as free\n", *p, x, y);
                    }
                    continue;
            }

            g_initial_grid[grid_index(x, y)] = cell_type;
            x++;
        }
        y++;
    }

    fclose(fp);

    // Validate start and goal positions
    if (g_start_x < 0 || g_start_y < 0) {
        fprintf(stderr, "Error: No start position 'S' found in grid\n");
        return -1;
    }
    if (g_goal_x < 0 || g_goal_y < 0) {
        fprintf(stderr, "Error: No goal position 'G' found in grid\n");
        return -1;
    }

    if (g_tw_mynode == 0) {
        printf("Grid loaded: %dx%d, start=(%d,%d), goal=(%d,%d)\n",
               g_grid_width, g_grid_height, g_start_x, g_start_y, g_goal_x, g_goal_y);
    }

    return 0;
}

int driver_init(void) {
    if (!g_grid_map_file) {
        fprintf(stderr, "Error: No grid map file specified\n");
        return -1;
    }
    return parse_grid_file(g_grid_map_file);
}

void driver_finalize(void) {
    if (g_initial_grid) { free(g_initial_grid); g_initial_grid = NULL; }
    if (g_visited_grid) { free(g_visited_grid); g_visited_grid = NULL; }
    if (g_exit_dirs) { free(g_exit_dirs); g_exit_dirs = NULL; }
    if (g_grid_map_file) { free(g_grid_map_file); g_grid_map_file = NULL; }
}



// ================================= Output functions ================================

void write_final_output(void) {
    if (!g_visited_grid || !g_exit_dirs) return;

    FILE *fp = fopen("search-results.txt", "w");
    if (!fp) {
        fprintf(stderr, "Error: Cannot create output file\n");
        return;
    }

    fprintf(fp, "Search Algorithm Results\n");
    fprintf(fp, "Grid size: %dx%d\n", g_grid_width, g_grid_height);
    fprintf(fp, "Start: (%d,%d), Goal: (%d,%d)\n", g_start_x, g_start_y, g_goal_x, g_goal_y);
    fprintf(fp, "Goal reached: %s\n", g_visited_grid[grid_index(g_goal_x, g_goal_y)] ? "YES" : "NO");
    fprintf(fp, "\nGrid visualization:\n");

    // Pretty print the grid with directional arrows
    for (int y = 0; y < g_grid_height; y++) {
        for (int x = 0; x < g_grid_width; x++) {
            int idx = grid_index(x, y);
            enum CellType cell_type = g_initial_grid[idx];
            bool visited = g_visited_grid[idx];
            enum Direction exit_dir = g_exit_dirs[idx];

            char c = '.';
            if (cell_type == CELL_OBSTACLE) {
                c = '#';
            } else if (x == g_start_x && y == g_start_y) {
                c = 'S';
            } else if (x == g_goal_x && y == g_goal_y) {
                c = visited ? 'G' : 'g';  // Capital if reached
            } else if (visited) {
                switch (exit_dir) {
                    case DIR_NORTH: c = '^'; break;
                    case DIR_SOUTH: c = 'v'; break;
                    case DIR_EAST:  c = '>'; break;
                    case DIR_WEST:  c = '<'; break;
                    default: c = 'X'; break;  // Stuck
                }
            }

            fprintf(fp, "%c ", c);
        }
        fprintf(fp, "\n");
    }

    fclose(fp);
    printf("Results written to search-results.txt\n");
}



