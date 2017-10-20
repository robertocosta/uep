#!/usr/bin/env python3

import matplotlib.pyplot as plt
import numpy as np
import sys
import pandas as pd

from utils import *

if len(sys.argv) < 2:
    print("Usage: {!s} <client log>".format(sys.argv[0]),
          file=sys.stderr)
    sys.exit(2)
clientlog = sys.argv[1]
client_scanner = line_scanner(clientlog)

decode_uep_times = []
def uep_decode_times_h(m):
    global decode_uep_times
    decode_uep_times.append(float(m.group(1)))
decode_lt_times = []
def lt_decode_times_h(m):
    global decode_lt_times
    decode_lt_times.append(float(m.group(1)))
mp_run_times = []
mp_dec_counts = []
mp_out_sizes = []
def mp_run_h(m):
    global mp_run_times
    global mp_dec_counts
    global mp_out_sizes
    mp_run_times.append(float(m.group(1)))
    mp_dec_counts.append(int(m.group(2)))
    mp_out_sizes.append(int(m.group(3)))
mp_setup_times = []
def mp_setup_h(m):
    global mp_setup_times
    mp_setup_times.append(float(m.group(1)))

client_scanner.add_regex("uep_decoder::push.*"
                         "push_time=([^ ]+)",
                         uep_decode_times_h)
client_scanner.add_regex("lt_decoder::push.*"
                         "push_time=([^ ]+)",
                         lt_decode_times_h)
client_scanner.add_regex("mp_context::run.*"
                         "run_duration=([^ ]+).*"
                         "decoded_count=(\d+).*"
                         "output_size=(\d+)",
                         mp_run_h)
client_scanner.add_regex("block_decoder::run_message_passing.*"
                         "mp_setup_time=([^ ]+)",
                         mp_setup_h)

client_scanner.scan()

avg_uep_dec_time = np.mean(decode_uep_times)
avg_lt_dec_time = np.mean(decode_lt_times)
avg_mp_run_time = np.mean(mp_run_times)
avg_mp_setup_time = np.mean(mp_setup_times)
print("Avg. UEP decoding time = {:e}".format(avg_uep_dec_time))
print("Avg. LT decoding time = {:e}".format(avg_lt_dec_time))
print("Avg. MP run time = {:e}".format(avg_mp_run_time))
print("Avg. MP setup time = {:e}".format(avg_mp_setup_time))
