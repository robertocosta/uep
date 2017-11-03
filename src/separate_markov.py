#!/usr/bin/env python3

import matplotlib.pyplot as plt

from utils import *

data = load_data("fast_data/markov_1509720620_17751789468932352005.pickle.xz")
param_matrix = data['param_matrix']
result_matrix = data['result_matrix']

plt.figure()

assert(param_matrix[0][0].chan_pBG == 1/10)
mib10per = [ r.avg_pers[0] for r in result_matrix[0]]
lib10per = [ r.avg_pers[1] for r in result_matrix[0]]
ks = [ sum(p.Ks) for p in param_matrix[0]]

plt.plot(ks, mib10per, marker='o')
plt.plot(ks, lib10per, marker='o')


plt.gca().set_yscale('log')

plt.show()
