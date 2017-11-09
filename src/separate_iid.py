#!/usr/bin/env python3

import math

import matplotlib.pyplot as plt
import numpy as np

from utils import *

#last_iid_key = "fast_data/iid_1509910549_137428857076329484.pickle.xz"

# Last done with variable nblocks 500 -> 10^5, e=30
last_iid_key = "fast_data/iid_1509876086_7510077353202185511.pickle.xz"

data = load_data(last_iid_key)
param_matrix = data['param_matrix']
result_matrix = data['result_matrix']

plt.figure()

for (j, rs) in enumerate(result_matrix):
    ohs = [ p.overhead for p in param_matrix[j] ]

    mib_pers = [ r.avg_pers[0] for r in rs ]
    mib_errcs = [ r.err_counts[0] for r in rs ]
    mib_tots = [r.actual_nblocks * p.Ks[0]
                for r, p in zip(rs, param_matrix[j])]
    mib_yerrs = [success_err(z,n)
                 for z,n in zip(mib_errcs, mib_tots)]

    lib_pers = [ r.avg_pers[1] for r in rs ]
    lib_errcs = [ r.err_counts[1] for r in rs ]
    lib_tots = [r.actual_nblocks * p.Ks[1]
                for r, p in zip(rs, param_matrix[j])]
    lib_yerrs = [success_err(z,n)
                 for z,n in zip(lib_errcs, lib_tots)]

    ps = param_matrix[j]
    chan_pBG = ps[0].chan_pBG
    chan_pGB = ps[0].chan_pGB
    assert(all(p.chan_pBG == chan_pBG for p in ps))
    assert(all(p.chan_pGB == chan_pGB for p in ps))
    assert(math.isclose(chan_pGB, 1-chan_pBG))
    iid_per = chan_pGB

    # for i in range(0, len(mib_pers)):
    #     if mib_pers[i] < 2e-3:
    #         mib_pers[i:] = [1e-8] * (len(mib_pers) - i)

    # for i in range(0, len(lib_pers)):
    #     if lib_pers[i] < 1e-3:
    #         lib_pers[i:] = [1e-8] * (len(lib_pers) - i)

    plt.plot(ohs, mib_pers,
             marker='.',
             linestyle='-',
             linewidth=1.5,
             label="MIB e = {:.0e}".format(iid_per))
    plt.plot(ohs, lib_pers,
             marker='.',
             linestyle='--',
             linewidth=1.5,
             label="LIB e = {:.0e}".format(iid_per))

    # actual_per = [r.dropped_count / (r.actual_nblocks * sum(ps[0].Ks))
    #               for r in rs]
    # plt.figure()
    # plt.plot(ohs, actual_per)
    # plt.gca().set_yscale('log')
    # plt.ylim(iid_per / 10, iid_per * 10)
    # plt.grid()

plt.gca().set_yscale('log')
plt.ylim(1e-8, 1)
plt.xlabel('Overhead')
plt.ylabel('UEP PER')
plt.legend()
plt.grid()

plt.savefig("plot_iid.pdf", format='pdf')

plt.show()
