#!/bin/bash

set -eu -o pipefail
set -x

BUILD=FAILED
PREPARE_VIDEO=FAILED
RUN=FAILED
PROCESS_OUTPUT=FAILED

function send_aws_mail {
    aws sns publish \
	--region 'us-east-1' \
	--topic-arn 'arn:aws:sns:us-east-1:402432167722:NotifyMe' \
	--message "$1\nBUILD = ${BUILD}\nPREPARE_VIDEO=${PREPARE_VIDEO}\nRUN=${RUN}\nPROCESS_OUTPUT=${PROCESS_OUTPUT}"
}

trap "send_aws_mail 'There was an error'" ERR

# Get dependencies
sudo apt-get update
sudo apt-get install -y \
     build-essential \
     cmake \
     git \
     libboost-all-dev \
     libprotobuf-dev \
     libpthread-stubs0-dev \
     p7zip \
     protobuf-compiler

# Clone repo
git clone --recursive https://github.com/riccz/uep.git

# Build
cd uep
git checkout master
mkdir -p build
pushd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make
popd

BUILD=OK

# Prepare video
pushd dataset
wget 'http://trace.eas.asu.edu/yuv/stefan/stefan_cif.7z'
7zr x stefan_cif.7z
popd
pushd h264_scripts
./encode.sh
popd

PREPARE_VIDEO=OK

# Run
pushd build/bin
./server -K '[50, 950]' -R '[1, 1]' -E 1 -n 1080 > server_console.log 2>&1 &
./client -n stefan_cif_long -t 30 -p 0 > client_console.log 2>&1 &
wait

# Upload logs
subdir_name=$(date +'%Y-%m-%d_%H-%M-%S')
for l in *.log; do
    aws s3 cp "$l" s3://uep.zanol.eu/simulation_logs/"$subdir_name"/"$l"
done
popd

RUN=OK

# Process output video
pushd h264_scripts
./decode.sh
popd

# Upload videos
pushd dataset_client
aws s3 cp "stefan_cif_long.264" s3://uep.zanol.eu/simulation_videos/"$subdir_name"/"stefan_cif_long.264"
popd

PROCESS_OUTPUT=OK

send_aws_mail "run_ber is finished ($subdir_name)"

# Shutdown
sudo poweroff
