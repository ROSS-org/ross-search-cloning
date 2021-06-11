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
mpirun -np 2 builds/highlife/binaries/bin/highlife --sync=2
```
