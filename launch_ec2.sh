#!/bin/bash

set -e -o pipefail
set -x

instance_id=$(aws ec2 run-instances \
		  --region us-east-1 \
		  --image-id ami-6dd58716 \
		  --count 1 \
		  --instance-type c4.large \
		  --subnet-id 'subnet-0c601120' \
		  --security-group-ids 'sg-d030c9a0' \
		  --key-name 'gpg auth key' \
		  --iam-instance-profile 'Name=uep_sim_server' \
		  --instance-initiated-shutdown-behavior 'terminate' \
		  --associate-public-ip-address | \
		  grep 'InstanceId' | sed 's/.*: "\(.*\)".*/\1/')

sleep 20

ipaddr=$(aws ec2 describe-instances \
	     --region us-east-1 \
	     --instance-ids "$instance_id" | \
	     grep 'PublicIpAddress' | sed 's/.*: "\(.*\)".*/\1/')

ssh -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no \
    admin@"$ipaddr" < 'mp_plots_ec2.sh'
