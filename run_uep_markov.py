import argparse
import datetime
import lzma
import math
import pickle
import random
import subprocess

import numpy as np

from uep import *

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Runs a Markov UEP simulation.')
    parser.add_argument("rf", help="MIB Repeating factor",type=int)
    parser.add_argument("ef", help="Expanding factor",type=int)
    parser.add_argument("nblocks", help="nblocks for the simulation",type=int)
    args = parser.parse_args()

    git_sha1 = None
    try:
        git_proc = subprocess.run(["git", "log",
                                   "-1",
                                   "--format=%H"],
                                  check=False,
                                  stdout=subprocess.PIPE)
        if git_proc.returncode == 0:
            git_sha1 = git_proc.stdout.strip().decode()
    except FileNotFoundError:
        pass

    if git_sha1 is None:
        try:
            git_sha1 = open('git_commit_sha1', 'r').read().strip()
        except FileNotFound:
            pass

    RFs = [args.rf,1]
    EF = args.ef

    c = 0.1
    delta = 0.5

    overhead = 0.25

    avg_per = 0
    avg_bad_run = 1

    nblocks = args.nblocks

    k_blocks = np.logspace(math.log10(100), math.log10(10000), 16, dtype=int)
    Ks_frac = [0.1, 0.9]

    pBG = 1/avg_bad_run
    assert(pBG >= 0 and pBG <= 1)
    pGB = pBG * avg_per / (1 - avg_per)
    assert(pGB >= 0 and pGB <= 1)

    avg_pers = np.zeros((len(k_blocks), len(Ks_frac)))
    avg_drops = np.zeros(len(k_blocks))
    used_Ks = np.zeros((len(k_blocks), len(Ks_frac)), dtype=int)
    error_counts = np.zeros((len(k_blocks), len(Ks_frac)), dtype=int)
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
        error_counts[j] = results['error_counts']

    newid = random.getrandbits(64)
    save_data("uep_markov_mpfix_fixdeg/uep_vs_k_markov_{:d}.pickle.xz".format(newid),
              git_sha1=git_sha1,
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
              avg_drops=avg_drops,
              error_counts=error_counts)
