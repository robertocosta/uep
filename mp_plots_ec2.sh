#!/bin/bash

set -e -o pipefail

# Get dependencies
sudo apt-get update
sudo apt-get install -y \
     build-essential \
     cmake \
     libboost-all-dev \
     protobuf-compiler \
     libprotobuf-dev \
     libpthread-stubs0-dev \
     git

# Clone repo
git clone https://github.com/riccz/uep.git

# Build
cd uep
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make

# Run
src/mp_plots

# Upload logs
subdir_name=$(date +'%Y-%m-%d_%H-%M-%S')
for l in *log; do
    aws s3 cp "$l" s3://uep.zanol.eu/simulation_logs/"$subdir_name"/"$l"
done

# Shutdown
sudo poweroff
