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
        print("Usage: {!s} <log dir>".format(sys.argv[0]))
        sys.exit(2)
    s3dir = sys.argv[1]

    config = botocore.client.Config(read_timeout=300)
    s3 = boto3.client('s3', config=config, region_name='us-east-1')

    fixed_err_p = 1e-4
    ks = [1000, 5000, 10000]
    bad_runs = np.linspace(500, 12500, 20)

    uep_err_rates = [] # Measured UEP err rates
    uep_err_rates_ci = []
    udp_real_err_rates = [] # Measured UDP err rates

    for (j, k) in enumerate(ks):
        uep_err_rates.append([])
        uep_err_rates_ci.append([])
        udp_real_err_rates.append([])
        for (i, br) in enumerate(bad_runs):
            srvname = "server_{:02d}_{:02d}.log".format(j,i)
            srvkey = "simulation_logs/{!s}/{!s}.tar.xz".format(s3dir, srvname)
            serverlog = s3.download_file(Bucket='uep.zanol.eu',
                                         Key=srvkey,
                                         Filename=srvname + ".tar.xz")
            check_call(["tar", "-xJf", srvname + ".tar.xz"])
            os.remove(srvname + ".tar.xz")

            clientname = "client_{:02d}_{:02d}.log".format(j,i)
            clientkey = "simulation_logs/{!s}/{!s}.tar.xz".format(s3dir, clientname)
            clientlog = s3.download_file(Bucket='uep.zanol.eu',
                                         Key=clientkey,
                                         Filename=clientname + ".tar.xz")
            check_call(["tar", "-xJf", clientname + ".tar.xz"])
            os.remove(clientname + ".tar.xz")

            bs = ber_scanner(srvname, clientname)
            bs.scan()

            udp_real_err_rates[-1].append(bs.udp_err_rate)
            uep_err_rates[-1].append(bs.uep_err_rates)
            uep_err_rates_ci[-1].append(bs.uep_err_rates_ci)

            print("{:02d}_{:02d} OK".format(j,i))
            os.remove(srvname)
            os.remove(clientname)

        print("Wanted UDP err.rate = {:e}"
              " measured = avg. {:e}"
              " +- {:e} (ci95%)".format(fixed_err_p,
                                        np.mean(udp_real_err_rates[-1]),
                                        1.96*np.std(udp_real_err_rates[-1])))
    split_mib = lambda x: x[0];
    split_lib = lambda x: x[1];
    split_nested_mib = lambda l: list(map(split_mib, l))
    split_nested_lib = lambda l: list(map(split_lib, l))

    mib_err_rates = list(map(split_nested_mib, uep_err_rates))
    lib_err_rates = list(map(split_nested_lib, uep_err_rates))
    mib_err_rates_ci = list(map(split_nested_mib, uep_err_rates_ci))
    lib_err_rates_ci = list(map(split_nested_lib, uep_err_rates_ci))

    plt.figure()
    plt.gca().set_yscale('log')

    for i in range(0, len(mib_err_rates)):
        mib_yerr = np.array(mib_err_rates_ci[i]).transpose()
        mib_yerr[0,:] += mib_err_rates[i]
        mib_yerr[1,:] -= mib_err_rates[i]
        lib_yerr = np.array(lib_err_rates_ci[i]).transpose()
        lib_yerr[0,:] += lib_err_rates[i]
        lib_yerr[1,:] -= lib_err_rates[i]

        plt.errorbar(bad_runs,
                     mib_err_rates[i],
                     yerr=mib_yerr,
                     label="MIB {:d}".format(ks[i]))
        plt.errorbar(bad_runs,
                     lib_err_rates[i],
                     yerr=lib_yerr,
                     label="LIB {:d}".format(ks[i]))

    plt.ylim(1e-8, 1)
    plt.legend()

    plt.savefig("plot_ber_markov2_vs_k.pdf", format="pdf")
    plt.show()
