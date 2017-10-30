#!/usr/bin/env python3

import re
import shelve
import subprocess

import matplotlib.pyplot as plt
import numpy as np

def run_nal_overhead(n, L):
    out = subprocess.check_output(["./nal_overhead",
                                   "-n", n,
                                   "-L", str(L)]).decode()
    data = dict()
    data['npkts0'] = int(re.search("npkts0=(\d+)", out).group(1))
    data['npkts1'] = int(re.search("npkts1=(\d+)", out).group(1))
    data['size0'] = int(re.search("size0=(\d+)", out).group(1))
    data['size1'] = int(re.search("size1=(\d+)", out).group(1))
    data['oh0'] = int(re.search("oh0=(\d+)", out).group(1))
    data['oh1'] = int(re.search("oh1=(\d+)", out).group(1))
    return data

if __name__ == "__main__":
    shelf = shelve.open("plot_overhead_data")

    if 'Ls' in shelf and 'ohs' in shelf:
        Ls = shelf['Ls']
        ohs = shelf['ohs']
    else:
        Ls = list(np.linspace(1, 1500, 256, dtype=int))
        Ls += range(330, 560)
        Ls = sorted(set(Ls))

        strname = "stefan_cif_long"
        uep_hdr = 4
        lt_hdr = 11

        ohs = []
        for L in Ls:
            d = run_nal_overhead(strname, L)
            tot_src = d['size0'] + d['size1']
            tot_udp = (d['npkts0'] + d['npkts1']) * (L + uep_hdr + lt_hdr)
            oh = (tot_udp - tot_src) / tot_src
            ohs.append(oh)

        shelf['Ls'] = Ls
        shelf['ohs'] = ohs

    shelf.close()

    i_min = np.argmin(ohs)
    print("Best packet size = {:d}".format(Ls[i_min]))
    print("Min overhead = {:f}".format(ohs[i_min]))

    plt.figure();
    plt.plot(Ls, ohs)

    plt.ylim(0, 0.2)
    plt.xlim(1, 1500)
    plt.grid()
    plt.xlabel("L [byte]")
    plt.ylabel("overhead")
    plt.savefig("plot_overhead.pdf", format='pdf')
    plt.show();
