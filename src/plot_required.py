#!/usr/bin/env python3

import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import re

logfiles = ['mp_plots_A.log', 'mp_plots_B.log', 'mp_plots_C.log']

for logf in logfiles:
    required_pkts = []
    req_re = re.compile('encoder::next_block.*coded_pkts=(\d+)')

    with open(logf, "r") as f:
        while True:
            l = f.readline()
            if l == '':
                break

            m = req_re.search(l)
            if m:
                dec_pkts = int(m.group(1))
                required_pkts.append(dec_pkts)

    print("Number of blocks: %d" % (len(required_pkts)))

    avg_required = np.mean(required_pkts)
    var_required = np.var(required_pkts)

    print("Average required pkts: %f" % (avg_required))
    print("Variance of required pkts: %f" % (var_required))

    plt.figure()
    plt.hist(required_pkts)

plt.show()
