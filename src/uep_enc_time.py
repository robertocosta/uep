#!/usr/bin/env python3

import matplotlib.pyplot as plt
import numpy as np
import sys
import pandas as pd

from utils import *

class uep_enc_time_scanner:
    def __init__(self, serverlog):
        self.server_scanner = line_scanner(serverlog)

        self.encode_times = []
        def encode_times_h(m):
            self.encode_times.append(float(m.group(1)))
        self.server_scanner.add_regex("lt_encoder::next_coded.*"
                                      "new_coded_packet.*"
                                      "encode_time=([^ ]+)",
                                      encode_times_h)

    def scan(self):
        self.server_scanner.scan()

        self.avg_enc_time = np.mean(self.encode_times)
        print("Avg. UEP encoding time = {:e}".format(self.avg_enc_time))

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: {!s} <server log>".format(sys.argv[0]),
              file=sys.stderr)
        sys.exit(2)
    serverlog = sys.argv[1]
    uets = uep_enc_time_scanner(serverlog)
    uets.scan()
