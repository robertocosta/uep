# Unequal error protection

## Dependencies
* GCC >= 4.9.2
* Python >= 3.4.2 with development headers
* Boost test library >= 1.55

On Debian Jessie the required packages can be installed by running
`sudo apt-get install build-essential python3-dev libboost-all-dev`

## Build
This project uses the [waf](https://waf.io/) build system.
To compile the project run, in the main directory
* `./waf configure`
* `./waf build`

The compiled binaries will be in the `build` directory.

To avoid having to call the build script from the project's main
directory you can copy `utils/waf` to `~/bin/waf`. Then the build
script can be called from any subdirectory of the project as `waf`.

## Test
The automated unit tests can be run with `./waf test`.
