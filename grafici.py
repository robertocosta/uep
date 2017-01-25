print("hello world!")
#import numpy as np
import matplotlib.pyplot as plt
import pandas as pd
df = pd.read_csv('datas.csv', sep = ';')
Data = df.values  # Numpy array with data
print(df.describe())

X = Data[:,0]
Y = Data[:,1]

plt.figure(1)
plt.plot(X,Y)