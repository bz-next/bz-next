# BZFlag Magnum Experiments

This repo is for active, messy development on a new graphics engine + UI widgets for BZFlag using Magnum and ImGUI

## Dependencies

The dependencies are the same as regular BZFlag, except you'll also have to have cmake installed.

## Cloning and building
Here I'm assuming you want a debug build, and want to generate a compile-commands.json for use with `clangd`:
```
git clone https://github.com/guyfox2/bzflag-experiments.git
cd bzflag-experiments
git checkout magnum-experiment
git submodule update --init --recursive
mkdir build
cd build
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=On -DCMAKE_BUILT_TYPE=Debug ..
make -j$(nproc)
```
The binaries will be in `Debug/bin`
