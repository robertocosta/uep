import datetime
import lzma
import math
import pickle
import random

import numpy as np

from uep import *

if __name__ == "__main__":
    RFs = [3, 1]
    EF = 4

    c = 0.1
    delta = 0.5

    overhead = 0.25

    avg_per = 0
    avg_bad_run = 1

    nblocks = 10000

    k_blocks = np.logspace(math.log10(10), math.log10(10000), 16, dtype=int)
    Ks_frac = [0.1, 0.9]

    pBG = 1/avg_bad_run
    assert(pBG >= 0 and pBG <= 1)
    pGB = pBG * avg_per / (1 - avg_per)
    assert(pGB >= 0 and pGB <= 1)

    avg_pers = np.zeros((len(k_blocks), len(Ks_frac)))
    avg_drops = np.zeros(len(k_blocks))
    used_Ks = np.zeros((len(k_blocks), len(Ks_frac)), dtype=int)
    for j, k_block in enumerate(k_blocks):
        Ks = [int(round(k_block * kf)) for kf in Ks_frac]
        if sum(Ks) < k_block:
            while k_block - sum(Ks) > 0:
               Ks[np.argmin(Ks)] += 1
        elif sum(Ks) > k_block:
            while sum(Ks) - k_block > 0:
               Ks[np.argmax(Ks)] -= 1
        assert(sum(Ks) == k_block)
        assert(all(ki > 0 for ki in Ks))
        used_Ks[j] = Ks

        sim = UEPSimulation(Ks=Ks, RFs=RFs, EF=EF, c=c, delta=delta,
                            overhead=overhead,
                            markov_pGB=pGB,
                            markov_pBG=pBG,
                            nblocks=nblocks)
        results = run_parallel(sim)
        avg_pers[j] = results['error_rates']
        avg_drops[j] = results['drop_rate']

    newid = random.getrandbits(64)
    save_data("uep_markov/uep_vs_k_markov_{:d}.pickle.xz".format(newid),
              timestamp=datetime.datetime.now().timestamp(),
              k_blocks=k_blocks,
              Ks_frac=Ks_frac,
              used_Ks=used_Ks,
              RFs=RFs,
              EF=EF,
              c=c,
              delta=delta,
              overhead=overhead,
              avg_per=avg_per,
              avg_bad_run=avg_bad_run,
              pBG=pBG,
              pGB=pGB,
              nblocks=nblocks,
              avg_pers=avg_pers,
              avg_drops=avg_drops)
