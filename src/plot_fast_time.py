#!/usr/bin/env python3

import lzma
import pickle
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

last_iid_data_key = "fast_data/iid_1509738311_495220755081646518.pickle.xz"

data = load_data(last_iid_data_key)
param_matrix = data['param_matrix']
result_matrix = data['result_matrix']

plt.figure("enc_time")

for (j, rs) in enumerate(result_matrix[0:1]):
    overheads = np.array([p.overhead for p in param_matrix[j]])
    enc_times = np.array([r.avg_enc_time for r in rs])
    iid_per = param_matrix[j][0].chan_pGB

    plt.plot(overheads, enc_times * 1000,
             label="e = {:.0e}".format(iid_per))

plt.xlabel('Overhead')
plt.ylabel('Encoding time [ms]')
plt.grid()
    
plt.savefig('plot_fast_enc_time.pdf', format='pdf')

plt.show()
