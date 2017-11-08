import concurrent.futures as cf
import copy
import math
import os
import random

from mppy import *
from uep_random import *

class UEPSimulation:
    def __init__(self, **kwargs):
        self.nblocks = kwargs.get('nblocks', 1)
        self.overhead = kwargs.get('overhead', 0)

        self.__rowgen = RowGenerator(Ks=kwargs['Ks'],
                                     RFs=kwargs.get('RFs',
                                                    [1 for k in kwargs['Ks']]),
                                     EF=kwargs.get('EF', 1),
                                     c=kwargs['c'],
                                     delta=kwargs['delta'])

    @property
    def Ks(self):
        return self.__rowgen.Ks

    @property
    def RFs(self):
        return self.__rowgen.RFs

    @property
    def EF(self):
        return self.__rowgen.EF

    @property
    def K(self):
        return self.__rowgen.K

    @property
    def c(self):
        return self.__rowgen.c

    @property
    def delta(self):
        return self.__rowgen.delta

    def run(self):
        results = dict()
        results['error_counts'] = [0 for k in self.Ks]

        n = math.ceil(self.K * (1 + self.overhead))
        mpctx = mp_context(self.__rowgen.K)
        for nb in range(self.nblocks):
            mpctx.reset()

            for l in range(n):
                mpctx.add_output(self.__rowgen())

            mpctx.run()
            dec = mpctx.input_symbols()

            offset = 0
            for i, k in enumerate(self.Ks):
                err = sum(1 for p in dec[offset:offset+k] if not p)
                results['error_counts'][i] += err
                offset += k

        z_ek = zip(results['error_counts'], self.Ks)
        results['error_rates'] = [e / (self.nblocks * k)
                                  for e,k in z_ek]
        return results

def run_parallel(sim):
    ncpu = len(os.sched_getaffinity(0))
    split_nb = [math.floor(sim.nblocks / ncpu) for i in range(ncpu)]
    for i in range(sim.nblocks % ncpu):
        split_nb[i] += 1

    result_futures = list()
    print("Starting parallel UEPSimulations"
          " on {:d} CPUs".format(ncpu))
    print("Split nblocks = {:d} -> {!s}".format(sim.nblocks, split_nb))
    with cf.ProcessPoolExecutor(ncpu) as executor:
        sim_copy = copy.deepcopy(sim)
        for nb in split_nb:
            sim_copy.nblocks = nb
            f = executor.submit(sim_copy.run)
            result_futures.append(f)
    print("Done")

    results = dict()
    results['error_counts'] = [0 for k in sim.Ks]
    results['error_rates'] = [0 for k in sim.Ks]
    for i, fs in enumerate(result_futures):
        r = f.result()
        for j, k in enumerate(sim.Ks):
            results['error_counts'][j] += r['error_counts'][j]
            results['error_rates'][j] += (r['error_rates'][j] *
                                          split_nb[j] / sim.nblocks)
    return results

if __name__ == "__main__":
    import matplotlib.pyplot as plt
    import numpy as np

    Ks = [100, 900]
    RFs = [3,1]
    EF = 4
    c = 0.1
    delta = 0.5

    nblocks = 100

    sim = UEPSimulation(Ks=Ks, RFs=RFs, EF=EF, c=c, delta=delta,
                        nblocks=nblocks)

    overheads = np.linspace(0, 0.2, 16)

    avg_pers = np.zeros((len(overheads), 2))
    for j, oh in enumerate(overheads):
        sim.overhead = oh
        results = run_parallel(sim)
        avg_pers[j,:] = results['error_rates']

    plt.figure()
    plt.plot(overheads, avg_pers[:,0])
    plt.plot(overheads, avg_pers[:,1])
    plt.gca().set_yscale('log')
    plt.show()
