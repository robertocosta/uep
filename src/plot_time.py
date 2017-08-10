#!/usr/bin/env python3

import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import re

logfiles = ['mp_plots_A.log', 'mp_plots_B.log', 'mp_plots_C.log']

for logf in logfiles:
    times = []
    times.append([])
    setup_times = []
    setup_times.append([])

    time_re = re.compile('mp_context::run.*run_duration=([^ ]+).*decoded_count=(\d+).*output_size=(\d+)');
    next_re = re.compile('encoder::next_block')
    setup_re = re.compile('block_decoder::run_message_passing.*mp_setup_time=([^ ]+)')

    blockno = 0
    last_out = 0
    with open(logf, "r") as f:
        while True:
            l = f.readline()
            if l == '':
                if times[blockno] == []:
                    del times[-1]
                    del setup_times[-1]
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


            m = setup_re.search(l)
            if m:
                setup_time = float(m.group(1))
                # Comes after each `run` time: aligned with `times`
                setup_times[blockno].append(setup_time)

            m = next_re.search(l)
            if m:
                blockno += 1
                last_out = 0
                times.append([])
                setup_times.append([])

    print("Number of blocks: %d" % (len(times)))

    sum_times = 0
    sum2_times = 0
    n_times = 0
    plt.figure()
    for i in times:
        pts = np.array(i)
        plt.scatter(pts[:,0], pts[:,1], marker='.')
        sum_times += sum(pts[:,1])
        sum2_times += sum(pts[:,1]) ** 2
        n_times += len(i)

    avg_time = sum_times / n_times
    var_time = (sum2_times - sum_times) / n_times
    print("Avg. mp run duration: %f" % (avg_time))
    print("Var. of mp run duration: %f" % (var_time))

    sum_setup_t = 0
    sum2_setup_t = 0
    n_setup_t = 0
    plt.figure()
    for b,i in enumerate(setup_times):
        dec_pts = np.array(times[b])
        plt.scatter(dec_pts[:,0], i)
        sum_setup_t += sum(i)
        sum2_setup_t += sum(i) ** 2
        n_setup_t += len(i)

    avg_setup_time = sum_setup_t / n_setup_t
    var_setup_time = (sum2_setup_t - sum_setup_t) / n_setup_t
    print("Avg. mp setup duration: %f" % (avg_setup_time))
    print("Var. of mp setup duration: %f" % (var_setup_time))

plt.show()
