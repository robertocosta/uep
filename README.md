# Unequal error protection

## Dependencies
* GCC >= 6.3
* CMake >= 3.2
* Boost libraries >= 1.62
* Protobuf library and compiler

On Debian Stretch the required packages can be installed by running
`sudo apt-get install build-essential cmake libboost-all-dev
protobuf-compiler libprotobuf-dev libpthread-stubs0-dev`

## Build
This project uses the [CMake](https://cmake.org/) build system.  To
compile the project inside a subdirectory named `build` run, in the
project's main directory
* `mkdir build && cd build`
* `cmake ..`
* `make`

The compiled binaries will be in the `build/src` and `build/test`
subdirectories.

## Test
The automated unit tests can be run with `make test` from the `build`
directory.
