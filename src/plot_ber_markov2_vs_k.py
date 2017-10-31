#!/usr/bin/env python3

import datetime
import decimal
import lzma
import numbers
import operator
import os
import pickle
import random
import shelve
import subprocess
import sys

import boto3
import matplotlib.pyplot as plt
import numpy as np
import botocore

from boto3.dynamodb.conditions import Attr, Key
from decimal import Decimal
from subprocess import check_call

from ber import *

if __name__ == "__main__":
    config = botocore.client.Config(read_timeout=300)
    s3 = boto3.resource('s3', config=config, region_name='us-east-1')
    uep_bucket = s3.Bucket('uep.zanol.eu')
    dyn = boto3.resource('dynamodb', region_name='us-east-1')
    sim_tab = dyn.Table('uep_sim_results')

    fixed_err_p = 1e-1
    ks = [1000]#, 5000, 10000]
    bad_runs = np.linspace(1, 1500, 10)

    udp_real_err_rates = [] # Measured UDP err rates
    uep_err_rates = [] # Measured UEP err rates
    uep_err_counts = [] # Cumulative err count for each sub-block
    uep_tot_pkts = []

    for (j, k) in enumerate(ks):
        udp_real_err_rates.append([])
        uep_err_rates.append([])
        uep_err_counts.append([])
        uep_tot_pkts.append([])

        for (i, br) in enumerate(bad_runs):
            dec_br = Decimal(br).quantize(Decimal('1e-4'))

            q = sim_tab.query(IndexName='k-avg_bad_run-index',
                              KeyConditionExpression=Key('k').eq(k) &
                              Key('avg_bad_run').eq(dec_br),
                              FilterExpression=Attr('masked').ne(True) &
                              Attr('k0').eq(int(0.1 * k)) &
                              Attr('k1').eq(k - int(0.1 * k)) &
                              Attr('rf0').eq(3) &
                              Attr('rf1').eq(1) &
                              Attr('ef').eq(4))
            if q['Count'] < 1:
                raise RuntimeError("Point at br={:f} was not computed".format(br))

            p_udp_err_rates = []
            p_uep_err_rates = []
            p_uep_err_counts = None
            p_uep_tot_pkts = None
            for pt in q['Items']:
                bs_o = uep_bucket.Object(pt['s3_ber_scanner']).get()
                bs = pickle.loads(lzma.decompress(bs_o['Body'].read()))

                print("[{!s}] k = {:d},"
                      " bad_run = {:f}"
                      " results:".format(pt['result_id'], k, dec_br))

                p_udp_err_rates.append(bs.udp_err_rate)
                print("UDP err rate = {:e}".format(bs.udp_err_rate))
                p_uep_err_rates.append(bs.uep_err_rates)
                print("UEP err rates = {!s}".format(bs.uep_err_rates))

                errs = list(map(operator.sub,
                                bs.tot_sent,
                                bs.tot_recvd))
                print("UEP err counts = {!s}".format(errs))
                print("UEP pkt counts = {!s}".format(bs.tot_sent))
                if p_uep_err_counts is None:
                    p_uep_err_counts = errs
                    p_uep_tot_pkts = bs.tot_sent
                else:
                    p_uep_err_counts[:] = map(operator.add,
                                              p_uep_err_counts,
                                              errs)
                    p_uep_tot_pkts[:] = map(operator.add,
                                            p_uep_tot_pkts,
                                            bs.tot_sent)

            udp_real_err_rates[-1].append(np.mean(p_udp_err_rates))
            uep_err_rates[-1].append(np.mean(np.array(p_uep_err_rates), 0))
            uep_err_counts[-1].append(p_uep_err_counts)
            uep_tot_pkts[-1].append(p_uep_tot_pkts)

        print("Wanted UDP err.rate = {:e}"
              " measured = avg. {:e}"
              " +- {:e} (ci95%)".format(0,
                                        np.mean(udp_real_err_rates[-1]),
                                        1.96/len(udp_real_err_rates[-1])*
                                        np.std(udp_real_err_rates[-1])))

    mib_err_rates = np.array(uep_err_rates)[:, :, 0]
    lib_err_rates = np.array(uep_err_rates)[:, :, 1]

    mib_err_rates_ci = []
    lib_err_rates_ci = []
    for j in range(0, len(ks)):
        mib_err_rates_ci.append([])
        lib_err_rates_ci.append([])
        for i in range(0, len(bad_runs)):
            mib_err_rates_ci[-1].append(success_err(uep_err_counts[j][i][0],
                                                    uep_tot_pkts[j][i][0]))
            lib_err_rates_ci[-1].append(success_err(uep_err_counts[j][i][1],
                                                    uep_tot_pkts[j][i][1]))
    mib_err_rates_ci = np.array(mib_err_rates_ci)
    lib_err_rates_ci = np.array(lib_err_rates_ci)

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
        plt.errorbar(bad_runs,
                     mib_err_rates[i],
                     yerr=mib_err_rates_ci[i].transpose(),
                     #fmt='-o',
                     linewidth=1.5,
                     elinewidth=1,
                     capthick=1,
                     capsize=3,
                     label="MIB K = {:d}".format(ks[i]))
        plt.errorbar(bad_runs,
                     lib_err_rates[i],
                     yerr=lib_err_rates_ci[i].transpose(),
                     #fmt='-o',
                     linewidth=1.5,
                     elinewidth=1,
                     capthick=1,
                     capsize=3,
                     label="LIB K = {:d}".format(ks[i]))

    plt.ylim(1e-8, 1)
    plt.legend()
    plt.grid()

    plt.savefig("plot_ber_markov2_vs_k.pdf", format="pdf")
    plt.show()
