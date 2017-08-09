#!/usr/bin/env python3

import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import re

logfiles = ['mp_plots_A.log', 'mp_plots_B.log', 'mp_plots_C.log']

for logf in logfiles:
    times = []
    times.append([])

    time_re = re.compile('mp_context::run.*run_duration=([^ ]+).*decoded_count=(\d+).*output_size=(\d+)');
    next_re = re.compile('encoder::next_block')

    blockno = 0
    last_out = 0
    with open(logf, "r") as f:
        while True:
            l = f.readline()
            if l == '':
                if times[blockno] == []:
                    del times[-1]
                break

            m = time_re.search(l)
            if m:
                time = float(m.group(1))
                dec_cnt = int(m.group(2))
                out_size = int(m.group(3))
                if out_size != last_out+1:
                    raise RuntimeError("Should be ordered")
                times[blockno].append((dec_cnt, time))
                last_out = out_size

            m = next_re.search(l)
            if m:
                blockno += 1
                last_out = 0
                times.append([])

    print("Number of blocks: %d" % (len(times)))

    sum_times = 0
    n_times = 0
    plt.figure()
    for i in times:
        pts = np.array(i)
        plt.scatter(pts[:,0], pts[:,1], marker='.')
        sum_times += sum(pts[:,1])
        n_times += len(i)

    avg_time = sum_times / n_times
    print("Avg. run duration: %f" % (avg_time))

    # lengths = list(map(len, times))
    # min_length = min(lengths)
    # for i in times:
    #     diff = len(i) - min_length
    #     if diff > 0:
    #         del i[-diff:]

    # print(list(map(len, times)))

    # print("Data truncated to length %d" % (min_length))

plt.show()
