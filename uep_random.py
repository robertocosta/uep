import bisect
import math
import random
import itertools

def soliton_pmd(K, deg):
    if deg == 1:
        return 1 / K
    elif deg >= 2 and deg <= K:
        return 1 / (deg * (deg - 1))
    else:
        return 0

def robust_S(K, c, delta):
    return c * math.log(K / delta) * math.sqrt(K)

def robust_tau(K_S, S_delta, deg):
    if deg >= 1 and deg <= K_S - 1:
        return 1 / (K_S * deg)
    elif deg == K_S:
        return math.log(S_delta) / K_S
    else:
        return 0

def robust_beta(K, K_S, S_delta):
    return sum(soliton_pmd(K, d) + robust_tau(K_S, S_delta, d)
               for d in range(1, K+1))

def robust_pmd(K, c, delta, deg):
    if deg < 1 or deg > K:
        return 0
    else:
        S = robust_S(K, c, delta)
        K_S = round(K / S)
        S_delta = S / delta
        beta = robust_beta(K, K_S, S_delta)
        return (soliton_pmd(K, deg) +
                robust_tau(K_S, S_delta, deg)) / beta

class DegreeGenerator:
    def __init__(self, K, c, delta):
        self.__K = K
        self.__c = c
        self.__delta = delta

        cdf_acc = lambda s, i: s + robust_pmd(self.__K,self.__c,self.__delta, i)
        cdf = itertools.accumulate(range(0, self.__K+1), func=cdf_acc)
        self.__cdf_cache = list(cdf)

    @property
    def K(self):
        return self.__K

    @property
    def c(self):
        return self.__c

    @property
    def delta(self):
        return self.__delta

    def __call__(self):
        u = random.random()
        deg = bisect.bisect_right(self.__cdf_cache, u)
        if deg != len(self.__cdf_cache):
            return deg
        else:
            raise RuntimeError("Not generated all the CDF")

class RowGenerator:
    def __init__(self, **kwargs):
        self.__Ks = tuple(kwargs['Ks'])
        self.__RFs = tuple(kwargs['RFs'])
        assert(len(self.__Ks) == len(self.__RFs))
        self.__EF = kwargs['EF']
        self.__c = kwargs['c']
        self.__delta = kwargs['delta']

        self.__K = sum(self.__Ks)
        self.__Kdeg = (self.__EF *
                       sum(k * rf for k, rf in
                           zip(self.__Ks, self.__RFs)))

        self.__deg_gen = DegreeGenerator(self.__Kdeg, self.__c, self.__delta)

        self.__build_pos_map()

    def __build_pos_map(self):
        self.__pos_map = list()
        offset = 0
        for k, rf in zip(self.__Ks, self.__RFs):
            ext = [offset + i for i in range(0, k)]
            for r in range(0, rf):
                self.__pos_map.extend(ext)
            offset += k;
        self.__pos_map *= self.__EF

    @property
    def Ks(self):
        return self.__Ks

    @property
    def RFs(self):
        return self.__RFs

    @property
    def EF(self):
        return self.__EF

    @property
    def K(self):
        return self.__K

    @property
    def c(self):
        return self.__c

    @property
    def delta(self):
        return self.__delta

    def __call__(self):
        deg = self.__deg_gen()
        row = random.sample(range(self.__Kdeg), deg)
        row_mapped = [self.__pos_map[i] for i in row]
        return row_mapped

if __name__ == "__main__":
    import matplotlib.pyplot as plt
    import numpy as np
    import time
    import sys


    degtimes = list()
    dg = DegreeGenerator(4800, 0.1, 0.5)
    for i in range(0, 10000):
        t = time.process_time()
        row = dg()
        tdiff = time.process_time() - t
        degtimes.append(tdiff)
        #print("Time diff {:f}".format(tdiff))
    
    print("done. Avg tdiff = {:f}".format(np.mean(degtimes)))

    rowtimes = list()
    rg = RowGenerator(Ks=[100,900], RFs=[3,1], EF=4, c=0.1, delta=0.5)
    for i in range(0, 10000):
        t = time.process_time()
        row = rg()
        tdiff = time.process_time() - t
        rowtimes.append(tdiff)
        print("Time diff {:f}".format(tdiff))
    
    print("done. Avg tdiff = {:f}".format(np.mean(rowtimes)))
    
    Ks = [1000, 9000]
    RFs = [3,1]
    EF = 4
    c = 0.2
    delta = 0.05

    row_gen = RowGenerator(Ks=[sum(Ks)], RFs=[1], EF=1, c=c, delta=delta)
    row_gen_uep = RowGenerator(Ks=Ks, RFs=RFs, EF=EF, c=c, delta=delta)

    rows = [row_gen() for i in range(0, 100)]
    rows_uep = [row_gen_uep() for i in range(0, 100)]

    selected = [i for r in rows for i in r]
    selected_uep = [i for r in rows_uep for i in r]
    histo = np.bincount(selected) / len(selected)
    histo_uep = np.bincount(selected_uep) / len(selected_uep)

    selection_prob = sum (1 for i in selected if i < Ks[0]) / len(selected)
    selection_prob_mib = sum (1 for i in selected_uep if i < Ks[0]) / len(selected_uep)

    print("Probability of selecting a (not) MIB packet is {:f}".format(selection_prob))
    print("Probability of selecting a (not) LIB packet is {:f}".format(1 - selection_prob))

    print("Probability of selecting a MIB packet is {:f}".format(selection_prob_mib))
    print("Probability of selecting a LIB packet is {:f}".format(1 - selection_prob_mib))

    
    plt.figure()
    plt.stem(histo)
    plt.figure()
    plt.stem(histo_uep)

    # plt.figure()
    # dg = DegreeGenerator(10000, 0.2, 0.05)
    # degs = [dg() for i in range(0, 10000)]
    # degs_histo = np.bincount(degs) / len(degs)
    # plt.stem(degs_histo)
    # plt.xlim(0, 50)

    plt.show()
