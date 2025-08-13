# Search Algorithm in ROSS

This is a PDES model implementing a random search algorithm in [ROSS][], a (massively) parallel discrete event simulator. The model simulates an agent trying to find a goal in a grid world with obstacles by random exploration.

[ROSS]: https://github.com/ROSS-org/ROSS

## Algorithm Description

The search algorithm works as follows:

1. **Grid Setup**: A 2D grid is loaded from a file containing:
   - `.` = free space (agent can move here)
   - `#` = obstacle (agent cannot move here)
   - `S` = start position (where agent begins)
   - `G` = goal position (agent's target)

2. **Agent Behavior**:
   - Agent starts at the 'S' position
   - At each step, randomly selects an available direction (avoiding obstacles and previously visited cells)
   - Moves to the selected cell and marks it as visited
   - Notifies neighboring cells that this position is no longer available
   - Continues until reaching the goal or getting stuck (no available moves)

3. **PDES Implementation**:
   - Each grid cell is represented by a separate LP (Logical Process)
   - Agent movement and cell state updates are communicated via events
   - No shared mutable state during simulation (PDES compliant)

## Compilation

The following are the instructions to download and compile:

```bash
git clone --recursive https://github.com/ross-org/search-ross
mkdir search-ross/build
cd search-ross/build
cmake .. -DCMAKE_INSTALL_PREFIX="$(pwd -P)/"
make install
```

After compiling, you will find the executable under the folder: `build/src/`

## Grid File Format

Create a grid file with the following format:

```
// Comments start with //
// First non-comment line: width height
9 7
// Grid layout (one character per cell, spaces are ignored)
. . . . . . . . .
. S . . . # . . .
. . . . . . . . .
. . . . . # . # .
. # . # # . . . .
. . . . . . . G #
. . . # . . . . .
```

## Execution

Example usage:

```bash
cd build
src/search --grid-map=path/to/grid.txt
```

Required options:
- `--grid-map=FILE`: Path to the grid file
- `--end=TIME`: Simulation end time

The simulation will create a `search-results.txt` file showing:
- Whether the goal was reached
- Grid visualization with path taken:
  - `S` = start position
  - `G` = goal (reached) or `g` = goal (unreached)
  - `^>v<` = direction agent exited from each visited cell
  - `X` = agent got stuck here
  - `#` = obstacle
  - `.` = unvisited free space

## Example Output

```
Search Algorithm Results
Grid size: 9x7
Start: (1,1), Goal: (6,5)
Goal reached: YES

Grid visualization:
. . . . . . . . .
. S─────┐ # . . .
. . . . │ . . . .
. . . . │ # . # .
. # . # └───┐ . .
. . . . . . └─G #
. . . # . . . . .
```

## Documentation

To generate the documentation, install [doxygen][] and dot (included in [graphviz][]), and then run:

```bash
doxygen docs/Doxyfile
```

The documentation will be stored in `docs/html`.

[doxygen]: https://www.doxygen.nl/
[graphviz]: https://www.graphviz.org/

## Model Architecture

- **State**: Each cell LP maintains local state (position, type, visited status, available directions)
- **Events**:
  - `MESSAGE_TYPE_agent_move`: Agent arrives at a cell
  - `MESSAGE_TYPE_cell_unavailable`: Notification that a neighbor became unavailable
- **Global Data**: Grid layout and final results (read at init, written at finalize)
- **Output**: Path visualization using directional characters
