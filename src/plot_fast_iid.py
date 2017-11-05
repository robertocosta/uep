#!/usr/bin/env python3

import concurrent.futures as cf
import datetime
import lzma
import multiprocessing
import pickle
import random
import sys

import boto3
import matplotlib.pyplot as plt
import numpy as np

if '../python' not in sys.path:
    sys.path.append('../python')
if './python' not in sys.path:
    sys.path.append('./python')

from uep_fast_run import *
from utils import *

if __name__ == "__main__":
    iid_pers = [0, 1e-2, 1e-1, 2e-1]
    overheads = np.linspace(0, 0.8, 32).tolist()

    base_params = simulation_params()
    base_params.Ks[:] = [100, 900]
    base_params.RFs[:] = [3, 1]
    base_params.EF = 4
    base_params.c = 0.1
    base_params.delta = 0.5
    base_params.L = 4
    base_params.nblocks = 1000

    param_matrix = list()
    for per in iid_pers:
        param_matrix.append(list())
        for oh in overheads:
            params = simulation_params(base_params)
            params.chan_pGB = per
            params.chan_pBG = 1 - per
            params.overhead = oh
            param_matrix[-1].append(params)

    result_matrix = run_uep_parallel(param_matrix)

    plt.figure(1)
    plt.gca().set_yscale('log')

    plt.figure(2)

    for (j, per) in enumerate(iid_pers):
        mib_pers = [ r.avg_pers[0] for r in result_matrix[j] ]
        mib_errcs = [ r.err_counts[0] for r in result_matrix[j] ]
        lib_pers = [ r.avg_pers[1] for r in result_matrix[j] ]
        lib_errcs = [ r.err_counts[1] for r in result_matrix[j] ]

        plt.figure(1)
        plt.plot(overheads, mib_pers,
                 marker='o',
                 linewidth=1.5,
                 label="MIB e = {:.0e}".format(per))
        plt.plot(overheads, lib_pers,
                 marker='o',
                 linewidth=1.5,
                 label="LIB e = {:.0e}".format(per))

        plt.figure(2)
        plt.plot(overheads, mib_errcs,
                 marker='o',
                 linewidth=1.5,
                 label="MIB e = {:.0e}".format(per))
        plt.plot(overheads, lib_errcs,
                 marker='o',
                 linewidth=1.5,
                 label="LIB e = {:.0e}".format(per))

    plt.figure(1)
    plt.ylim(1e-8, 1)
    plt.xlabel('Overhead')
    plt.ylabel('UEP PER')
    plt.legend()
    plt.grid()

    plt.figure(2)
    plt.xlabel('Overhead')
    plt.ylabel('UEP error count')
    plt.legend()
    plt.grid()

    plt.figure(1)
    plt.savefig('plot_fast_iid.pdf', format='pdf')
    plt.figure(2)
    plt.savefig('plot_fast_iid_errc.pdf', format='pdf')
    plt.ylim(0, 30)
    plt.savefig('plot_fast_iid_errc_small.pdf', format='pdf')

    try:
        newid = random.getrandbits(64)
        ts = int(datetime.datetime.now().timestamp())

        plotkey = "fast_plots/iid_{:d}_{:d}.pdf".format(ts, newid)
        ploturl = save_plot('plot_fast_iid.pdf', plotkey)
        print("Uploaded IID plot at {}".format(ploturl))
        errplot_key = "fast_plots/iid_errc_{:d}_{:d}.pdf".format(ts, newid)
        errplot_url = save_plot('plot_fast_iid_errc.pdf', errplot_key)
        print("Uploaded IID error count plot at {}".format(errplot_url))
        errplotsmall_key = ("fast_plots/"
                            "iid_errc_small_{:d}_{:d}.pdf").format(ts, newid)
        errplotsmall_url = save_plot('plot_fast_iid_errc_small.pdf',
                                     errplotsmall_key)
        print("Uploaded IID error count (small) plot"
              " at {}".format(errplotsmall_url))

        data_key = "fast_data/iid_{:d}_{:d}.pickle.xz".format(ts, newid)
        data_url = save_data(data_key,
                             result_matrix=result_matrix,
                             param_matrix=param_matrix)
        print("Uploaded IID data at {}".format(data_url))

        send_notification(("Uploaded IID\n"
                           "Plot: {!s}\n"
                           "Err count: {!s}\n"
                           "Small err count: {!s}\n"
                           "Data: {!s}\n").format(ploturl,
                                                  errplot_url,
                                                  errplotsmall_url,
                                                  data_url))
    except RuntimeError as e:
        print("Plot upload failed: {!s}".format(e))

    plt.show()
