#!/usr/bin/env python3

import sys
import os

import boto3
import matplotlib.pyplot as plt
import numpy as np
import botocore

from subprocess import check_call

from ber import *

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: {!s} <s3 log dir>".format(sys.argv[0]))
        sys.exit(2)
    s3dir = sys.argv[1]

    config = botocore.client.Config(read_timeout=300)
    s3 = boto3.client('s3', config=config, region_name='us-east-1')
    # loglist = s3.list_objects_v2(Bucket='uep.zanol.eu',
    #                              Prefix=s3dir + '/')
    # if loglist.IsTruncated: raise RuntimeError("Too long")

    udp_err_rates = []
    uep_err_rates = []

    udp_bers = np.logspace(-8, -1, 10)
    for (b_i, b) in enumerate(udp_bers):
        srvname = "server_{:02d}.log".format(b_i)
        srvkey = "simulation_logs/{!s}/{!s}.tar.xz".format(s3dir, srvname)
        serverlog = s3.download_file(Bucket='uep.zanol.eu',
                                     Key=srvkey,
                                     Filename=srvname + ".tar.xz")
        check_call(["tar", "-xJf", srvname + ".tar.xz"])
        os.remove(srvname + ".tar.xz")

        clientname = "client_{:02d}.log".format(b_i)
        clientkey = "simulation_logs/{!s}/{!s}.tar.xz".format(s3dir, clientname)
        clientlog = s3.download_file(Bucket='uep.zanol.eu',
                                     Key=clientkey,
                                     Filename=clientname + ".tar.xz")
        check_call(["tar", "-xJf", clientname + ".tar.xz"])
        os.remove(clientname + ".tar.xz")

        bs = ber_scanner(srvname, clientname)
        bs.scan()

        udp_err_rates.append(bs.udp_err_rate)
        uep_err_rates.append(bs.uep_err_rates)

        print("{:02d} OK".format(b_i))
        os.remove(srvname)
        os.remove(clientname)


    mib_err_rates = list(map(lambda x: x[0], uep_err_rates))
    lib_err_rates = list(map(lambda x: x[1], uep_err_rates))

    plt.figure()
    plt.scatter(udp_err_rates, mib_err_rates)
    plt.gca().set_xscale('log')
    plt.gca().set_yscale('log')
    plt.scatter(udp_err_rates, lib_err_rates)
    plt.ylim(1e-8, 1)
    plt.show()
