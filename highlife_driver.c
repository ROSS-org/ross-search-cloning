// The C driver file for a ROSS model
// This file includes:
// - an initialization function for each LP type
// - a forward event function for each LP type
// - a reverse event function for each LP type
// - a finalization function for each LP type

// Includes
#include <stdio.h>

#include "highlife.h"
#include "ross.h"

static inline void HL_initAllZeros(state *s) {
  for (size_t i = 0; i < W_WIDTH * W_HEIGHT; i++) {
      s->grid[i] = 0;
  }
}

static inline void HL_initAllOnes(state *s) {
  HL_initAllZeros(s);
  for (size_t i = W_WIDTH; i < W_WIDTH * (W_HEIGHT - 1); i++) {
      s->grid[i] = 1;
  }
}

static inline void HL_initOnesInMiddle(state *s) {
  HL_initAllZeros(s);
  // Iterating over row 10
  for (size_t i = 10 * W_WIDTH; i < 11 * W_WIDTH; i++) {
    if ((i >= (10 * W_WIDTH + 10)) && (i < (10 * W_WIDTH + 20))) {
        s->grid[i] = 1;
    }
  }
}

static inline void HL_initOnesAtCorners(state *s, unsigned long self) {
  HL_initAllZeros(s);

  if (self == 0) {
    s->grid[W_WIDTH] = 1;                                   // upper left
    s->grid[2 * W_WIDTH - 1] = 1;                           // upper right
  } else if (self == g_tw_total_lps - 1) {
    s->grid[(W_WIDTH * (W_HEIGHT - 2))] = 1;                // lower left
    s->grid[(W_WIDTH * (W_HEIGHT - 2)) + W_WIDTH - 1] = 1;  // lower right
  }
}

static inline void HL_initSpinnerAtCorner(state *s, unsigned long self) {
  HL_initAllZeros(s);

  if (self == 0) {
    s->grid[W_WIDTH] = 1;            // upper left
    s->grid[W_WIDTH+1] = 1;            // upper left +1
    s->grid[2*W_WIDTH - 1] = 1;  // upper right
  }
}

static inline void HL_initReplicator(state *s, unsigned long self) {
  HL_initAllZeros(s);

  if (self == 0) {
    size_t x, y;
    x = W_WIDTH / 2;
    y = W_HEIGHT / 2;

    s->grid[x + y * W_WIDTH + 1] = 1;
    s->grid[x + y * W_WIDTH + 2] = 1;
    s->grid[x + y * W_WIDTH + 3] = 1;
    s->grid[x + (y + 1) * W_WIDTH] = 1;
    s->grid[x + (y + 2) * W_WIDTH] = 1;
    s->grid[x + (y + 3) * W_WIDTH] = 1;
  }
}

static inline void HL_initDiagonal(state *s) {
  HL_initAllZeros(s);

  for (int i = 0; i < W_WIDTH && i < W_HEIGHT; i++) {
      s->grid[(i+1)*W_WIDTH + i] = 1;
  }
}

void HL_printWorld(FILE *stream, state *s) {
  size_t i, j;

  fprintf(stream, "Print World - Iteration %d\n", s->steps);

  fprintf(stream, "Ghost row: ");
  for (j = 0; j < W_WIDTH; j++) {
    fprintf(stream, "%u ", (unsigned int)s->grid[j]);
  }
  fprintf(stream, "\n");

  for (i = 1; i < W_HEIGHT-1; i++) {
    fprintf(stream, "Row %2zu: ", i);
    for (j = 0; j < W_WIDTH; j++) {
      fprintf(stream, "%u ", (unsigned int)s->grid[(i * W_WIDTH) + j]);
    }
    fprintf(stream, "\n");
  }

  fprintf(stream, "Ghost row: ");
  for (j = 0; j < W_WIDTH; j++) {
    fprintf(stream, "%u ", (unsigned int)s->grid[(i * W_WIDTH) + j]);
  }
  fprintf(stream, "\n");
}

static inline unsigned int HL_countAliveCells(
        unsigned char *data, size_t x0, size_t x1, size_t x2, size_t y0, size_t y1, size_t y2) {
  // y1+x1 represents the position in the one dimensional array where
  // the point (y, x) would be located in 2D space, if the array was a
  // two dimensional array.
  // y1+x0 is the position to the left of (y, x)
  // y0+x1 is the position on top of (y, x)
  // y2+x2 is the position diagonally bottom, right to (y, x)
  return data[y0 + x0] + data[y0 + x1] + data[y0 + x2] + data[y1 + x0] + data[y1 + x2] + data[y2 + x0] +
         data[y2 + x1] + data[y2 + x2];
}

/// Serial version of standard byte-per-cell life.
void HL_iterateSerial(unsigned char *grid, unsigned char *grid_msg) {
  // The code seems more intricate than what it actually is.
  // The second and third loops are in charge of computing a single
  // transition of the game of life.
  // The first loop runs the transition a number of times (`iterations`
  // times).
  for (size_t y = 1; y < W_HEIGHT-1; ++y) {
    // This seems oddly complicated, but it is just obfuscated module
    // operations. The grid of the game of life fold on itself. It is
    // geometrically a donut/torus.
    //
    // `y1` is the position in the 1D array which corresponds to the
    // first column for the row `y`
    //
    // `x1` is the relative position of `x` with respect to the first
    // column in row `y`
    //
    // For example, let's assume that (y, x) = (2, 3) and the following
    // world:
    //
    //   0 1 2 3 4
    // 0 . . . . .
    // 1 . . . . .
    // 2 . . . # .
    // 3 . . . . .
    //
    // Then x1 = x = 3,
    // and  y1 = y * 5 = 10.
    // Which, when added up, give us the position in the 1D array
    //
    // . . . . . . . . . . . . . # . . . . . .
    // 0 1 2 3 4 5 6 7 8 9 1 1 1 1 1 1 1 1 1 1
    //                     0 1 2 3 4 5 6 7 8 9
    //
    // x0 is the column to the left of x1 (taking into account the torus geometry)
    // y0 is the row above to y1 (taking into account the torus geometry)
    size_t y0, y1, y2;
    y1 = y * W_WIDTH;
    y0 = ((y + W_HEIGHT - 1) % W_HEIGHT) * W_WIDTH;
    y2 = ((y + 1) % W_HEIGHT) * W_WIDTH;

    for (size_t x = 0; x < W_WIDTH; ++x) {
      // Computing important positions for x
      size_t x0, x1, x2;
      x1 = x;
      x0 = (x1 + W_WIDTH - 1) % W_WIDTH;
      x2 = (x1 + 1) % W_WIDTH;

      // Determining the next state for a single cell. We count how
      // many alive neighbours the current (y, x) cell has and
      // follow the rules for HighLife
      int neighbours = HL_countAliveCells(grid, x0, x1, x2, y0, y1, y2);
      if (grid[y1 + x1]) {  // if alive
        grid_msg[y1 + x1] = neighbours == 2 || neighbours == 3;
      } else {  // if dead
        grid_msg[y1 + x1] = neighbours == 3 || neighbours == 6;
      }
    }
  }
}

void HL_swap(unsigned char *grid, unsigned char *grid_msg, size_t n_cells) {
  unsigned char tmp;
  for (size_t i = 0; i < n_cells; i++) {
    tmp = grid_msg[i];
    grid_msg[i] = grid[i];
    grid[i] = tmp;
  }
}

static inline void copy_row(unsigned char *into, unsigned char *from) {
  for (int i = 0; i < W_WIDTH; i++) {
    into[i] = from[i];
  }
}

void send_tick(tw_lp *lp) {
  int self = lp->gid;
  tw_event *e = tw_event_new(self, 1, lp);
  message *msg = tw_event_data(e);
  msg->type = STEP;
  msg->sender = self;
  tw_event_send(e);
}

void print_row(unsigned char *row) {
  for (size_t i = 0; i < W_WIDTH; i++) {
    printf("%d ", row[i]);
  }
  printf("\n");
}

void send_rows(state *s, tw_lp *lp) {
  int self = lp->gid;

  // Sending rows to update
  int lp_id_up = (self + g_tw_total_lps - 1) % g_tw_total_lps;
  int lp_id_down = (self+1) % g_tw_total_lps;

  tw_event *e_drow = tw_event_new(lp_id_up, 0.5, lp);
  message *msg_drow = tw_event_data(e_drow);
  msg_drow->type = ROW_UPDATE;
  msg_drow->dir = DOWN_ROW; // Other LP's down row, not mine
  copy_row(msg_drow->row, s->grid + (W_WIDTH));
  tw_event_send(e_drow);

  tw_event *e_urow = tw_event_new(lp_id_down, 0.5, lp);
  message *msg_urow = tw_event_data(e_urow);
  msg_urow->type = ROW_UPDATE;
  msg_urow->dir = UP_ROW;
  copy_row(msg_urow->row, s->grid + (W_WIDTH * (W_HEIGHT - 2)));
  tw_event_send(e_urow);
}

// Init function
// - called once for each LP
void highlife_init(state *s, tw_lp *lp) {
  unsigned long self = lp->gid;

  switch (init_pattern) {
  case 0: HL_initAllZeros(s); break;
  case 1: HL_initAllOnes(s); break;
  case 2: HL_initOnesInMiddle(s); break;
  case 3: HL_initOnesAtCorners(s, self); break;
  case 4: HL_initSpinnerAtCorner(s, self); break;
  case 5: HL_initReplicator(s, self); break;
  case 6: HL_initDiagonal(s); break;
  default:
    printf("Pattern %u has not been implemented \n", init_pattern);
    /*exit(-1);*/
    MPI_Abort(MPI_COMM_WORLD, -1);
  }

  // Finding name for file
  const char *fmt = "output/highlife-gid=%d.txt";
  int sz = snprintf(NULL, 0, fmt, self);
  char filename[sz + 1]; // `+ 1` for terminating null byte
  snprintf(filename, sizeof(filename), fmt, self);

  s->fp = fopen(filename, "w");
  if (!s->fp) {
    perror("File opening failed\n");
    MPI_Abort(MPI_COMM_WORLD, -1);
  } else {
    HL_printWorld(s->fp, s);
    fprintf(s->fp, "\n");
  }

  // Tick message to myself
  send_tick(lp);
  // Sending rows to update
  send_rows(s, lp);
}

// Forward event handler
void highlife_event(state *s, tw_bf *bf, message *in_msg, tw_lp *lp) {
  // initialize the bit field
  (void)bf;
  //*(int *)bf = (int)0;

  // handle the message
  switch (in_msg->type) {
  case STEP:
    // Next step in the simulation (is stored in second parameter)
    HL_iterateSerial(s->grid, in_msg->rev_state);
    // Exchanging parameters from one site to the other
    HL_swap(s->grid, in_msg->rev_state, W_WIDTH * W_HEIGHT);
    s->steps++;
    // Sending tick for next STEP
    send_tick(lp);
    // Sending rows to update
    send_rows(s, lp);
    break;

  case ROW_UPDATE:
    switch (in_msg->dir) {
    case UP_ROW:
      HL_swap(s->grid, in_msg->row, W_WIDTH);
      break;
    case DOWN_ROW:
      HL_swap(s->grid + W_WIDTH*(W_HEIGHT-1), in_msg->row, W_WIDTH);
      /*HL_printWorld(stdout, s);*/
      break;
    }
    break;
  }

  // tw_output(lp, "Hello from %d\n", self);
}

// Reverse Event Handler
void highlife_event_reverse(state *s, tw_bf *bf, message *in_msg, tw_lp *lp) {
  (void)bf;
  (void)lp;
  // int self = lp->gid;

  // handle the message
  switch (in_msg->type) {
  case STEP:
    s->steps--;
    HL_swap(s->grid, in_msg->rev_state, W_WIDTH * W_HEIGHT);
    break;
  case ROW_UPDATE:
    switch (in_msg->dir) {
    case UP_ROW:
      HL_swap(s->grid, in_msg->row, W_WIDTH);
      break;
    case DOWN_ROW:
      HL_swap(s->grid + W_WIDTH*(W_HEIGHT-1), in_msg->row, W_WIDTH);
      break;
    }
    break;
  }
}

// report any final statistics for this LP
void highlife_final(state *s, tw_lp *lp) {
  int self = lp->gid;
  // Does not work! Once the model is closed, no output is processed
  // tw_output(lp, "%d handled %d Hello and %d Goodbye messages\n",
  //           self, s->rcvd_count_H, s->rcvd_count_G);
  printf("%d handled %d STEP messages\n", self, s->steps);

  if (!s->fp) {
    perror("File opening failed\n");
    MPI_Abort(MPI_COMM_WORLD, -1);
  } else {
    fprintf(s->fp, "%d handled %d STEP messages\n\n", self, s->steps);
    HL_printWorld(s->fp, s);
    fclose(s->fp);
  }
}
