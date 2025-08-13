# Grid Search Algorithm in ROSS

A **Parallel Discrete Event Simulation (PDES)** implementation of a random search algorithm using the ROSS framework. This project demonstrates how an agent explores a 2D grid world to find a goal position while avoiding obstacles.

## Overview

This simulation models an agent that:
- Starts at position 'S' in a 2D grid
- Randomly explores available directions
- Avoids obstacles ('#') and previously visited cells
- Attempts to reach the goal ('G')
- Gets "stuck" if no valid moves remain

The implementation uses ROSS (Rensselaer Optimistic Simulation System) where each grid cell is a separate **Logical Process (LP)**, enabling parallel execution.

## Features

### Dual Visualization System

The project supports two visualization modes for displaying the agent's path:

#### Line Drawing Mode (Default)
```
┌ ─ ─ ┐ ┌ ─ ─ ┐ . . . .
│ S ┐ │ └ ─ ┐ │ . . . .
│ ┌ ┘ └ ┐ . │ │ . . . .
│ │ . . │ ┌ ┘ │ . . . .
└ ┘ . ┌ ┘ │ X │ . . . .
. . . └ ─ ┘ └ ┘ . . . .
. . . . . . . . . . . g
```
- Uses Unicode box-drawing characters (┌┐└┘│─)
- Shows smooth connected paths

#### ASCII-Only Mode
```
> > > v > > > v . . . .
^ S v v ^ < < v . . . .
^ v < > v . ^ v . . . .
^ v . . v > ^ v . . . .
^ < . v < ^ X v . . . .
. . . > > ^ ^ < . . . .
. . . . . . . . . . . g
```
- Uses directional arrows: `^v<>`
- Shows exit direction from each cell
- Enabled via CMake: `-DASCII_ONLY_VISUALIZATION=ON`

## File Structure

```
ross-search-clone/
├── src/                   # Core source code
│   ├── search.main.c      # Main ROSS application entry point
│   ├── driver.{c,h}       # Grid parsing & visualization output
│   ├── state.{c,h}        # LP state management & event handlers
│   ├── mapping.{c,h}      # LP-to-PE mapping functions
│   └── utils.{c,h}        # Utility functions
├── example-grids/         # Sample grid map files
│   ├── 7x9-v1.txt         # Basic grid with obstacles
│   ├── 12x8-empty.txt     # Open grid, no obstacles
│   ├── 7x9-impossible.txt # There is no path to goal
│   └── 12x8-empty.txt     # Larger with no obstacles
├── external/ROSS/         # ROSS framework (git submodule)
├── docs/                  # Doxygen documentation
└── CMakeLists.txt         # Build configuration
```

## Building and Installation

### Prerequisites
- **CMake** (≥ 4.0)
- **C Compiler** with C17 support (GCC/Clang)
- **MPI** implementation
- **Git** (for submodules)

### Quick Start
```bash
# Configure and build
mkdir build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX="$(pwd -P)"
make -j install

# The executable will be at: build/bin/search
```

### Build Options
```bash
# Enable ASCII-only visualization
cmake .. -DASCII_ONLY_VISUALIZATION=ON
```

## Usage

### Basic Execution
```bash
path-to/bin/search --grid-map=path-to/example-grids/7x9-v1.txt
```

### Output
Results are written to `search-results.txt`:
```
Search Algorithm Results
Grid size: 9x7
Start: (1,1), Goal: (7,5)
Goal reached: YES/NO

Grid visualization:
[Visual representation of path taken]
```

## Grid File Format

Grid files define the search environment:

```
// Lines starting with // are comments
// First non-comment line: width height
9 7
// Grid layout (spaces are ignored)
. . . . . . . . .
. S . . . # . . .
. . . . . . . . .
. . . . . # . # .
. # . # # . . . .
. . . . . . . G #
. . . # . . . . .
```

### Legend
| Character | Meaning |
|-----------|---------|
| `.` | Free space (agent can move) |
| `#` | Obstacle (blocks movement) |
| `S` | Start position (exactly one required) |
| `G` | Goal position (exactly one required) |

### Creating Custom Grids
1. First line: `width height` (integers)
2. Grid layout: `height` lines with `width` characters each
3. Comments allowed with `//` prefix
4. Spaces are ignored in grid layout
5. Must have exactly one 'S' and one 'G'

### Line Drawing Characters
| Character | Path Type |
|-----------|-----------|
| `│` | Vertical path |
| `─` | Horizontal path |
| `┌┐└┘` | Corner turns |

### ASCII Arrow Directions
| Arrow | Direction |
|-------|-----------|
| `^` | North (up) |
| `v` | South (down) |
| `>` | East (right) |
| `<` | West (left) |

## PDES Implementation Details

### Architecture
- **Logical Processes (LPs)**: Each grid cell is an independent LP
- **Events**: Agent movement and cell availability notifications
- **No Shared State**: Follows PDES principles for parallel execution
- **Optimistic Synchronization**: Uses ROSS's Time Warp mechanism

### Message Types
- `MESSAGE_TYPE_agent_move`: Agent arrives at a cell
- `MESSAGE_TYPE_cell_unavailable`: Neighbor becomes unavailable

### Event Handlers
- **Forward**: Process events and update state
- **Reverse**: Undo operations for rollback capability
- **Commit**: Output final results and statistics

## Algorithm Details

### Search Strategy
1. **Start**: Agent begins at 'S' position
2. **Exploration**: At each cell, randomly select from available directions
3. **Constraints**:
   - Cannot move to obstacles (#)
   - Cannot revisit previously visited cells
   - Cannot move outside grid boundaries
4. **Termination**:
   - **Success**: Reaches goal ('G')
   - **Failure**: No valid moves available (gets stuck)

### Randomization
- Uses ROSS's built-in PRNG for reproducible results
- Each cell LP maintains independent random state
- Direction selection is uniform random from available options

## Development

### Code Style
- Follows project style guide (`style-guide.md`)
- C17 standard compliance
- Extensive input validation and error checking
- Doxygen-compatible documentation

### Testing
```bash
# Run with example grids
make -j install
cd build

# Test different scenarios
mkdir test && cd test
path-to/bin/search --grid-map=path-to/example-grids/7x9-v1.txt
path-to/bin/search --grid-map=path-to/example-grids/12x8-empty.txt
path-to/bin/search --grid-map=path-to/example-grids/7x9-impossible.txt
```

### Contributing
- All structs include `is_valid_*` and `assert_valid_*` functions
- Memory management follows RAII principles where possible
- Use existing code patterns for consistency

## Troubleshooting

### Common Issues

**Visualization Issues**
```bash
# If Unicode characters don't display properly
cmake .. -DASCII_ONLY_VISUALIZATION=ON
make -j install
```

## Further Reading

- [ROSS Documentation](https://ross-org.github.io/)
- [Parallel Discrete Event Simulation](https://en.wikipedia.org/wiki/Discrete_event_simulation)
- [Time Warp Algorithm](https://en.wikipedia.org/wiki/Time_Warp_(simulation))
