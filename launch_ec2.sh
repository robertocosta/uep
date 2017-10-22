#!/bin/bash

set -eu -o pipefail
set -x

script="$1"

instance_id=$(aws ec2 run-instances \
		  --region us-east-1 \
		  --image-id ami-5cbe7326 \
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
    descr_out=$(aws ec2 describe-instances \
		 --region us-east-1 \
		 --instance-ids "$instance_id")

    set +e +o pipefail
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
	    set -eux -o pipefail
	    base64 -d <<< "${script_base64}" > "${script}"
	    chmod a+x "${script}"
	    sudo apt-get update
	    sudo apt-get install -y screen
	    screen -d -m -L screen.log bash "${script}"
EOF
fi
