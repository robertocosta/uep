#!/usr/bin/env python3

import os
import shelve
import sys

import boto3
import matplotlib.pyplot as plt
import numpy as np
import botocore

from subprocess import check_call

from ber import *

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: {!s} [-f] <zero-BER log dir>"
              " <iid BER log dir>".format(sys.argv[0]))
        sys.exit(2)
    if len(sys.argv) > 3:
        use_cached = not sys.argv[1] == '-f'
    else:
        use_cached = True
    zerodir = sys.argv[-2]
    iiddir = sys.argv[-1]

    zeroshelf = shelve.open(zerodir)
    iidshelf = shelve.open(iiddir)

    config = botocore.client.Config(read_timeout=300)
    s3 = boto3.client('s3', config=config, region_name='us-east-1')

    udp_err_rates = [0, 1e-4, 1e-2, 1e-1] # Set err rates
    overheads = np.linspace(0, 0.4, 20)

    uep_err_rates = [] # Measured UEP err rates
    uep_err_rates_ci = []
    udp_real_err_rates = [] # Measured UDP err rates

    # UDP err = 0
    uep_err_rates.append([])
    uep_err_rates_ci.append([])
    udp_real_err_rates.append([])
    for (i, oh) in enumerate(overheads):
        if not use_cached:
            srvname = "server_{:02d}.log".format(i)
            srvkey = "simulation_logs/{!s}/{!s}.tar.xz".format(zerodir, srvname)
            serverlog = s3.download_file(Bucket='uep.zanol.eu',
                                         Key=srvkey,
                                         Filename=srvname + ".tar.xz")
            check_call(["tar", "-xJf", srvname + ".tar.xz"])
            os.remove(srvname + ".tar.xz")

            clientname = "client_{:02d}.log".format(i)
            clientkey = "simulation_logs/{!s}/{!s}.tar.xz".format(zerodir, clientname)
            clientlog = s3.download_file(Bucket='uep.zanol.eu',
                                         Key=clientkey,
                                         Filename=clientname + ".tar.xz")
            check_call(["tar", "-xJf", clientname + ".tar.xz"])
            os.remove(clientname + ".tar.xz")

            bs = ber_scanner(srvname, clientname)
            zeroshelf["bs_{:02d}".format(i)] = bs
            bs.scan()
        else:
            bs = zeroshelf["bs_{:02d}".format(i)]

        udp_real_err_rates[-1].append(bs.udp_err_rate)
        uep_err_rates[-1].append(bs.uep_err_rates)
        uep_err_rates_ci[-1].append(bs.uep_err_rates_ci)

        print("{:02d} OK".format(i))
        if not use_cached:
            os.remove(srvname)
            os.remove(clientname)

    print("Wanted UDP err.rate = {:e}"
          " measured = avg. {:e}"
          " +- {:e} (ci95%)".format(0,
                                    np.mean(udp_real_err_rates[0]),
                                    1.96*np.std(udp_real_err_rates[0])))

    for (j, p) in enumerate(udp_err_rates[1:]):
        uep_err_rates.append([])
        uep_err_rates_ci.append([])
        udp_real_err_rates.append([])
        for (i, oh) in enumerate(overheads):
            if not use_cached:
                srvname = "server_{:02d}_{:02d}.log".format(j,i)
                srvkey = "simulation_logs/{!s}/{!s}.tar.xz".format(iiddir, srvname)
                serverlog = s3.download_file(Bucket='uep.zanol.eu',
                                             Key=srvkey,
                                             Filename=srvname + ".tar.xz")
                check_call(["tar", "-xJf", srvname + ".tar.xz"])
                os.remove(srvname + ".tar.xz")

                clientname = "client_{:02d}_{:02d}.log".format(j,i)
                clientkey = "simulation_logs/{!s}/{!s}.tar.xz".format(iiddir, clientname)
                clientlog = s3.download_file(Bucket='uep.zanol.eu',
                                             Key=clientkey,
                                             Filename=clientname + ".tar.xz")
                check_call(["tar", "-xJf", clientname + ".tar.xz"])
                os.remove(clientname + ".tar.xz")

                bs = ber_scanner(srvname, clientname)
                zeroshelf["bs_{:02d}_{:02d}".format(j,i)] = bs
                bs.scan()
            else:
                bs = zeroshelf["bs_{:02d}_{:02d}".format(j,i)]

            udp_real_err_rates[-1].append(bs.udp_err_rate)
            uep_err_rates_ci[-1].append(bs.uep_err_rates)
            uep_err_rates[-1].append(bs.uep_err_rates)

            print("{:02d}_{:02d} OK".format(j,i))
            if not use_cached:
                os.remove(srvname)
                os.remove(clientname)

        print("Wanted UDP err.rate = {:e}"
              " measured = avg. {:e}"
              " +- {:e} (ci95%)".format(p,
                                        np.mean(udp_real_err_rates[-1]),
                                        1.96*np.std(udp_real_err_rates[-1])))

    mib_err_rates = np.array(uep_err_rates)[:, :, 0]
    mib_err_rates_ci = np.array(uep_err_rates_ci)[:, :, 0]
    lib_err_rates = np.array(uep_err_rates)[:, :, 1]
    lib_err_rates_ci = np.array(uep_err_rates_ci)[:, :, 1]

    mib_err_rates_ci -= mib_err_rates
    mib_err_rates_ci[:,:,0] = -mib_err_rates_ci[:,:,0]
    lib_err_rates_ci -= lib_err_rates
    lib_err_rates_ci[:,:,0] = -lib_err_rates_ci[:,:,0]

    plt.figure()
    plt.gca().set_yscale('log')

    for i in range(0, len(mib_err_rates)):
        plt.errorbar(overheads,
                     mib_err_rates[i],
                     yerr=mib_err_rates_ci[i].transpose(),
                     label="MIB e={:e}".format(udp_err_rates[i]))
        plt.errorbar(overheads,
                     lib_err_rates[i],
                     yerr=lib_err_rates_ci[i].transpose(),
                     label="LIB e={:e}".format(udp_err_rates[i]))

    plt.ylim(1e-8, 1)
    plt.legend()

    plt.savefig("plot_ber_iid.pdf", format="pdf")
    zeroshelf.close()
    iidshelf.close()
    plt.show()
