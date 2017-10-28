#!/bin/bash

set -eu -o pipefail
set -x

H264ENCODE="$(realpath ../jsvm/bin/H264AVCEncoderLibTestStaticd)"
H264DECODE="$(realpath ../jsvm/bin/H264AVCDecoderLibTestStaticd)"
BSEXTR="$(realpath ../jsvm/bin/BitStreamExtractorStaticd)"

DATADIR="../dataset"
STREAMNAME="stefan_cif"
LONG_FRAMENO=16000
CFGDIR="$PWD"
pushd "$DATADIR"
cp "$CFGDIR"/${STREAMNAME}.cfg "$CFGDIR"/${STREAMNAME}_*.cfg ./

frameno=$("$H264ENCODE" -pf "${STREAMNAME}.cfg" 2>&1 | tee "${STREAMNAME}_encode.log" | grep "Frames:" | sed 's/^.*Frames: \([[:digit:]]*\)$/\1/')
"$BSEXTR" -pt "${STREAMNAME}.trace" "${STREAMNAME}.264" > "${STREAMNAME}_trace.log" 2>&1

# Repeat the GOPs to make a longer stream (>= LONG_FRAMENO frames)
set +o pipefail
gopoffset=$(grep 'SliceData' < "${STREAMNAME}.trace" | sed 's/^\([x0-9a-fA-F]*\).*/\1/' | head -n1)
set -o pipefail
dd if="${STREAMNAME}.264" of="${STREAMNAME}_gops.264" bs=1 skip="$((gopoffset))"

ntimes=$(( (LONG_FRAMENO + frameno - 1) / frameno ))
cp "${STREAMNAME}.yuv" "${STREAMNAME}_long.yuv"
cp "${STREAMNAME}.264" "${STREAMNAME}_long.264"
for i in $(seq 0 $((ntimes - 2)) ); do
    cat ${STREAMNAME}_gops.264 >> ${STREAMNAME}_long.264
    cat ${STREAMNAME}.yuv >> ${STREAMNAME}_long.yuv
done
rm "${STREAMNAME}_gops.264"

"$BSEXTR" -pt "${STREAMNAME}_long.trace" "${STREAMNAME}_long.264" > "${STREAMNAME}_long_trace.log" 2>&1

#"$H264DECODE" "${STREAMNAME}.264" "${STREAMNAME}_out.yuv" > "${STREAMNAME}_decode.log" 2>&1
#"$H264DECODE" "${STREAMNAME}_long.264" "${STREAMNAME}_long_out.yuv" > "${STREAMNAME}_long_decode.log" 2>&1

popd
