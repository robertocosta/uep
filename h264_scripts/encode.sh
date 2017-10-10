#!/bin/bash

set -eu -o pipefail
set -x

H264ENCODE="$(realpath ../jsvm/bin/H264AVCEncoderLibTestStaticd)"
H264DECODE="$(realpath ../jsvm/bin/H264AVCDecoderLibTestStaticd)"
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
cp "stefan_cif.yuv" "stefan_cif_long.yuv"
cp "stefan_cif.264" "stefan_cif_long.264"
for i in $(seq 0 $((ntimes - 2)) ); do
    cat stefan_gops.264 >> stefan_cif_long.264
    cat stefan_cif.yuv >> stefan_cif_long.yuv
done
rm "stefan_gops.264"

"$BSEXTR" -pt "stefan_cif_long.trace" "stefan_cif_long.264"

"$H264DECODE" "stefan_cif.264" "stefan_cif_out.yuv"
"$H264DECODE" "stefan_cif_long.264" "stefan_cif_long_out.yuv"

popd
