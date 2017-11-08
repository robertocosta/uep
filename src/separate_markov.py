#!/usr/bin/env python3

import matplotlib.pyplot as plt
import numpy as np

from utils import *

# Last using fixed nblocks
#last_markov_key = "fast_data/markov_1509935225_8012946579946993038.pickle.xz"

# Last using fixed n.errs
last_markov_key = "fast_data/markov_1509867828_16491124733610827659.pickle.xz"

data = load_data(last_markov_key)
param_matrix = data['param_matrix']
result_matrix = data['result_matrix']

plt.figure()

for (j, rs) in enumerate(result_matrix):
    ks = [ sum(p.Ks) for p in param_matrix[j] ]

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
    oh = ps[0].overhead
    assert(all(p.overhead == oh for p in ps))
    chan_pBG = ps[0].chan_pBG
    chan_pGB = ps[0].chan_pGB
    assert(all(p.chan_pBG == chan_pBG for p in ps))
    assert(all(p.chan_pGB == chan_pGB for p in ps))
    avg_bad_run = 1/chan_pBG
    avg_per = chan_pGB / (chan_pGB + chan_pBG)

    for i, p in enumerate(mib_pers):
        if p < 1e-8:
            mib_pers[i] = 1e-8

    for i, p in enumerate(lib_pers):
        if p < 1e-8:
            lib_pers[i] = 1e-8

    plt.plot(ks, mib_pers,
             marker='.',
             linestyle='-',
             linewidth=1.5,
             label="MIB E[#B] = {:.2f}".format(avg_bad_run))
    plt.plot(ks, lib_pers,
             marker='.',
             linestyle='--',
             linewidth=1.5,
             label="LIB E[#B] = {:.2f}".format(avg_bad_run))

plt.gca().set_yscale('log')
plt.ylim(1e-8, 1)
plt.xlabel('K')
plt.ylabel('UEP PER')
plt.legend()
plt.grid()

plt.savefig("plot_markov.pdf", format='pdf')

plt.show()
