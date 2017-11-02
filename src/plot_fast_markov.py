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

class UEPRunner:
    def __init__(self, print_lock):
        self.print_lock = print_lock

    def __call__(self, params):
        res = run_uep(params)
        with self.print_lock:
            print(".", end="")
        return res

if __name__ == "__main__":
    fixed_params = [
        {'avg_per': 1e-3,
         'avg_bad_run': 10,
         'overhead': 0.25},
        {'avg_per': 1e-3,
         'avg_bad_run': 100,
         'overhead': 0.25},
        # {'avg_per': 1e-3,
        #  'avg_bad_run': 1000,
        #  'overhead': 0.25},
    ]
    ks = np.logspace(math.log10(10), math.log10(10000), 64, dtype=int).tolist()
    k0_fraction = 0.1

    base_params = simulation_params()
    base_params.RFs[:] = [3, 1]
    base_params.EF = 4
    base_params.c = 0.1
    base_params.delta = 0.5
    base_params.L = 4
    base_params.nblocks_min = 500
    base_params.wanted_errs = 30
    base_params.nblocks_max = int(1e6)

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

    print_lock = multiprocessing.Lock()
    result_futures = list()
    with cf.ProcessPoolExecutor() as executor:
        print_mngr = multiprocessing.Manager()
        print_lock = print_mngr.Lock()
        with print_lock:
            print("Running", end=" ")
        for ps in param_matrix:
            result_futures.append(list())
            for p in ps:
                f = executor.submit(UEPRunner(print_lock), p)
                result_futures[-1].append(f)
    print()
    result_matrix = list()
    for fs in result_futures:
        result_matrix.append([f.result() for f in fs])

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
        s3 = boto3.resource('s3', region_name='us-east-1')
        sns = boto3.resource('sns', region_name='us-east-1')

        newid = random.getrandbits(64)
        ts = int(datetime.datetime.now().timestamp())

        plotkey = "fast_plots/markov_{:d}_{:d}.pdf".format(ts, newid)
        errplot_key = "fast_plots/markov_errc_{:d}_{:d}.pdf".format(ts, newid)
        errplotsmall_key = ("fast_plots/"
                            "markov_errc_small_{:d}_{:d}.pdf").format(ts, newid)
        s3.meta.client.upload_file('plot_fast_markov.pdf',
                                   'uep.zanol.eu',
                                   plotkey,
                                   ExtraArgs={'ACL': 'public-read'})

        ploturl = ("https://s3.amazonaws.com/"
                   "uep.zanol.eu/{!s}".format(plotkey))
        print("Uploaded Markov plot at {!s}".format(ploturl))

        s3.meta.client.upload_file('plot_fast_markov_errc.pdf',
                                   'uep.zanol.eu',
                                   errplot_key,
                                   ExtraArgs={'ACL': 'public-read'})
        errplot_url = ("https://s3.amazonaws.com/"
                   "uep.zanol.eu/{!s}".format(errplot_key))
        print("Uploaded Markov error count plot at {!s}".format(errplot_url))

        s3.meta.client.upload_file('plot_fast_markov_errc_small.pdf',
                                   'uep.zanol.eu',
                                   errplotsmall_key,
                                   ExtraArgs={'ACL': 'public-read'})
        errplotsmall_url = ("https://s3.amazonaws.com/"
                   "uep.zanol.eu/{!s}".format(errplotsmall_key))
        print("Uploaded Markov error count (small) plot"
              " at {!s}".format(errplotsmall_url))

        data = lzma.compress(pickle.dumps({'result_matrix': result_matrix,
                                           'param_matrix': param_matrix}))
        data_key = "fast_data/markov_{:d}_{:d}.pickle.xz".format(ts, newid)
        s3.meta.client.put_object(Body=data,
                                  Bucket='uep.zanol.eu',
                                  Key=data_key,
                                  ACL='public-read')
        data_url = ("https://s3.amazonaws.com/"
                   "uep.zanol.eu/{!s}".format(data_key))
        print("Uploaded Markov data at {!s}".format(data_url))

        sns.meta.client.publish(TopicArn='arn:aws:sns:us-east-1:402432167722:NotifyMe',
                                Message=("Uploaded Markov\n"
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
