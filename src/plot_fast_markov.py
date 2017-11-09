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
from utils import *

fixed_params = [{'avg_good_run': 90,
                 'avg_bad_run': 10,
                 'overhead': 0.25},
                {'avg_good_run': 990,
                 'avg_bad_run': 10,
                 'overhead': 0.25},
                 {'avg_good_run': 9990,
                 'avg_bad_run': 10,
                 'overhead': 0.25}]
#ks = np.linspace(1100, 1300, 3, dtype=int).tolist()
ks = [100] + np.linspace(500, 1500, 3, dtype=int).tolist()
k0_fraction = 0.1

base_params = simulation_params()
base_params.RFs[:] = [3, 1]
base_params.EF = 4
base_params.c = 0.1
base_params.delta = 0.5
base_params.L = 4
#base_params.nblocks = 200
#base_params.nCycles = 15000
error_n = 10000 # avg # of errors

param_matrix = list()
for p in fixed_params:
    param_matrix.append(list())
    for k in ks:
        params = simulation_params(base_params)
        k0 = int(k0_fraction * k)
        params.Ks[:] = [k0, k - k0]
        params.chan_pGB = 1/p['avg_good_run']
        params.chan_pBG = 1/p['avg_bad_run']
		#params.nblocks = 200;
		#nPckts = (p['avg_good_run']+p['avg_bad_run'])*nCycles
        piB = params.chan_pGB / (params.chan_pGB + params.chan_pBG)
        piG = 1 - piB
        nCycles = piG / params.chan_pGB + (error_n-1) / piB
        params.nCycles = int(nCycles)+1
        #params.nblocks = int(((p['avg_good_run']+p['avg_bad_run'])*nCycles)/k)
        params.nblocks = int(nCycles/k)+1
        print('nblocks = '+str(params.nblocks))
        params.overhead = p['overhead']
        param_matrix[-1].append(params)

result_matrix = run_uep_parallel(param_matrix)

datestr = datetime.datetime.now().strftime("%Y-%m-%d_%H-%M-%S")

plt.figure(1)
plt.gca().set_yscale('log')
plt.figure(2)
nblocks_vs_k = [ r.nblocks for r in param_matrix[0] ]
plt.plot(ks, nblocks_vs_k,
         marker='o',
         linewidth=0.5,
         label=("Cycle length = 100"))
#plt.ylim(0, 1)
plt.xlabel('K')
plt.ylabel('nblocks')
plt.legend()
plt.grid()
plt.savefig(datestr+'_nblocks-vs-k.png', format='png')

for (j, p) in enumerate(fixed_params):
    mib_pers = [ r.avg_pers[0] for r in result_matrix[j] ]
    lib_pers = [ r.avg_pers[1] for r in result_matrix[j] ]
    plt.figure(1)
    plt.plot(ks, mib_pers,
             marker='o',
             linewidth=0.5,
             label=("MIB E[#B] = {:.1f},"
                    " E[#G] = {:.1f},"
                    " E[#error] = {:d},"
                    " t = {:.2f}".format(p['avg_bad_run'],
                                         p['avg_good_run'],
                                         error_n,
                                         p['overhead'])))
    plt.plot(ks, lib_pers,
             marker='o',
             linewidth=0.5,
             label=("LIB E[#B] = {:.2f},"
                    " E[#G] = {:.2f},"
                    " E[#error] = {:d},"
                    " t = {:.2f}".format(p['avg_bad_run'],
                                         p['avg_good_run'],
                                         error_n,
                                         p['overhead'])))
    
plt.ylim(1e-7, 1)
plt.xlabel('K')
plt.ylabel('UEP PER')
plt.legend()
plt.grid()

s3 = boto3.resource('s3', region_name='us-east-1')
newid = random.getrandbits(64)
ts = int(datetime.datetime.now().timestamp())
data = lzma.compress(pickle.dumps({'result_matrix': result_matrix,
                                   'param_matrix': param_matrix}))
filename = datestr+'_per-vs-k_oh-'+'{:.2f}'.format(p['overhead'])+'.xz'
out_file = open(filename, 'wb')
out_file.write(data)
# dictionary = pickle.loads(lzma.decompress(open("file.xz","rb").read()))
# res_mat = dictionary['result_matrix']
# res_mat[indice_curva(fixed_params), k]

# par_mat = dictionary['param_matrix']
# par_mat[indice_curva(fixed_params), k]
# par_mat[indice_curva(fixed_params)][:]

#plt.savefig('plot_fast_markov_'+datestr+'.png', format='png')
plt.savefig(filename+'.png', format='png')

#plotkey = "fast_plots/markov_{:d}_{:d}.pdf".format(ts, newid)
#s3.meta.client.upload_file('plot_fast_markov.pdf',
#                           'uep.zanol.eu',
#                           plotkey,
#                           ExtraArgs={'ACL': 'public-read'})
#ploturl = ("https://s3.amazonaws.com/"
#           "uep.zanol.eu/{!s}".format(plotkey))
#print("Uploaded Markov plot at {!s}".format(ploturl))

#plt.show()
