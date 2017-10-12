#!/bin/bash

set -eu -o pipefail
set -x

H264DECODE="$(realpath ../jsvm/bin/H264AVCDecoderLibTestStaticd)"
BSEXTR="$(realpath ../jsvm/bin/BitStreamExtractorStaticd)"
PSNR="$(realpath ../jsvm/bin/PSNRStaticd)"
FILTER="$(realpath ../build/bin/filter_received)"

ORIGDIR="$(realpath ../dataset)"
DATADIR="../dataset_client"
STREAMNAME="stefan_cif_long"
STREAM_W=352
STREAM_H=288
pushd "$DATADIR"

"$BSEXTR" -pt "${STREAMNAME}.trace" "${STREAMNAME}.264" > "${STREAMNAME}_trace.log" 2>&1
"$FILTER" "$STREAMNAME" > "${STREAMNAME}_filter.log" 2>&1
"$H264DECODE" "${STREAMNAME}_filtered.264" "${STREAMNAME}_filtered.yuv" > "${STREAMNAME}_filtered_decode.log" 2>&1

size_orig=$(stat -c '%s' "${ORIGDIR}/${STREAMNAME}.yuv")
size_filt=$(stat -c '%s' "${STREAMNAME}_filtered.yuv")
[ "$size_orig" == "$size_filt" ] || \
    ( echo "There were frames fully lost" && exit 100 )
"$PSNR" "$STREAM_W" "$STREAM_H" "${ORIGDIR}/${STREAMNAME}.yuv" "${STREAMNAME}_filtered.yuv" > "${STREAMNAME}_filtered_psnr.log" 2>&1

popd
