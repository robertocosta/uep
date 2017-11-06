#!/usr/bin/env python3

import concurrent.futures as cf
import datetime
import lzma
import math
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
    fixed_params = [
        {'avg_per': 1e-3,
         'avg_bad_run': 10,
         'overhead': 0.25},
        {'avg_per': 1e-3,
         'avg_bad_run': 100,
         'overhead': 0.25},
        {'avg_per': 1e-3,
         'avg_bad_run': 500,
         'overhead': 0.25},
    ]
    ks = np.linspace(10, 6000, 32, dtype=int).tolist()
    k0_fraction = 0.1

    base_params = simulation_params()
    base_params.RFs[:] = [3, 1]
    base_params.EF = 4
    base_params.c = 0.1
    base_params.delta = 0.5
    base_params.L = 4
    base_params.nblocks_min = 500
    base_params.wanted_errs = 30
    base_params.nblocks_max = int(1e5)

    param_matrix = list()
    for p in fixed_params:
        param_matrix.append(list())
        for k in ks:
            params = simulation_params(base_params)
            k0 = int(k0_fraction * k)
            params.Ks[:] = [k0, k - k0]
            params.chan_pGB = 1/p['avg_bad_run'] * p['avg_per'] / (1 - p['avg_per'])
            params.chan_pBG = 1/p['avg_bad_run']
            params.overhead = p['overhead']
            param_matrix[-1].append(params)

    result_matrix = run_uep_parallel(param_matrix)

    plt.figure(1)
    plt.gca().set_yscale('log')

    plt.figure(2)

    for (j, p) in enumerate(fixed_params):
        mib_pers = [ r.avg_pers[0] for r in result_matrix[j] ]
        mib_errcs = [ r.err_counts[0] for r in result_matrix[j] ]
        lib_pers = [ r.avg_pers[1] for r in result_matrix[j] ]
        lib_errcs = [ r.err_counts[1] for r in result_matrix[j] ]

        plt.figure(1)
        plt.plot(ks, mib_pers,
                 marker='o',
                 linewidth=1.5,
                 label=("MIB E[#B] = {:.2f},"
                        " e = {:.0e},"
                        " t = {:.2f}".format(p['avg_bad_run'],
                                             p['avg_per'],
                                             p['overhead'])))
        plt.plot(ks, lib_pers,
                 marker='o',
                 linewidth=1.5,
                 label=("LIB E[#B] = {:.2f},"
                        " e = {:.0e},"
                        " t = {:.2f}".format(p['avg_bad_run'],
                                             p['avg_per'],
                                             p['overhead'])))

        plt.figure(2)
        plt.plot(ks, mib_errcs,
                 marker='o',
                 linewidth=1.5,
                 label=("MIB E[#B] = {:.2f},"
                        " e = {:.0e},"
                        " t = {:.2f}".format(p['avg_bad_run'],
                                             p['avg_per'],
                                             p['overhead'])))
        plt.plot(ks, lib_errcs,
                 marker='o',
                 linewidth=1.5,
                 label=("LIB E[#B] = {:.2f},"
                        " e = {:.0e},"
                        " t = {:.2f}".format(p['avg_bad_run'],
                                             p['avg_per'],
                                             p['overhead'])))

    plt.figure(1)
    plt.ylim(1e-8, 1)
    plt.xlabel('K')
    plt.ylabel('UEP PER')
    plt.legend()
    plt.grid()

    plt.figure(2)
    plt.xlabel('K')
    plt.ylabel('UEP error count')
    plt.legend()
    plt.grid()

    plt.figure(1)
    plt.savefig('plot_fast_markov.pdf', format='pdf')
    plt.figure(2)
    plt.savefig('plot_fast_markov_errc.pdf', format='pdf')
    plt.ylim(0, base_params.wanted_errs * 1.25)
    plt.savefig('plot_fast_markov_errc_small.pdf', format='pdf')

    try:
        newid = random.getrandbits(64)
        ts = int(datetime.datetime.now().timestamp())

        plotkey = "fast_plots/markov_{:d}_{:d}.pdf".format(ts, newid)
        ploturl = save_plot('plot_fast_markov.pdf', plotkey)
        print("Uploaded Markov plot at {}".format(ploturl))
        errplot_key = "fast_plots/markov_errc_{:d}_{:d}.pdf".format(ts, newid)
        errplot_url = save_plot('plot_fast_markov_errc.pdf', errplot_key)
        print("Uploaded Markov error count plot at {}".format(errplot_url))
        errplotsmall_key = ("fast_plots/"
                            "markov_errc_small_{:d}_{:d}.pdf").format(ts, newid)
        errplotsmall_url = save_plot('plot_fast_markov_errc_small.pdf',
                                     errplotsmall_key)
        print("Uploaded Markov error count (small) plot"
              " at {}".format(errplotsmall_url))

        data_key = "fast_data/markov_{:d}_{:d}.pickle.xz".format(ts, newid)
        data_url = save_data(data_key,
                             result_matrix=result_matrix,
                             param_matrix=param_matrix)
        print("Uploaded Markov data at {}".format(data_url))

        send_notification(("Uploaded Markov\n"
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
