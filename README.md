# HighLife in ROSS

This is a small model implementing [HighLife][] in [ROSS][], a (massively) parallel
discrete event simulator, based on the [template model][template-model].

[HighLife]: https://conwaylife.com/wiki/OCA:HighLife
[ROSS]: https://github.com/ROSS-org/ROSS
[template-model]: https://github.com/ROSS-org/template-model

# Compilation

The following are the instructions to download and compile:

```bash
git clone --depth=1 --single-branch https://github.com/ROSS-org/ROSS
git clone https://github.com/helq/ross-highlife
ln -s ../../ross-highlife ROSS/models
mkdir -P builds/highlife
cd builds/highlife
cmake ../../ROSS -DROSS_BUILD_MODELS=ON -DCMAKE_INSTALL_PREFIX="$(pwd -P)/binaries"
make
make install
```

# Execution

An example of running in one core or two:

```bash
mkdir -P output
builds/highlife/binaries/bin/highlife
mpirun -np 2 builds/highlife/binaries/bin/highlife --sync=3 --batch=1 --pattern=5 --end=41
```

Inside the directory `output/` there will be two files containing the initial highlife
world and the final state after iteration 40.

Given the nature of interlocked execution for highlife, the best option is `--sync=2` not
`--sync=3`, i.e, conservative execution trumps over optimistic. This is because events can
be processed much faster than they are transmited between LPs (so, rollback eats a
considerable amount of time at execution time in the optimistic mode).

# Exercises for the reader

This HighLife model is meant mostly as an educational tool for the dear reader/developer
starting on ROSS development.

The following are a series of proposed exercises various characteristics of ROSS yet to be
implemented in the model:

- The code should be self-contained and easy to read. First, modify `W_WIDTH` and
   `W_HEIGHT` (keep values at least 6 for now), run the code, check the stats for each
   case and as well as the files under `output/`
- Allow as many LPs as the user wishes to use per PE, instead of 1 as it is currently
    encoded.
- Each LP holds a portion of the world of size `W_WIDTH x W_HEIGHT` and has only two
    neighbors, one above and one below. Modify this to allow for LPs to have neighbors at
    the sides as well. Now, there would be two variables, not only one per PE to idicate
    how many LPs would be per PE.
- Modify `W_WIDTH` and `W_HEIGHT`, run the code and check the stats again. What is the
    best value for `W_WIDTH` and `W_HEIGHT`?
- Create a new LP type that will be in charge of initializing the world removing such
    responsibility from the regular LPs. This requires to add new message type and a
    custom LP mapping. The LP mapping is in charge of determining which LP is in charge of
    initializing and which are regular grid portions.
