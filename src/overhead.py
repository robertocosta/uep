#!/usr/bin/env python3

import functools
import json
import matplotlib.pyplot as plt
import numpy as np
import sys

from utils import *

class overhead_scanner:
    def __init__(self, serverlog):
        self.server_scanner = line_scanner(serverlog)

        self.sent_pkts = 0
        self.udp_pktsize = None
        def sent_pkts_h(m):
            self.sent_pkts += 1
            if self.udp_pktsize is None: self.udp_pktsize = int(m.group(1))
        self.server_scanner.add_regex("data_server::handle_sent.*"
                                      "udp_pkt_sent.*"
                                      "sent_size=(\d+)",
                                      sent_pkts_h)
        self.lt_pkts = 0
        self.lt_pktsize = None
        self.lt_blocks = set()
        def lt_pkts_h(m):
            self.lt_pkts += 1
            self.lt_blocks.add(m.group(1))
            if self.lt_pktsize is None: self.lt_pktsize = int(m.group(2))
        self.server_scanner.add_regex("lt_encoder::next_coded.*"
                                      "new_coded_packet=fountain_packet\{[^\}]*"
                                      "blockno=(\d+)[^\}]*"
                                      "size=(\d+).*?\}",
                                      lt_pkts_h)

    def scan(self):
        self.server_scanner.scan()

        self.sent_diff = self.lt_pkts - self.sent_pkts
        if self.sent_diff != 0:
            print(("Some LT packets were not sent:"
                  " LT pkts = {:d}"
                  " UDP pkts = {:d}").format(self.lt_pkts, self.sent_pkts))

        self.tot_udp_size = self.udp_pktsize * self.sent_pkts
        self.tot_lt_size = self.lt_pktsize * self.lt_pkts
        print("Total UDP payload = {:f} MByte".format(self.tot_udp_size / 1024 / 1024))
        print("Total LT encoded data = {:f} MByte".format(self.tot_lt_size / 1024 / 1024))
        print("Total input LT blocks = {:d}".format(len(self.lt_blocks)))
        


if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: {!s} <server log> <client log>".format(sys.argv[0]),
              file=sys.stderr)
        sys.exit(2)
    serverlog = sys.argv[1]
    clientlog = sys.argv[2]
    ohs = overhead_scanner(serverlog)
    ohs.scan()
