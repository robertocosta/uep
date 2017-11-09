import datetime
import lzma
import pickle
import random

import numpy as np

from uep import *

if __name__ == "__main__":
    Ks = [100, 900]
    RFs = [3,1]
    EF = 4
    c = 0.1
    delta = 0.5

    nblocks = 10000

    sim = UEPSimulation(Ks=Ks, RFs=RFs, EF=EF, c=c, delta=delta,
                        nblocks=nblocks)

    overheads = np.linspace(0, 0.4, 16)

    avg_pers = np.zeros((len(overheads), 2))
    for j, oh in enumerate(overheads):
        sim.overhead = oh
        results = run_parallel(sim)
        avg_pers[j,:] = results['error_rates']

    newid = random.getrandbits(64)
    save_data("uep_vs_oh_iid_{:d}.pickle.xz".format(newid),
              timestamp=datetime.datetime.now().timestamp(),
              overheads=overheads,
              avg_pers=avg_pers)
