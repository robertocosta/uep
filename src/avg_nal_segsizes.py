#!/usr/bin/env python3

def read_trace_line(filename):
    for line in open(filename, "rt"):
        line = line.rstrip("\n")
        if not line: continue
        fields = filter(bool, line.split(" "))
        traceline = dict()
        try:
            traceline["startPos"] = int(next(fields), 16)
        except ValueError as e:
            continue
        traceline["len"] = int(next(fields))
        traceline["lid"] = int(next(fields))
        traceline["tid"] = int(next(fields))
        traceline["qid"] = int(next(fields))
        traceline["packet_type"] = next(fields)
        traceline["discardable"] = (next(fields) == "Yes")
        traceline["truncatable"] = (next(fields) == "Yes")
        yield traceline

if __name__ == "__main__":
    import matplotlib.pyplot as plt
    import numpy as np
    import re
    import sys

    if len(sys.argv) < 2:
        print("Usage: {!s} <logfile>".format(sys.argv[0]), file=sys.stderr)
    logfile = sys.argv[1]

    # Sizes and paddding of the chunks with prio=0
    base_layer = {"sizes": [], "padding": []}
    # Sizes and paddding of the chunks with prio=1
    enhancement_layers = {"sizes": [], "padding": []}
    packet_size = None
    size_re = re.compile("nal_reader::pack_nals.*after_padding.*"
                         "packed_size=(\d+).*"
                         "priority=(\d+).*"
                         "padding=(\d+).*"
                         "pkt_size=(\d+)")

    for line in open(logfile, "rt"):
        m = size_re.search(line)
        if not m: continue
        size = int(m.group(1))
        prio = int(m.group(2))
        pad = int(m.group(3))
        pktsize = int(m.group(4))

        if prio == 0:
            base_layer["sizes"].append(size)
            base_layer["padding"].append(pad)
        else:
            enhancement_layers["sizes"].append(size)
            enhancement_layers["padding"].append(pad)
        if packet_size is None:
            packet_size = pktsize

    avg_bl_size = np.mean(base_layer["sizes"])
    avg_el_size = np.mean(enhancement_layers["sizes"])

    avg_bl_pad = np.mean(base_layer["padding"])
    avg_el_pad = np.mean(enhancement_layers["padding"])

    print("Base layer: avg. size = {:e}, avg. padding = {:e}".format(avg_bl_size, avg_bl_pad))
    print("Enhancement layers: avg. size = {:e}, avg.padding = {:e}".format(avg_el_size, avg_el_pad))

    plt.figure()
    plt.hist(base_layer["sizes"])
    plt.figure()
    plt.hist(enhancement_layers["sizes"])

    plt.show()
