import numpy as np
import matplotlib.pyplot as plt
import pandas as pd

ideal_pmd = pd.read_csv('ideal_pmd.csv').values
plt.figure()
plt.plot(ideal_pmd[:,0], ideal_pmd[:,1])
plt.xlim([0,50])

# robust_pmd = pd.read_csv('robust_pmd.csv').values
# plt.figure()
# plt.bar(robust_pmd[:,0], robust_pmd[:,1])
# plt.xlim([0,50])

ideal = pd.read_csv('ideal_soliton_samples.csv')
robust = pd.read_csv('robust_soliton_samples.csv')
print("Ideal:")
print(ideal.describe())
print("Robust:")
print(robust.describe())

ideal = ideal.values[:,0]
robust = robust.values[:,0]

hi = np.bincount(ideal) / np.sum(ideal)
plt.figure()

plt.plot(range(0,len(hi)), hi)
plt.xlim(0,50)

plt.show()
