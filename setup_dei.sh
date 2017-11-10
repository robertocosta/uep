#!/bin/bash

set -eu -o pipefail

dei_user=${1}

pushd build
make -j$(( $(nproc) + 1))
popd

ssh ${dei_user}@login.dei.unipd.it 'mkdir -p ~/uep_run'
scp \
    build/lib/mppy.so \
    run_uep_iid.job \
    run_uep_iid.py \
    uep.py \
    uep_random.py \
    ${dei_user}@login.dei.unipd.it:~/uep_run/
ssh -t ${dei_user}@login.dei.unipd.it 'cd ~/uep_run && $SHELL -l'
