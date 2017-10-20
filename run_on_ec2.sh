#!/bin/bash

set -eu -o pipefail
set -x

script="$1"

script_base64=$(base64 "$script")
./launch_ec2.sh <<EOF
set -eux -o pipefail
base64 -d <<< "${script_base64}" > "${script}"
chmod a+x "${script}"
sudo apt-get update
sudo apt-get install -y screen
screen -d -m -L screen.log bash "${script}"
EOF
