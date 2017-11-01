#!/bin/bash

set -eu -o pipefail

script="${1-}"
ec2type="${2-c4.large}"
size="${3-8}"

setup_script=<<EOF
  set -eu -o pipefail
  while true; do
    sudo apt-get update && \
    sudo apt-get install -y \
    build-essential \
    cmake \
    git \
    libboost-all-dev \
    libprotobuf-dev \
    libpthread-stubs0-dev \
    p7zip \
    protobuf-compiler \
    python3-boto3 \
    python3-dev \
    python3-matplotlib \
    python3-numpy \
    python3-scipy \
    screen \
    swig && \
    break
  done
  git clone --recursive "https://github.com/riccz/uep.git"
  pushd uep
  git checkout master
  mkdir -p build
  pushd build
  cmake -DCMAKE_BUILD_TYPE=Release ..
  make -j\$(( \$(nproc) + 1))
  popd
  popd
EOF

instance_id=$(aws ec2 run-instances \
                  --region us-east-1 \
                  --image-id ami-5cbe7326 \
                  --block-device-mappings "DeviceName=xvda,Ebs={DeleteOnTermination=true,VolumeSize=${size},VolumeType=gp2}" \
                  --count 1 \
                  --instance-type "${ec2type}" \
                  --subnet-id 'subnet-0c601120' \
                  --security-group-ids 'sg-d030c9a0' \
                  --key-name 'gpg auth key' \
                  --iam-instance-profile 'Name=uep_sim_server' \
                  --instance-initiated-shutdown-behavior 'terminate' \
                  --associate-public-ip-address | \
                  grep 'InstanceId' | sed 's/.*: "\(.*\)".*/\1/')

while true; do
    set +e +o pipefail
    descr_out=$(aws ec2 describe-instances \
                 --region us-east-1 \
                 --instance-ids "$instance_id")

    ipaddr=$(grep 'PublicIpAddress' <<< "$descr_out" | \
                 sed 's/.*: "\(.*\)".*/\1/')
    set -e -o pipefail

    if [ -n "$ipaddr" ]; then
        break
    fi
    sleep 10
done

while true; do
    set +e +o pipefail
    ssh -o ConnectTimeout=10 \
        -o UserKnownHostsFile=/dev/null \
        -o StrictHostKeyChecking=no \
        admin@"$ipaddr" <<< "${setup_script}"
    ssh_rc="$?"
    set -e -o pipefail

    if [ "$ssh_rc" == "0" ]; then
        break
    fi
    sleep 10
done

if [ -z "$script" ]; then
    # Open interactive SSH
    ssh -o ConnectTimeout=10 \
        -o UserKnownHostsFile=/dev/null \
        -o StrictHostKeyChecking=no \
        admin@"$ipaddr"
else
    # Send the script
    script_base64=$(base64 "$script")
    ssh -o ConnectTimeout=10 \
        -o UserKnownHostsFile=/dev/null \
        -o StrictHostKeyChecking=no \
        -t admin@"$ipaddr" <<EOF
          set -eu -o pipefail
          base64 -d <<< "${script_base64}" > "${script}"
          chmod a+x "${script}"
          screen -d -m -L screen.log python3 "${script}"
          set +eu +o pipefail
          screen -r
EOF
fi

# Local Variables:
# indent-tabs-mode: nil
# End:
