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
from operator import itemgetter
from subprocess import check_call

from ber import *
from utils.aws import *
from utils.logs import *
from utils.stats import *

if __name__ == "__main__":
    config = botocore.client.Config(read_timeout=300)
    s3 = boto3.resource('s3', config=config, region_name='us-east-1')
    uep_bucket = s3.Bucket('uep.zanol.eu')
    dyn = boto3.resource('dynamodb', region_name='us-east-1')
    sim_tab = dyn.Table('uep_sim_results')

    wanted = [{'k': 1000,
               'avg_per': Decimal('1e-1'),
               'overhead': Decimal('0.3')},
              {'k': 1000,
               'avg_per': Decimal('1e-2'),
               'overhead':Decimal('0.15')},
              {'k': 1000,
               'avg_per': Decimal('1e-2'),
               'overhead': Decimal('0.2')}]

    points = [ [] for w in wanted ]
    for (j, w) in enumerate(wanted):
        wk = w['k']
        wp = w['avg_per']
        wo = w['overhead']
        q = sim_tab.query(IndexName='k-avg_bad_run-index',
                          KeyConditionExpression=Key('k').eq(wk),
                          FilterExpression=Attr('k0').eq(int(0.1 * wk)) &
                          Attr('k1').eq(wk - int(0.1 * wk)) &
                          Attr('avg_per').eq(wp) &
                          Attr('overhead').eq(wo) &
                          Attr('rf0').eq(3) &
                          Attr('rf1').eq(1) &
                          Attr('ef').eq(4))# &
                          #Attr('stream_name').eq('stefan_cif_long'))# &
                          #Attr('masked').ne(True))
        for p in q['Items']:
            points[j].append(p)

    points = [ sorted(ps, key=itemgetter('avg_bad_run')) for ps in points ]

    bad_runs = [ [] for p in points ]
    udp_real_err_rates = [ [] for p in points ]
    uep_err_rates = [ [] for p in points ]
    uep_err_counts = [ [] for p in points ]
    uep_tot_pkts = [ [] for p in points ]

    for (j, w) in enumerate(wanted):
        for p in points[j]:
            bs_o = uep_bucket.Object(p['s3_ber_scanner']).get()
            bs = pickle.loads(lzma.decompress(bs_o['Body'].read()))
            print("[{!s}] k = {!s}, bad_run = {!s}"
                  " results:".format(p['result_id'],
                                     p['k'],
                                     p['avg_bad_run']))

            errs = list(map(operator.sub,
                            bs.tot_sent,
                            bs.tot_recvd))

            print("UDP err rate = {:e}".format(bs.udp_err_rate))
            print("UEP err rates = {!s}".format(bs.uep_err_rates))
            print("UEP err counts = {!s}".format(errs))
            print("UEP pkt counts = {!s}".format(bs.tot_sent))

            if p['avg_bad_run'] not in bad_runs[j]: # First point at this overhead
                bad_runs[j].append(p['avg_bad_run'])
                udp_real_err_rates[j].append(bs.udp_err_rate)
                uep_err_rates[j].append(bs.uep_err_rates)
                uep_err_counts[j].append(errs)
                uep_tot_pkts[j].append(bs.tot_sent)
            else:
                i = bad_runs[j].index(p['avg_bad_run'])
                sumw = sum(uep_tot_pkts[j][i])
                w = sum(bs.tot_sent)
                udp_real_err_rates[j][i] = update_average(udp_real_err_rates[j][i],
                                                          sumw,
                                                          bs.udp_err_rate,
                                                          w)
                uep_err_rates[j][i][:] = map(lambda m, s: update_average(m, sumw, s, w),
                                             uep_err_rates[j][i],
                                             bs.uep_err_rates)
                uep_err_counts[j][i][:] = map(operator.add,
                                              uep_err_counts[j][i],
                                              errs)
                uep_tot_pkts[j][i][:] = map(operator.add,
                                            uep_tot_pkts[j][i],
                                            bs.tot_sent)

    plt.figure()
    plt.gca().set_yscale('log')

    for (j, w) in enumerate(wanted):
        s = sorted(zip(bad_runs[j],
                       uep_err_rates[j],
                       uep_err_counts[j],
                       uep_tot_pkts[j]),
                   key=operator.itemgetter(0))
        s = np.array(s)
        brs = s[:,0]
        mib_per = np.array(list(s[:,1]))[:,0]
        lib_per = np.array(list(s[:,1]))[:,1]
        mib_errc = np.array(list(s[:,2]))[:,0]
        lib_errc = np.array(list(s[:,2]))[:,1]
        mib_npkts = np.array(list(s[:,3]))[:,0]
        lib_npkts = np.array(list(s[:,3]))[:,1]

        mib_per_ci = []
        for z,n in zip(mib_errc, mib_npkts):
            mib_per_ci.append(success_err(z, n))
        mib_per_ci = np.array(mib_per_ci)

        lib_per_ci = []
        for z,n in zip(lib_errc, lib_npkts):
            lib_per_ci.append(success_err(z, n))
        lib_per_ci = np.array(lib_per_ci)

        plt.plot(brs,
                 mib_per,
                 marker='o',
                 linewidth=1.5,
                 label="MIB K = {!s}".format(w['k']))
        plt.plot(brs,
                 lib_per,
                 marker='o',
                 linewidth=1.5,
                 label="LIB K = {!s}".format(w['k']))

    plt.ylim(1e-3, 1)
    plt.xlabel('E[#_B]')
    plt.ylabel('UEP PER')
    plt.legend()
    plt.grid()

    plt.savefig("plot_ber_markov2_vs_k.pdf", format="pdf")
    plt.show()
