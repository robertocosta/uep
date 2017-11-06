import math

import matplotlib.pyplot as plt
import numpy as np

from scipy.sparse import dok_matrix

class fixed_errors_mc:
    def __init__(self, event_prob, max_errs):
        assert(event_prob >= 0 and event_prob <= 1)
        assert(max_errs > 0)
        self.__p = event_prob
        self.__e = max_errs
        self.__steps_cache = [list() for i in range(0, max_errs)]

        # Fill the diagonal
        for j in range(1, self.__e+1):
            self.__cache_set(j, j, self.__p ** j)

    def __cache_get(self, j, k):
        # Map j -> j-1 (unused row 0)
        # Map k -> k-1 - (j-1) (unused k < j)
        return self.__steps_cache[j-1][k-1 - (j-1)]

    def __cache_set(self, j, k, v):
        # Map j -> j-1 (unused row 0)
        # Map k -> k-1 - (j-1) (unused k < j)
        row = self.__steps_cache[j-1]
        if len(row) != (k-1 - (j-1)):
            raise RuntimeError("Should set once, in order")
        else:
            row.append(v)

    def __cache_missing(self, j):
        # Map j -> j-1 (unused row 0)
        # Reverse map k -> k-1 - (j-1) (unused k < j)
        return len(self.__steps_cache[j-1]) + (j-1) + 1
        
    def steps_probability(self, k, start=0):
        assert(start < self.__e and start >= 0)
        assert(k > 0)
        j = self.__e - start

        # Complete row j=0
        m_k = self.__cache_missing(1)
        while m_k <= k-j+1:
            p = (1 - self.__p) ** (k - 1) * self.__p
            self.__cache_set(1, m_k, p)
            m_k = self.__cache_missing(1)

        # Complete other rows
        for l in range(2, j+1):
            m_k = self.__cache_missing(l)
            while m_k <= k-j+l:
                p = (self.__cache_get(l, m_k-1) * (1 - self.__p) +
                     self.__cache_get(l-1, m_k-1) * self.__p)
                self.__cache_set(l, m_k, p)
                m_k = self.__cache_missing(l)

        return self.__cache_get(j,k)

if __name__ == "__main__":
    mc_test = fixed_errors_mc(0.1, 3)
    assert(math.isclose(mc_test.steps_probability(1, 2), 0.1))
    assert(math.isclose(mc_test.steps_probability(2, 1), 0.1**2))
    assert(math.isclose(mc_test.steps_probability(3, 0), 0.1**3))
    
    assert(math.isclose(mc_test.steps_probability(2, 2), 0.1*0.9))
    assert(math.isclose(mc_test.steps_probability(3, 2), 0.1*(0.9**2)))
    assert(math.isclose(mc_test.steps_probability(4, 2), 0.1*(0.9**3)))
    assert(math.isclose(mc_test.steps_probability(10, 2), 0.1*(0.9**9)))

    assert(math.isclose(mc_test.steps_probability(3, 1), 0.9*(0.1**2) * 2))
    assert(math.isclose(mc_test.steps_probability(4, 1), 0.9*(0.9*(0.1**2) * 2) + 0.1 * (0.9**2)*0.1))
    assert(math.isclose(mc_test.steps_probability(4, 0), 0.9*(0.1**3) + 0.1*(0.9*(0.1**2) * 2)))

    e = 30
    ps = np.logspace(-2, 0, 256)
    ks = [int(e/p) for p in ps]

    steps_probs = list()
    for p, k in zip(ps, ks):
        mc = fixed_errors_mc(p, e)
        sp = mc.steps_probability(k)
        steps_probs.append(sp)

    plt.figure()
    plt.plot(ps, steps_probs)
    plt.gca().set_xscale('log')
    plt.gca().set_yscale('log')
    plt.grid()
    
    plt.show()
    

    
    
        
    
