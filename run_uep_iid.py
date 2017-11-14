import argparse
import datetime
import lzma
import pickle
import random
import subprocess

import numpy as np

from uep import *

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Runs an IID UEP simulation.',
                                     allow_abbrev=False)
    parser.add_argument("rf", help="MIB Repeating factor",type=int)
    parser.add_argument("ef", help="Expanding factor",type=int)
    parser.add_argument("nblocks", help="nblocks for the simulation",type=int)
    parser.add_argument("--iid_per", help="Channel packet drop rate",type=float, default=0)
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
    RFs = [args.rf,1]
    EF = args.ef

    c = 0.1
    delta = 0.5

    iid_per = args.iid_per

    nblocks = args.nblocks

    sim = UEPSimulation(Ks=Ks, RFs=RFs, EF=EF, c=c, delta=delta,
                        nblocks=nblocks, iid_per=iid_per)

    #oh = set(np.linspace(0,0.4,16))
    #oh.update(np.linspace(0,0.1,16))
    #oh.update(np.linspace(0.2,0.3,16))
    #oh = sorted(oh)
    #overheads = np.array(oh[20:])
    overheads = np.linspace(0, 0.4, 16).tolist()
    #overheads += np.linspace(0.4, 0.8, 17)[1:].tolist()

    used_rngstate = random.getstate()

    avg_pers = np.zeros((len(overheads), len(Ks)))
    avg_drops = np.zeros(len(overheads))
    avg_ripples = np.zeros(len(overheads))
    error_counts = np.zeros((len(overheads), len(Ks)), dtype=int)
    for j, oh in enumerate(overheads):
        print("Run with oh = {:.3f}".format(oh))
        sim.overhead = oh
        results = run_parallel(sim)
        avg_pers[j] = results['error_rates']
        avg_drops[j] = results['drop_rate']
        avg_ripples[j] = results['avg_ripple']
        error_counts[j] = results['error_counts']
        print("  PERs = {!s}".format(avg_pers[j]))
        print("  errors = {!s}".format(error_counts[j]))

    newid = random.getrandbits(64)
    save_data("uep_iid_other_deg_fix/uep_vs_oh_iid_{:d}.pickle.xz".format(newid),
              git_sha1=git_sha1,
              timestamp=datetime.datetime.now().timestamp(),
              used_rngstate=used_rngstate,
              overheads=overheads,
              Ks=Ks,
              RFs=RFs,
              EF=EF,
              c=c,
              delta=delta,
              iid_per=iid_per,
              nblocks=nblocks,
              avg_pers=avg_pers,
              avg_drops=avg_drops,
              avg_ripples=avg_ripples,
              error_counts=error_counts)