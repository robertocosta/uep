#!/bin/bash

set -eu -o pipefail
set -x

H264ENCODE="$(realpath ../jsvm/bin/H264AVCEncoderLibTestStaticd)"
BSEXTR="$(realpath ../jsvm/bin/BitStreamExtractorStaticd)"

DATADIR="../dataset"
CFGDIR="$PWD"
pushd "$DATADIR"
cp "$CFGDIR"/stefan*.cfg ./

"$H264ENCODE" -pf "stefan.cfg"
"$BSEXTR" -pt "stefan_cif.trace" "stefan_cif.264"

# Repeat the GOPs to make a longer stream (>= 4000 frames)
set +o pipefail
gopoffset=$(grep 'SliceData' < "stefan_cif.trace" | sed 's/^\([x0-9a-fA-F]*\).*/\1/' | head -n1)
set -o pipefail
dd if="stefan_cif.264" of="stefan_gops.264" bs=1 skip="$((gopoffset))"

gopframes=90
ntimes=$(( (4000 + gopframes - 1) / gopframes ))
for i in $(seq 0 $((ntimes - 2)) ); do
    cat stefan_gops.264 >> stefan_cif.264
done
rm "stefan_gops.264"

"$BSEXTR" -pt "stefan_cif.trace" "stefan_cif.264"

popd
