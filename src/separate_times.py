#!/usr/bin/env python3

import math

import matplotlib.pyplot as plt
import numpy as np

from utils import *

# Last times. nblocks = 1000, EF in [1,2,8] with edge elision
#last_time_key = "fast_data/time_1509976414_7182746344620220916.pickle.xz"

# Last times. nblocks = 100, EF in [1,2,8]
last_time_key = "fast_data/time_1510066795_4516541816776410923.pickle.xz"

data = load_data(last_time_key)
param_matrix = data['param_matrix']
result_matrix = data['result_matrix']

plt.figure("enc_times")
plt.figure("dec_times")

for j, rs in enumerate(result_matrix):
    ps = param_matrix[j]
    
    overheads = np.array([ p.overhead for p in ps ])
    # Time for one packet
    enc_time = np.array([ r.avg_enc_time for r in rs ])
    # Time for one block
    dec_time = np.array([ r.avg_dec_time for r in rs ])

    k = sum(ps[0].Ks)
    assert(all(sum(p.Ks) == k for p in ps))
    
    enc_time *= k
    enc_time *= (1 + overheads)

    ef = ps[0].EF
    assert(all(p.EF == ef for p in ps))

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
plt.ylabel('Decoding time [ms]')
plt.legend()
plt.grid()

plt.figure("enc_times")
plt.savefig('plot_fast_enc_time.pdf', format='pdf')
plt.figure("dec_times")
plt.savefig('plot_fast_dec_time.pdf', format='pdf')

plt.show()
