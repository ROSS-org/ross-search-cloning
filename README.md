# HighLife in ROSS

This is a small model implementing [HighLife][] in [ROSS][], a (massively) parallel
discrete event simulator.

[HighLife]: https://conwaylife.com/wiki/OCA:HighLife
[ROSS]: https://github.com/ROSS-org/ROSS

# Compilation

The following are the instructions to download and compile:

```bash
git clone --depth=1 --single-branch https://github.com/ROSS-org/ROSS
git clone https://github.com/helq/ross-highlife
ln -s ../../ross-highlife ROSS/models
mkdir -P builds/highlife
cd builds/highlife
cmake ../../ROSS -DROSS_BUILD_MODELS=ON -DCMAKE_INSTALL_PREFIX="$(pwd)/binaries"
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
