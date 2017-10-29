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

    udp_err_rates = [0, 1e-4, 1e-2, 1e-1] # Set err rates
    overheads = np.linspace(0, 0.2, 10)
    overheads.resize(15)
    overheads[10:] = np.linspace(0.2, 0.4, 6)[1:]

    udp_real_err_rates = [] # Measured UDP err rates
    uep_err_rates = [] # Measured UEP err rates
    uep_err_counts = [] # Cumulative err count for each sub-block
    uep_tot_pkts = []

    for (j, p) in enumerate(udp_err_rates):
        dec_p = Decimal(p).quantize(Decimal('1e-8'))
        udp_real_err_rates.append([])
        uep_err_rates.append([])
        uep_err_counts.append([])
        uep_tot_pkts.append([])

        for (i, oh) in enumerate(overheads):
            dec_oh = Decimal(oh).quantize(Decimal('1e-4'))

            q = sim_tab.query(IndexName='iid_per-overhead-index',
                              KeyConditionExpression=Key('iid_per').eq(dec_p) &
                              Key('overhead').eq(dec_oh),
                              FilterExpression=Attr('k0').eq(100) &
                              Attr('k1').eq(900) &
                              Attr('rf0').eq(3) &
                              Attr('rf1').eq(1) &
                              Attr('ef').eq(4))
            if q['Count'] < 1:
                raise RuntimeError("Point at oh={:f} was not computed".format(oh))

            p_udp_err_rates = []
            p_uep_err_rates = []
            p_uep_err_counts = None
            p_uep_tot_pkts = None
            for p in q['Items']:
                bs_o = uep_bucket.Object(p['s3_ber_scanner']).get()
                bs = pickle.loads(lzma.decompress(bs_o['Body'].read()))

                p_udp_err_rates.append(bs.udp_err_rate)
                p_uep_err_rates.append(bs.uep_err_rates)

                errs = list(map(operator.sub,
                                bs.tot_sent,
                                bs.tot_recvd))
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
                                        1.96*np.std(udp_real_err_rates[-1])))

    mib_err_rates = np.array(uep_err_rates)[:, :, 0]
    lib_err_rates = np.array(uep_err_rates)[:, :, 1]

    mib_err_rates_ci = []
    lib_err_rates_ci = []
    for j in range(0, len(udp_err_rates)):
        mib_err_rates_ci.append([])
        lib_err_rates_ci.append([])
        for i in range(0, len(overheads)):
            mib_err_rates_ci[-1].append(success_err(uep_err_counts[j][i][0],
                                                    uep_tot_pkts[j][i][0]))
            lib_err_rates_ci[-1].append(success_err(uep_err_counts[j][i][1],
                                                    uep_tot_pkts[j][i][1]))
    mib_err_rates_ci = np.array(mib_err_rates_ci)
    lib_err_rates_ci = np.array(lib_err_rates_ci)

    plt.figure()
    plt.gca().set_yscale('log')

    for i in range(0, len(mib_err_rates)):
        plt.errorbar(overheads,
                     mib_err_rates[i],
                     yerr=mib_err_rates_ci[i].transpose(),
                     #fmt='-o',
                     linewidth=1.5,
                     elinewidth=1,
                     capthick=1,
                     capsize=3,
                     label="MIB e = {:.0e}".format(udp_err_rates[i]))
        plt.errorbar(overheads,
                     lib_err_rates[i],
                     yerr=lib_err_rates_ci[i].transpose(),
                     #fmt='-o',
                     linewidth=1.5,
                     elinewidth=1,
                     capthick=1,
                     capsize=3,
                     label="LIB e = {:.0e}".format(udp_err_rates[i]))

    plt.ylim(1e-8, 1)
    plt.legend()
    plt.grid()

    plt.savefig("plot_ber_iid.pdf", format="pdf")
    plt.show()
