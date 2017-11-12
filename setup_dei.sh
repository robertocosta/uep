#!/bin/bash

set -eu -o pipefail

dei_user=${1}
build_dir="build_dei"
cmake_dir="$PWD"

mkdir -p "$build_dir"
pushd "$build_dir"
cmake -DCMAKE_BUILD_TYPE=Release "$cmake_dir"
make -j$(( $(nproc) + 1)) mppy
popd

git log -1 --format='%H' > git_commit_sha1

ssh ${dei_user}@login.dei.unipd.it 'mkdir -p ~/uep_run'
scp \
    "${build_dir}/lib/mppy.so" \
    git_commit_sha1 \
    run_uep_iid.job \
    run_uep_iid.py \
    run_uep_markov.job \
    run_uep_markov.py \
    src/channel.py \
    uep.py \
    uep_random.py \
    ${dei_user}@login.dei.unipd.it:~/uep_run/
ssh -t ${dei_user}@login.dei.unipd.it 'cd ~/uep_run && $SHELL -l'
