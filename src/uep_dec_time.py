#!/usr/bin/env python3

import matplotlib.pyplot as plt
import numpy as np
import sys
import pandas as pd

from utils.logs import *

class uep_dec_time_scanner:
    def __init__(self, clientlog):
        self.client_scanner = line_scanner(clientlog)

        self.decode_uep_times = []
        def uep_decode_times_h(m):
            self.decode_uep_times.append(float(m.group(1)))
        self.decode_lt_times = []
        def lt_decode_times_h(m):
            self.decode_lt_times.append(float(m.group(1)))
        self.mp_run_times = []
        self.mp_dec_counts = []
        self.mp_out_sizes = []
        def mp_run_h(m):
            self.mp_run_times.append(float(m.group(1)))
            self.mp_dec_counts.append(int(m.group(2)))
            self.mp_out_sizes.append(int(m.group(3)))
        self.mp_setup_times = []
        def mp_setup_h(m):
            self.mp_setup_times.append(float(m.group(1)))

        self.client_scanner.add_regex("uep_decoder::push.*"
                                      "push_time=([^ ]+)",
                                      uep_decode_times_h)
        self.client_scanner.add_regex("lt_decoder::push.*"
                                      "push_time=([^ ]+)",
                                      lt_decode_times_h)
        self.client_scanner.add_regex("mp_context::run.*"
                                      "run_duration=([^ ]+).*"
                                      "decoded_count=(\d+).*"
                                      "output_size=(\d+)",
                                      mp_run_h)
        self.client_scanner.add_regex("block_decoder::run_message_passing.*"
                                      "mp_setup_time=([^ ]+)",
                                      mp_setup_h)

    def scan(self):
        self.client_scanner.scan()

        self.avg_uep_dec_time = np.mean(self.decode_uep_times)
        self.avg_lt_dec_time = np.mean(self.decode_lt_times)
        self.avg_mp_run_time = np.mean(self.mp_run_times)
        self.avg_mp_setup_time = np.mean(self.mp_setup_times)
        print("Avg. UEP decoding time = {:e}".format(self.avg_uep_dec_time))
        print("Avg. LT decoding time = {:e}".format(self.avg_lt_dec_time))
        print("Avg. MP run time = {:e}".format(self.avg_mp_run_time))
        print("Avg. MP setup time = {:e}".format(self.avg_mp_setup_time))

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: {!s} <client log>".format(sys.argv[0]),
              file=sys.stderr)
        sys.exit(2)
    clientlog = sys.argv[1]
    udts = uep_dec_time_scanner(clientlog)
    udts.scan()
