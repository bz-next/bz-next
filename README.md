# BZ-Next: Modernizing BZFlag's Renderer

This repo is for active, messy development on a new graphics engine + UI widgets for BZFlag using Magnum and ImGUI

Check out the project homepage: [https://bz-next.github.io/](https://bz-next.github.io/)
## What is it?

BZ-Next is an experimental fork of BZFlag that redoes the rendering engine from the ground-up, using modern OpenGL, and the Magnum graphics library.

See the [about the project](https://bz-next.github.io/about) page for more information!

## What does it currently look like?

![Rats Nest by Winny](https://bz-next.github.io/assets/img/screen0.jpg)
Map: Rats Nest by Winny

![Urban Jungle by Army of One](https://bz-next.github.io/assets/img/screen1.jpg)
Map: Urban Jungle by Army of One

![Fairground](https://bz-next.github.io/assets/img/screen2.jpg)
Map: Fairground

## Building

Currently, BZ-Next has been built for Linux, and [cross-compiled for Windows using MinGW64](https://bz-next.github.io/building/).

The following instructions are for Fedora 39. If using another distro, package names and package install procedure may be different, but the overall process should be the same. For Windows MinGW instructions, see [Building](https://bz-next.github.io/building/).

### Install development tools

```
$ sudo dnf groupinstall "Development Tools"
$ sudo dnf install cmake g++ mesa-libGL-devel mesa-libGLU-devel SDL2-devel libpng-devel curl-devel c-ares-devel glew-devel ncurses-devel
```

### Clone repository and populate submodules

```
$ git clone https://github.com/bz-next/bz-next.git
$ cd bz-next
$ git submodule update --init --recursive
```

### Make a directory to contain build stuff

```
$ mkdir build
$ cd build
```

### Run cmake to configure the build

Specify any build options here. You can build a release build (more optimized) by specifying `-DCMAKE_BUILD_TYPE=Release` instead of `Debug`. `-DCMAKE_EXPORT_COMPILE_COMMANDS=On` generates a `compile-commands.json` that can be fed to `clangd` for editor instrumentation.

The following will build a debug build, and disable the server, bzadmin, etc:

```
$ cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=On -DCMAKE_BUILD_TYPE=Debug -DENABLE_CLIENT=TRUE -DENABLE_SERVER=FALSE -DENABLE_PLUGINS=FALSE ..
```

If you're building for an embedded device with GLES, add the following defines to your cmake step:

```
-DMAGNUM_TARGET_GLES=ON -DMAGNUM_TARGET_GLES2=ON
```

GLES builds currently disable multisampling (defaults to 4x on non-GLES builds), sets texture filters to nearest instead of linear, and sets anisotropy to minimums.

### Run make

```
$ make -j$(nproc)
```

### Running the client

The client looks for textures in `/data` from the directory from which it is run. It's best to execute the client from the main repo directory so that it can find everything it needs.

```
$ cd ..
$ build/Debug/bin/bzflag-next
```

If you built a Release client, the path would be `build/Release/bin/bzflag-next`