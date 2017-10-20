#!/usr/bin/env python3

import matplotlib.pyplot as plt
import numpy as np
import sys
import pandas as pd

from utils import *

if len(sys.argv) < 2:
    print("Usage: {!s} <server log>".format(sys.argv[0]),
          file=sys.stderr)
    sys.exit(2)
serverlog = sys.argv[1]
server_scanner = line_scanner(serverlog)

encode_times = []
def encode_times_h(m):
    global encode_times
    encode_times.append(float(m.group(1)))
server_scanner.add_regex("lt_encoder::next_coded.*"
                         "new_coded_packet.*"
                         "encode_time=([^ ]+)",
                         encode_times_h)

server_scanner.scan()

avg_enc_time = np.mean(encode_times)
print("Avg. UEP encoding time = {:e}".format(avg_enc_time))
