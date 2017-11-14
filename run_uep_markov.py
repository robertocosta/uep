import argparse
import datetime
import lzma
import math
import pickle
import random

import numpy as np

from uep import *

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Runs a simulation.')
    parser.add_argument("rf", help="MIB Repeating factor",type=int)
    parser.add_argument("ef", help="Expanding factor",type=int)
    parser.add_argument("nblocks", help="nblocks for the simulation",type=int)
    parser.add_argument("EnG", help="Avg. number of contiguous Good slots",type=int)
    parser.add_argument("EnB", help="Avg. number of contiguous Bad slots",type=int)
    args = parser.parse_args()
    
    RFs = [args.rf, 1]
    EF = args.ef

    c = 0.1
    delta = 0.5

    overhead = 0.25
    assert(args.EnG > 0)
    avg_good_run = args.EnG
    assert(args.EnB > 0)
    avg_bad_run = args.EnB

    nblocks = args.nblocks

    #k_blocks = np.linspace(200,4200, 21, dtype=int)
    #k_blocks = np.linspace(5200,6200, 2, dtype=int)
    k_blocks = np.linspace(200,4200, 21, dtype=int)

    
    Ks_frac = [0.05, 0.95]
    pBG = 1/avg_bad_run
    pGB = 1/avg_good_run
    avg_per = pGB / (pGB + pBG)
    assert(pBG >= 0 and pBG <= 1)
    #pGB = pBG * avg_per / (1 - avg_per)
    assert(pGB >= 0 and pGB <= 1)
    
    avg_pers = np.zeros((len(k_blocks), len(Ks_frac)))
    avg_drops = np.zeros(len(k_blocks))
    used_Ks = np.zeros((len(k_blocks), len(Ks_frac)), dtype=int)
    for j, k_block in enumerate(k_blocks):
        Ks = [int(round(k_block * kf)) for kf in Ks_frac]
        print(Ks)
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
    save_data("uep_17_11_14_3/uep_vs_k_markov_{:d}.pickle.xz".format(newid),
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
