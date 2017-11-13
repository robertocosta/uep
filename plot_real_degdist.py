import math

import matplotlib.pyplot as plt
import numpy as np

from uep_random import *

Ks = [100, 900]
RFs = [3, 1]
EF = 4
c = 0.1
delta = 0.5

Kdeg = EF * sum(k*rf for k, rf in zip(Ks, RFs))

plt.figure()

ds = range(1, sum(Ks)+1)
# Leave out the last one
pmd = [robust_pmd(Kdeg, c, delta, d) for d in ds[:-1]]
pmd.append(sum(robust_pmd(Kdeg, c, delta, d)
               for d in range(sum(Ks)+1, Kdeg+1)))
assert(math.isclose(sum(pmd), 1, rel_tol=1e-3))

print("pmd(sum(K)) = {:f}".format(pmd[-1]))

plt.semilogy(ds, pmd, marker='.', linestyle=' ')
#         basefmt=' ', markerfmt='C0.', linefmt='C0-')
#plt.gca().set_yscale('log')
plt.xlabel('Degree')
plt.ylabel('Degree probability')
plt.grid()

plt.savefig('real_degdist.pdf', format='pdf')
plt.show()
