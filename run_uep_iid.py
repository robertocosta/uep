import datetime
import lzma
import pickle
import random
import subprocess

import numpy as np

from uep import *

if __name__ == "__main__":
    Ks = [100, 1900]
    RFs = [5, 1]
    EF = 1

    c = 0.1
    delta = 0.5

    iid_per = 0

    nblocks = 10000

    sim = UEPSimulation(Ks=Ks, RFs=RFs, EF=EF, c=c, delta=delta,
                        nblocks=nblocks)

    #oh = set(np.linspace(0,0.4,16))
    #oh.update(np.linspace(0,0.1,16))
    #oh.update(np.linspace(0.2,0.3,16))
    #oh = sorted(oh)
    #overheads = np.array(oh[20:])
    overheads = np.linspace(0, 0.4, 16)

    avg_pers = np.zeros((len(overheads), len(Ks)))
    avg_drops = np.zeros(len(overheads))
    avg_ripples = np.zeros(len(overheads))
    error_counts = np.zeros((len(overheads), len(Ks)), dtype=int)
    for j, oh in enumerate(overheads):
        sim.overhead = oh
        results = run_parallel(sim)
        avg_pers[j] = results['error_rates']
        avg_drops[j] = results['drop_rate']
        avg_ripples[j] = results['avg_ripple']
        error_counts[j] = results['error_counts']

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

    newid = random.getrandbits(64)
    save_data("uep_iid_mpfix_fixdeg/uep_vs_oh_iid_{:d}.pickle.xz".format(newid),
              git_sha1=git_sha1,
              timestamp=datetime.datetime.now().timestamp(),
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
