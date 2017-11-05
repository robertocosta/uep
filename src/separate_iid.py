#!/usr/bin/env python3

import math

import matplotlib.pyplot as plt
import numpy as np

from utils import *

last_iid_key = "fast_data/iid_1509876086_7510077353202185511.pickle.xz"

data = load_data(last_iid_key)
param_matrix = data['param_matrix']
result_matrix = data['result_matrix']

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

    plt.figure()
    plt.errorbar(ohs, mib_pers, yerr=np.array(mib_yerrs).transpose(),
                 fmt='.-',
                 elinewidth=1,
                 capthick=1,
                 capsize=3,
                 linewidth=1.5,
                 label="MIB e = {:.0e}".format(iid_per))
    plt.errorbar(ohs, lib_pers, yerr=np.array(lib_yerrs).transpose(),
                 fmt='.--',
                 elinewidth=1,
                 capthick=1,
                 capsize=3,
                 linewidth=1.5,
                 label="LIB e = {:.0e}".format(iid_per))

    plt.gca().set_yscale('log')
    plt.ylim(1e-8, 1)
    plt.xlabel('Overhead')
    plt.ylabel('UEP PER')
    plt.legend()
    plt.grid()

    plt.savefig("plot_iid_{:d}.pdf".format(j), format='pdf')

    actual_per = [r.dropped_count / (r.actual_nblocks * sum(ps[0].Ks))
                  for r in rs]
    plt.figure()
    plt.plot(ohs, actual_per)
    plt.gca().set_yscale('log')
    plt.ylim(iid_per / 10, iid_per * 10)
    plt.grid()

plt.show()
