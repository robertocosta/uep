#!/usr/bin/env python3

import multiprocessing
import sys

import matplotlib.pyplot as plt
import numpy as np

if '../python' not in sys.path:
    sys.path.append('../python')
if './python' not in sys.path:
    sys.path.append('./python')

from uep_fast_run import *

def run_uep_map(params):
    res = simulation_results()
    run_uep(params, res)
    return res

iid_pers = [0, 1e-2, 1e-1, 3e-1]
overheads = np.linspace(0, 0.8, 16).tolist()

base_params = simulation_params()
base_params.Ks[:] = [100, 900]
base_params.RFs[:] = [3, 1]
base_params.EF = 4
base_params.c = 0.1
base_params.delta = 0.5
base_params.L = 4
base_params.nblocks = 10

param_matrix = list()
for per in iid_pers:
    param_matrix.append(list())
    for oh in overheads:
        params = simulation_params(base_params)
        params.chan_pGB = per
        params.chan_pBG = 1 - per
        params.overhead = oh
        param_matrix[-1].append(params)

with multiprocessing.Pool() as pool:
    result_matrix = list()
    for ps in param_matrix:
        r = pool.map(run_uep_map, ps)
        result_matrix.append(r)

plt.figure()
plt.gca().set_yscale('log')

for (j, per) in enumerate(iid_pers):
    mib_pers = [ r.avg_pers[0] for r in result_matrix[j] ]
    lib_pers = [ r.avg_pers[1] for r in result_matrix[j] ]
    plt.plot(overheads, mib_pers,
             marker='o',
             linewidth=1.5,
             label="MIB e = {:.0e}".format(per))
    plt.plot(overheads, lib_pers,
             marker='o',
             linewidth=1.5,
             label="LIB e = {:.0e}".format(per))

plt.ylim(1e-8, 1)
plt.xlabel('Overhead')
plt.ylabel('UEP PER')
plt.legend()
plt.grid()

plt.savefig('plot_fast_iid.pdf', format='pdf')
plt.show()
