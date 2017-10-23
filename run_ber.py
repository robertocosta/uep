#!/usr/bin/env python3

import glob
import numpy as np
import os
import re
import subprocess

from subprocess import check_call, check_output, Popen

BUILD="FAILED"
PREPARE_VIDEO="FAILED"
RUN="FAILED"
PROCESS_OUTPUT="FAILED"
UPLOADED_FILES=""

def send_aws_mail(msg):
    tmpfile = check_output("mktemp")
    t = open(tmpfile, 'wt')
    t.write(msg + "\n")
    t.write("BUILD = {!s}\n".format(BUILD))
    t.write("PREPARE_VIDEO = {!s}\n".format(PREPARE_VIDEO))
    t.write("RUN = {!s}\n".format(RUN))
    t.write("PROCESS_OUTPUT = {!s}".format(PROCESS_OUTPUT))
    t.write("--- UPLOADED_FILES ---")
    t.write(UPLOADED_FILES)

    check_call(("aws sns publish"
                " --region 'us-east-1'"
                " --topic-arn"
                " 'arn:aws:sns:us-east-1:402432167722:NotifyMe'"
                " --message 'file://{!s}'").format(tmpfile),
               shell=True)
    os.remove(tmpfile)

try:
    # Get dependencies
    check_call("sudo apt-get update", shell=True)
    check_call(("sudo apt-get install -y"
                " build-essential"
                " cmake"
                " git"
                " libboost-all-dev"
                " libprotobuf-dev"
                " libpthread-stubs0-dev"
                " p7zip"
                " protobuf-compiler"),
               shell=True)

    # Clone repo
    check_call(("git clone --recursive"
                " 'https://github.com/riccz/uep.git'"),
               shell=True)
    os.chdir("uep")

    # Build
    check_call('''
    git checkout master
    mkdir -p build
    cd build
    cmake -DCMAKE_BUILD_TYPE=Release ..
    make
    cd ..
    ''', shell=True)

    BUILD="OK"

    # Prepare video
    check_call('''
    cd dataset
    wget 'http://trace.eas.asu.edu/yuv/stefan/stefan_cif.7z'
    7zr x stefan_cif.7z
    cd ..
    cd h264_scripts
    ./encode.sh
    cd ..
    ''', shell=True)

    PREPARE_VIDEO="OK"

    # Run
    os.chdir("build/bin")
    udp_bers = np.logspace(-8, -1, 40)
    for (b_i, b) in enumerate(udp_bers):
        srv_proc = Popen("./server"
                         " -p {:d}"
                        " -K '[100, 900]'"
                         " -R '[3, 1]'"
                         " -E 4"
                         " -n 6000"
                         #" -r 1000000"
                         " > server_console.log 2>&1".format(12312 + b_i),
                         shell=True)
        time.sleep(10)
        clt_proc = Popen("./client"
                         " -l {:d}"
                         " -r {:d}"
                         " -n stefan_cif_long"
                         " -t 30"
                         " -p {:e}"
                         " > client_console.log 2>&1".format(12345 + b_i,
                                                             12312 + b_i,
                                                             b),
                         shell=True)
        if not clt_proc.wait() == 0:
            raise subprocess.CalledProcessError(clt_proc.returncode,
                                                clt_proc.args)
        if not srv_proc.wait() == 0:
            raise subprocess.CalledProcessError(srv_proc.returncode,
                                                srv_proc.args)

        logfiles = glob.glob("*[!0-9].log")
        for l in logfiles:
            m = re.match("(.*)\.log", l)
            os.replace(l, "{!s}_{:02d}.log".format(m.group(1), b_i))

        break # DELETE ME

    # Upload logs
    subdir_name = check_output(["date", "+'%Y-%m-%d_%H-%M-%S'"])
    for l in glob.glob("*.log"):
        s3_url = "s3://uep.zanol.eu/simulation_logs/{!s}/{!s}".format(subdir_name,
                                                                      l)
        check_call("aws s3 cp '{!s}' '{!s}'".format(l, s3_url), shell=True)
        publink=check_output("aws s3 presign '{}'"
                             " --expires-in 31536000".format(s3_url),
                             shell=True)
        UPLOADED_FILES += "{!s}: {!s}\n".format(l, publink)
    os.chdir("../..")

    RUN="OK"

    # Process output video
    check_call('''
    cd h264_scripts
    ./decode.sh
    cd ..
    ''', shell=True)

    # # Upload videos
    # check_call('''
    # pushd dataset_client
    # aws s3 cp "stefan_cif_long.264" s3://uep.zanol.eu/simulation_videos/"$subdir_name"/"stefan_cif_long.264"
    # popd
    # ''', shell=True)

    PROCESS_OUTPUT="OK"

    send_aws_mail("run_ber is finished")

    # Shutdown
    #check_call("sudo poweroff", shell=True)

except subprocess.CalledProcessError as e:
    send_aws_mail("run_ber failed")
