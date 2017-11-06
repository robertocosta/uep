import datetime
import lzma
import pickle
import sys
if '../python' not in sys.path:
    sys.path.append('../python')
if './python' not in sys.path:
    sys.path.append('./python')
import os
import matplotlib.pyplot as plt
import numpy as np
from uep_fast_run import *
from utils import *

s3 = boto3.client('s3')
iid_results = s3.list_objects_v2(
    Bucket='uep.zanol.eu',
    Prefix='iid_',
)
print(dir(iid_results))
last_markov_key = "fast_data/markov_1509867828_16491124733610827659.pickle.xz"
