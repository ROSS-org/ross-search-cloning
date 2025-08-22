# Cloning model - Search Algorithm in ROSS

This is a PDES model implementing a random search algorithm in [ROSS][], a (massively) parallel discrete event simulator. The model simulates an agent trying to find a goal in a grid world with obstacles by random exploration. When the agent encounters a decision to take, it asks to be cloned and both simulations are run in parallel.

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
git clone --recursive https://github.com/ross-org/ross-search-cloning
mkdir ross-search-cloning/build
cd ross-search-cloning/build
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
. # . # . . . . .
. . . . . . . G #
. . . # . . . . .
```

## Execution

### On 1 Core (1 PE)

Example usage in one core (it will not clone itself):

```bash
cd build
bin/search --grid-map=path/to/grid.txt
```

Options:
- `--grid-map=FILE`: Path to the grid file (required)
- `--end=TIME`: Simulation end time

The simulation will create a `search-results-pe=X.txt` file showing:
- Whether the goal was reached
- Grid visualization with path taken:
  - `S` = start position
  - `G` = goal (reached) or `g` = goal (unreached)
  - `^>v<` = direction agent exited from each visited cell
  - `X` = agent got stuck here
  - `#` = obstacle
  - `.` = unvisited free space

### On multiple Cores (PEs)

```bash
cd build
mpirun -np 20 bin/search --synch=3 --grid-map=path/to/grid.txt
```

This will run the simulation in parallel in 20 PEs. It will start on ONE core, and every single time the model wants to take a decision, it can ask to be cloned and thus take two paths.

## Example Output

The file `search-results-pe=X.txt` will contain the path that a particular simulation took:

```
Search Results on PE X
Grid size: 9x7
Start: (1,1), Goal: (7,5)
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

## Model Architecture

- **State**: Each cell LP maintains local state (position, type, visited status, available directions)
- **Events**:
  - `MESSAGE_TYPE_agent_move`: Agent arrives at a cell
  - `MESSAGE_TYPE_cell_unavailable`: Notification that a neighbor became unavailable
- **Global Data**: Grid layout and final results (read at init, written at finalize)
- **Output**: Path visualization using directional characters
