import argparse
import datetime
import lzma
import pickle
import random
import subprocess
import time

import numpy as np

from uep import *
from utils.aws import *
from utils.stats import *

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Runs an UEP simulation and times it.',
                                     allow_abbrev=False)
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

    Ks = [100, 1900]
    RFs = [args.rf, 1]
    EF = args.ef

    c = 0.1
    delta = 0.5

    nblocks = args.nblocks

    sim = UEPSimulation(Ks=Ks, RFs=RFs, EF=EF, c=c, delta=delta,
                        nblocks=nblocks)

    overheads = np.linspace(0, 0.8, 33)

    used_rngstate = random.getstate()

    avg_enc_times = np.zeros(len(overheads))
    avg_dec_times = np.zeros(len(overheads))
    for j, oh in enumerate(overheads):
        print("Run with oh = {:.3f}".format(oh))
        sim.overhead = oh
        results = sim.run() # Single process to measure the correct time
        avg_enc_times[j] = results['avg_enc_time']
        avg_dec_times[j] = results['avg_dec_time']
        print("  Avg. enc: {:f}, avg. dec: {:f}".format(avg_enc_times[j],
                                                        avg_dec_times[j]))

    newid = random.getrandbits(64)
    save_data("uep_time/uep_vs_oh_time_{:d}.pickle.xz".format(newid),
              git_sha1=git_sha1,
              timestamp=datetime.datetime.now().timestamp(),
              used_rngstate=used_rngstate,
              overheads=overheads,
              Ks=Ks,
              RFs=RFs,
              EF=EF,
              c=c,
              delta=delta,
              nblocks=nblocks,
              avg_enc_times=avg_enc_times,
              avg_dec_times=avg_dec_times)
