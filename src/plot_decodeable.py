#!/usr/bin/env python3

import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import re

logfiles = ['mp_plots_A.log', 'mp_plots_B.log', 'mp_plots_C.log']

for logf in logfiles:
    decoded_pkts = []
    decoded_pkts.append([])

    dec_re = re.compile('mp_context::run.*decoded_count=(\d+).*output_size=(\d+)');
    next_re = re.compile('encoder::next_block')

    blockno = 0
    last_out = 0
    with open(logf, "r") as f:
        while True:
            l = f.readline()
            if l == '':
                if decoded_pkts[blockno] == []:
                    del decoded_pkts[-1]
                break

            m = dec_re.search(l)
            if m:
                dec_count = int(m.group(1))
                out_size = int(m.group(2))
                if out_size != last_out+1:
                    raise RuntimeError("Should be ordered")
                decoded_pkts[blockno].append(dec_count);
                last_out = out_size

            m = next_re.search(l)
            if m:
                blockno += 1
                last_out = 0
                decoded_pkts.append([])

    print("Number of blocks: %d" % (len(decoded_pkts)))

    lengths = list(map(len, decoded_pkts))
    max_length = max(lengths)
    for i in decoded_pkts:
        pad = [i[-1]] * (max_length - len(i))
        i += pad
    print("Data padded to length %d" % (max_length))

    plt.figure()
    for i in decoded_pkts:
        plt.plot(range(1, max_length+1), i)

    avg_dec = np.mean(decoded_pkts, 0);
    var_dec = np.var(decoded_pkts, 0);
    plt.figure()
    plt.plot(range(1, max_length+1), avg_dec)
    plt.figure()
    plt.plot(range(1, max_length+1), var_dec)

plt.show()
