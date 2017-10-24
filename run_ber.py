#!/usr/bin/env python3

import datetime
import glob
import os
import re
import subprocess
import time
import sys

import boto3
import botocore
import numpy as np

from subprocess import check_call, check_output, Popen, call

PROGRESS = {"BUILD": "FAILED",
            "PREPARE_VIDEO": "FAILED",
            "RUN": "FAILED",
            "PROCESS_OUTPUT": "FAILED",
            "UPLOADED_FILES": []}

def send_aws_mail(msg):
    global PROGRESS
    run_data = PROGRESS

    if run_data is not None:
        msg += "\n"
        msg += "BUILD = {!s}\n".format(run_data["BUILD"])
        msg += "PREPARE_VIDEO = {!s}\n".format(run_data["PREPARE_VIDEO"])
        msg += "RUN = {!s}\n".format(run_data["RUN"])
        msg += "PROCESS_OUTPUT = {!s}\n".format(run_data["PROCESS_OUTPUT"])

        msg += "--- UPLOADED_FILES ---\n"
        for f in run_data["UPLOADED_FILES"]:
            msg += "{!s}: {!s}\n".format(f[0], f[1])

    client = boto3.client('sns', region_name='us-east-1')
    r = client.publish(TopicArn='arn:aws:sns:us-east-1:402432167722:NotifyMe',
                       Message=msg)
    if not r['MessageId']: raise RuntimeError("SNS publish failed")

def build():
    global PROGRESS

    # Get dependencies
    while True:
        try:
            check_call(["sudo", "apt-get", "update"])
            check_call(["sudo", "apt-get", "install" ,"-y",
                        "build-essential",
                        "cmake",
                        "git",
                        "libboost-all-dev",
                        "libprotobuf-dev",
                        "libpthread-stubs0-dev",
                        "p7zip",
                        "protobuf-compiler"])
            break
        except Exception as e:
            continue
    print("Dependencies installed")

    # Clone repo
    check_call(["git", "clone", "--recursive",
                "https://github.com/riccz/uep.git"])
    print("Repo cloned")

    # Build
    check_call('''
    set -e
    cd uep
    git checkout master
    mkdir -p build
    cd build
    cmake -DCMAKE_BUILD_TYPE=Release ..
    make
    ''', shell=True)
    print("UEP built")
    PROGRESS['BUILD'] = 'OK'

def prepare_video():
    global PROGRESS

    config = botocore.client.Config(read_timeout=300)
    s3 = boto3.resource('s3', region_name='us-east-1', config=config)
    bucket = s3.Bucket('uep.zanol.eu')

    # Download if exists
    try:
        bucket.download_file('dataset.tar.xz', 'uep/dataset.tar.xz')
    except botocore.exceptions.ClientError as e:
        if e.response['Error']['Code'] == "404":
            exists = False
        else:
            raise
    else:
        exists = True

    if not exists: # Generate the dataset
        check_call('''
        set -e
        cd uep/dataset
        wget 'http://trace.eas.asu.edu/yuv/stefan/stefan_cif.7z'
        7zr x stefan_cif.7z
        cd ../h264_scripts
        ./encode.sh
        cd ..
        tar -cJf dataset.tar.xz dataset/
        ''', shell=True)
        r = dataset.put(body=open('dataset.tar.xz', 'rb'))
        print("Uploaded dataset")
        PROGRESS['PREPARE_VIDEO'] = "OK"
    else:
        check_call('''
        set -e
        cd uep
        tar -xJf dataset.tar.xz
        ''', shell=True)
        print("Extracted downloaded dataset")
        PROGRESS['PREPARE_VIDEO'] = "CACHED"
    os.remove('uep/dataset.tar.xz')

def run():
    global PROGRESS

    os.chdir("uep/build/bin")

    udp_bers = np.logspace(-4, -0.1, 24)
    avg_bad_run = 500
    for (b_i, b) in enumerate(udp_bers):
        srv_clog = open("server_console.log", "wt")
        srv_tcp_port = 12312 + b_i
        srv_proc = Popen(["./server",
                          "-p", str(srv_tcp_port),
                          "-K", "[200, 4000]",
                          "-R", "[5, 1]",
                          "-E", "1",
                          "-n", "5500"
                          "-r", "500000"],
                         stdout=srv_clog,
                         stderr=srv_clog)

        time.sleep(10)

        clt_clog = open("client_console.log", "wt")
        clt_udp_port = 12345 + b_i
        clt_proc = Popen(["./client",
                          "-l", str(clt_udp_port),
                          "-r", str(srv_tcp_port),
                          "-n", "stefan_cif",
                          "-t", "30",
                          "-p", "[{:e}, {:e}]".format(1/avg_bad_run * b/(1-b),
                                                      1/avg_bad_run)],
                         stdout=clt_clog,
                         stderr=clt_clog)

        if not (clt_proc.wait() == 0):
            raise subprocess.CalledProcessError(clt_proc.returncode,
                                                clt_proc.args)
        if not (srv_proc.wait() == 0):
            raise subprocess.CalledProcessError(srv_proc.returncode,
                                                srv_proc.args)
        print("Run ({:d}/{:d}) with udp_ber={:e} OK".format(b_i+1, len(udp_bers), b))

        logfiles = glob.glob("*[!0-9].log") # Exclude the already renamed ones
        for l in logfiles:
            m = re.match("(.*)\.log", l)
            new_l = "{!s}_{:02d}.log".format(m.group(1), b_i)
            os.replace(l, new_l)
            check_call(["tar", "-cJf", new_l + ".tar.xz", new_l])
            os.remove(new_l)

    # Upload logs
    subdir_name = format(datetime.datetime.now(), "%Y-%m-%d_%H-%M-%S")
    s3 = boto3.client('s3', region_name='us-east-1')
    for l in glob.glob("*.log.tar.xz"):
        objname = "simulation_logs/{!s}/{!s}".format(subdir_name, l)
        s3.put_object(Body=open(l, 'rb'), Bucket='uep.zanol.eu', Key=objname)
        publink = s3.generate_presigned_url('get_object',
                                            Params={'Bucket': 'uep.zanol.eu',
                                                    'Key': objname},
                                            ExpiresIn=31536000)
        PROGRESS['UPLOADED_FILES'].append((l, publink))

    os.chdir("../..")
    PROGRESS['RUN'] = "OK"

def process_output():
    global PROGRESS

    check_call('''
    set -e
    cd uep/h264_scripts
    ./decode.sh
    ''', shell=True)

    # Upload videos
    check_call('''
    set -e
    cd uep/dataset_client
    aws s3 cp "stefan_cif_long.264" s3://uep.zanol.eu/simulation_videos/"$subdir_name"/"stefan_cif_long.264"
    ''', shell=True)

    PROGRESS['PROCESS_OUTPUT'] = "OK"

if __name__ == "__main__":
    try:
        build()
        prepare_video()
        run()
        # process_output()
        send_aws_mail("run_ber completed")

        # Shutdown
        check_call(["sudo", "poweroff"])

    except Exception as e:
        send_aws_mail("run_ber failed")
