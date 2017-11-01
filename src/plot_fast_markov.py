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

fixed_params = [{'avg_per': 0,
                'overhead': 0.25,
                 'k': 1000},
                {'avg_per': 1e-1,
                'overhead': 0.25,
                 'k': 1000},
                {'avg_per': 3e-1,
                 'overhead': 0.25,
                 'k': 1000}]
avg_bad_runs = np.linspace(1, 1500, 32).tolist()
k0_fraction = 0.1

base_params = simulation_params()
base_params.RFs[:] = [3, 1]
base_params.EF = 4
base_params.c = 0.1
base_params.delta = 0.5
base_params.L = 4
base_params.nblocks = 50

param_matrix = list()
for p in fixed_params:
    param_matrix.append(list())
    for br in avg_bad_runs:
        params = simulation_params(base_params)
        k0 = int(k0_fraction * p['k'])
        params.Ks[:] = [k0, p['k'] - k0]
        params.chan_pGB = 1/br * p['avg_per'] / (1 - p['avg_per'])
        params.chan_pBG = 1/br
        params.overhead = p['overhead']
        param_matrix[-1].append(params)

with multiprocessing.Pool() as pool:
    result_matrix = list()
    for ps in param_matrix:
        r = pool.map(run_uep_map, ps)
        result_matrix.append(r)

plt.figure()
plt.gca().set_yscale('log')

for (j, p) in enumerate(fixed_params):
    mib_pers = [ r.avg_pers[0] for r in result_matrix[j] ]
    lib_pers = [ r.avg_pers[1] for r in result_matrix[j] ]
    plt.plot(avg_bad_runs, mib_pers,
             marker='o',
             linewidth=1.5,
             label=("MIB K = {:d},"
                    " e = {:.0e},"
                    " t = {:.2f}".format(p['k'],
                                         p['avg_per'],
                                         p['overhead'])))
    plt.plot(avg_bad_runs, lib_pers,
             marker='o',
             linewidth=1.5,
             label=("LIB K = {:d},"
                    " e = {:.0e},"
                    " t = {:.2f}".format(p['k'],
                                         p['avg_per'],
                                         p['overhead'])))

plt.ylim(1e-8, 1)
plt.xlabel('Overhead')
plt.ylabel('UEP PER')
plt.legend()
plt.grid()

plt.savefig('plot_fast_iid.pdf', format='pdf')
plt.show()
