# Unequal error protection

## Dependencies
* GCC >= 6.3
* CMake >= 3.2
* Boost libraries >= 1.58
* Protobuf library and compiler >= 3.0

On Debian Stretch the required packages can be installed by running
`sudo apt-get install build-essential cmake libboost-all-dev
protobuf-compiler libprotobuf-dev libpthread-stubs0-dev`

## Build
This project uses the [CMake](https://cmake.org/) build system.  To
compile the project inside a subdirectory named `build` run, in the
project's main directory
* `mkdir -p build && cd build`
* `cmake -DCMAKE_BUILD_TYPE=Release ..`
* `make`

The compiled binaries will be in the `build/bin` subdirectory.

## Test
The automated unit tests can be run with `make test` from the `build`
directory.
