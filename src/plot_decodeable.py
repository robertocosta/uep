import pandas as pd
import numpy as n
import matplotlib.pyplot as plt

data = pd.read_json('test_log.json')
recv = data.ReceivedPackets
numdec = data.DecodeablePackets

plt.figure()
plt.plot(recv, numdec)
plt.show()
