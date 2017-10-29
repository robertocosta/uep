#!/bin/bash

set -eu -o pipefail

script="${1-}"

instance_id=$(aws ec2 run-instances \
                  --region us-east-1 \
                  --image-id ami-5cbe7326 \
                  --block-device-mappings "DeviceName=xvda,Ebs={DeleteOnTermination=true,VolumeSize=32,VolumeType=gp2}" \
                  --count 1 \
                  --instance-type c4.large \
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
        admin@"$ipaddr" <<< "echo 'SSH ok'"
    ssh_rc="$?"
    set -e -o pipefail

    if [ "$ssh_rc" == "0" ]; then
        break
    fi
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
        admin@"$ipaddr" <<EOF
            set -eu -o pipefail
            base64 -d <<< "${script_base64}" > "${script}"
            chmod a+x "${script}"
            sudo apt-get update
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
                 python3-matplotlib \
                 python3-numpy \
                 screen
            git clone --recursive "https://github.com/riccz/uep.git"
            pushd uep
            git checkout master
            mkdir -p build
            pushd build
            cmake -DCMAKE_BUILD_TYPE=Release ..
            make
            popd
            popd
            screen -d -m -L screen.log python3 "${script}"
EOF
    echo "Started ${script} at ${ipaddr}"
fi

# Local Variables:
# indent-tabs-mode: nil
# End:
