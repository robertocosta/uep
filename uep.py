import math
import matplotlib.pyplot as plt
import numpy as np
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
        self.__mpctx = mp_context(self.__rowgen.K)

        self.__results = dict()

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
        for nb in range(self.nblocks):
            self.__mpctx.reset()

            for l in range(n):
                self.__mpctx.add_output(self.__rowgen())

            self.__mpctx.run()
            dec = self.__mpctx.input_symbols()

            offset = 0
            for i, k in enumerate(self.Ks):
                err = sum(1 for p in dec[offset:offset+k] if not p)
                results['error_counts'][i] += err
                offset += k

        z_ek = zip(results['error_counts'], self.Ks)
        results['error_rates'] = [e / (self.nblocks * k)
                                  for e,k in z_ek]
        return results

if __name__ == "__main__":
    Ks = [100, 900]
    RFs = [3,1]
    EF = 4
    c = 0.1
    delta = 0.5

    nblocks = 50

    sim = UEPSimulation(Ks=Ks, RFs=RFs, EF=EF, c=c, delta=delta,
                        nblocks=nblocks)

    overheads = np.linspace(0.2, 0.25, 16)

    avg_pers = np.zeros((len(overheads), 2))
    for j, oh in enumerate(overheads):
        sim.overhead = oh
        results = sim.run()
        avg_pers[j,:] = results['error_rates']

    plt.figure()
    plt.plot(overheads, avg_pers[:,0])
    plt.plot(overheads, avg_pers[:,1])
    plt.gca().set_yscale('log')
    plt.show()
