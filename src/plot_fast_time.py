#!/usr/bin/env python3

import lzma
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
    efs = [1,2,8]
    overheads = np.linspace(0, 0.2, 32).tolist()
    k = 20000
    nblocks = 100

    base_params = simulation_params()
    base_params.Ks[:] = [k]
    base_params.RFs[:] = [1]
    base_params.c = 0.1
    base_params.delta = 0.5
    base_params.L = 4
    base_params.nblocks = nblocks
    base_params.chan_pGB = 0
    base_params.chan_pBG = 1

    param_matrix = list()
    for ef in efs:
        param_matrix.append(list())
        for oh in overheads:
            params = simulation_params(base_params)
            params.overhead = oh
            params.EF = ef
            param_matrix[-1].append(params)

    result_matrix = run_uep_parallel(param_matrix)

    plt.figure("enc_times")
    plt.figure("dec_times")

    for (j, ef) in enumerate(efs):
        enc_time = np.array([ r.avg_enc_time for r in result_matrix[j] ])
        dec_time = np.array([ r.avg_dec_time for r in result_matrix[j] ])

        plt.figure("enc_times")
        plt.plot(overheads, enc_time * 1000,
                 marker='o',
                 linewidth=1.5,
                 label="EF = {:d}".format(ef))
        plt.figure("dec_times")
        plt.plot(overheads, dec_time * 1000,
                 marker='o',
                 linewidth=1.5,
                 label="EF = {:d}".format(ef))

    plt.figure("enc_times")
    plt.xlabel('Overhead')
    plt.ylabel('Encoding time [ms]')
    plt.legend()
    plt.grid()

    plt.figure("dec_times")
    plt.xlabel('Overhead')
    plt.ylabel('Encoding time [ms]')
    plt.legend()
    plt.grid()

    plt.figure("enc_times")
    plt.savefig('plot_fast_enc_time.pdf', format='pdf')
    plt.figure("dec_times")
    plt.savefig('plot_fast_dec_time.pdf', format='pdf')

    try:
        newid = random.getrandbits(64)
        ts = int(datetime.datetime.now().timestamp())

        encplot_key = "fast_plots/enc_time_{:d}_{:d}.pdf".format(ts, newid)
        encplot_url = save_plot('plot_fast_enc_time.pdf', encplot_key)
        print("Uploaded encoding time plot at {}".format(encplot_url))
        decplot_key = "fast_plots/dec_time_{:d}_{:d}.pdf".format(ts, newid)
        decplot_url = save_plot('plot_fast_dec_time.pdf', decplot_key)
        print("Uploaded decoding time plot at {}".format(decplot_url))

        data_key = "fast_data/time_{:d}_{:d}.pickle.xz".format(ts, newid)
        data_url = save_data(data_key,
                             result_matrix=result_matrix,
                             param_matrix=param_matrix)
        print("Uploaded time data at {}".format(data_url))

        send_notification(("Uploaded time\n"
                           "Enc. plot: {!s}\n"
                           "Dec. plot: {!s}\n"
                           "Data: {!s}\n").format(encplot_url,
                                                  decplot_url,
                                                  data_url))
    except RuntimeError as e:
        print("Plot upload failed: {!s}".format(e))

    plt.show()
