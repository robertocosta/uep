#!/bin/bash

set -eu -o pipefail
set -x

H264ENCODE="$(realpath ../jsvm/bin/H264AVCEncoderLibTestStaticd)"
H264DECODE="$(realpath ../jsvm/bin/H264AVCDecoderLibTestStaticd)"
BSEXTR="$(realpath ../jsvm/bin/BitStreamExtractorStaticd)"

DATADIR="../dataset"
STREAMNAME="stefan_cif"
CFGDIR="$PWD"
pushd "$DATADIR"
cp "$CFGDIR"/${STREAMNAME}.cfg "$CFGDIR"/${STREAMNAME}_*.cfg ./

"$H264ENCODE" -pf "${STREAMNAME}.cfg"
"$BSEXTR" -pt "${STREAMNAME}.trace" "${STREAMNAME}.264"

# Repeat the GOPs to make a longer stream (>= 4000 frames)
set +o pipefail
gopoffset=$(grep 'SliceData' < "${STREAMNAME}.trace" | sed 's/^\([x0-9a-fA-F]*\).*/\1/' | head -n1)
set -o pipefail
dd if="${STREAMNAME}.264" of="${STREAMNAME}_gops.264" bs=1 skip="$((gopoffset))"

gopframes=90
ntimes=$(( (4000 + gopframes - 1) / gopframes ))
cp "${STREAMNAME}.yuv" "${STREAMNAME}_long.yuv"
cp "${STREAMNAME}.264" "${STREAMNAME}_long.264"
for i in $(seq 0 $((ntimes - 2)) ); do
    cat ${STREAMNAME}_gops.264 >> ${STREAMNAME}_long.264
    cat ${STREAMNAME}.yuv >> ${STREAMNAME}_long.yuv
done
rm "${STREAMNAME}_gops.264"

"$BSEXTR" -pt "${STREAMNAME}_long.trace" "${STREAMNAME}_long.264"

"$H264DECODE" "${STREAMNAME}.264" "${STREAMNAME}_out.yuv"
"$H264DECODE" "${STREAMNAME}_long.264" "${STREAMNAME}_long_out.yuv"

popd
