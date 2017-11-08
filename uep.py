import math
import matplotlib.pyplot as plt
import numpy as np
import random

from mppy import *
from uep_random import *

if __name__ == "__main__":
    Ks = [100, 900]
    RFs = [3,1]
    EF = 4
    c = 0.1
    delta = 0.5

    nblocks = 100

    overheads = np.linspace(0.2, 0.25, 16)
    
    row_gen = RowGenerator(Ks=Ks, RFs=RFs, EF=EF, c=c, delta=delta)
    mp = mp_context(row_gen.K)

    pers = np.zeros((nblocks, len(overheads), 2))    
    for i in range(0, nblocks):
        for j, oh in enumerate(overheads):
            mp.reset()
            n = math.ceil(row_gen.K * (1 + oh))
            for l in range(0, n):
                row = row_gen() 
                mp.add_output(row)
            mp.run()
            dec = mp.input_symbols()
            per_mib = sum(1 for p in dec[0:Ks[0]] if not p) / Ks[0]
            per_lib = sum(1 for p in dec[Ks[0]:] if not p) / Ks[1]
            pers[i,j,:] = (per_mib, per_lib)

    avg_pers = np.mean(pers, 0)

    plt.figure()
    plt.plot(overheads, avg_pers[:,0])
    plt.plot(overheads, avg_pers[:,1])
    plt.gca().set_yscale('log')
    plt.show()
