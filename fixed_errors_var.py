import numpy as np

class Markov2:
    def __init__(self, p_GB, p_BG):
        assert(p_GB >= 0 and p_GB <= 1)
        assert(p_BG >= 0 and p_BG <= 1)
        self.__p_GB = p_GB
        self.__p_BG = p_BG
        stat_p_G = self.__p_BG / (self.__p_BG + self.__p_GB)
        if np.random.random() < stat_p_G:
            self.__state = 0
        else:
            self.__state = 1

    def __call__(self):
        if self.__state == 0:
            if np.random.random() < self.__p_GB:
                self.__state = 1
        else:
            if np.random.random() < self.__p_BG:
                self.__state = 0
        return self.__state

def simulate_fixedn(p, N):
    z = 0
    while N > 0:
        N -= 1
        if np.random.random() < p:
            z += 1
    return z

def simulate_fixedn_mk(pGB, pBG, N):
    z = 0
    mk = Markov2(pGB, pBG)
    while N > 0:
        N -= 1
        if mk() == 1:
            z += 1
    return z

def simulate_fixederrs(p, e):
    N = 0
    while e > 0:
        N += 1
        if np.random.random() < p:
            e -= 1
    return N

def simulate_fixederrs_mk(pGB, pBG, e):
    N = 0
    mk = Markov2(pGB, pBG)
    while e > 0:
        N += 1
        if mk() == 1:
            e -= 1
    return N

def fixederrs_theo_var(p, e):
    return (-4*p + 3*e*p + e) / (p*p)

def fixedn_theo_var(p, N):
    return p*(1-p)/N

if __name__ == "__main__":
    trials = 100
    e = 10
    N = 100
    p = e/N

    zs = list()
    p1s = list()
    ns = list()
    p2s = list()
    for i in range(0, trials):
        z = simulate_fixedn_mk(p, 1-p, N)
        zs.append(z)
        p1s.append(z/N)
        n = simulate_fixederrs_mk(p, 1-p, e)
        ns.append(n)
        p2s.append(e/n)

    print("Real p = {:e},"
          " fixed N theo var(p): {:e},"
          " fixed e theo var(N): {:e}".format(p,
                                              fixedn_theo_var(p,N),
                                              fixederrs_theo_var(p, e)))
    print("Fixed N: mean = {:e}, var = {:e}".format(np.mean(p1s), np.var(p1s)))
    print("Fixed e: mean = {:e}, var(p) = {:e}, var(N) = {:e}".format(np.mean(p2s), np.var(p2s), np.var(ns)))
