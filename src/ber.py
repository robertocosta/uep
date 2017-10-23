#!/usr/bin/env python3

import functools
import json
import matplotlib.pyplot as plt
import numpy as np
import sys

from utils import *

class ber_scanner:
    def __init__(self, serverlog, clientlog):
        self.server_scanner = line_scanner(serverlog)
        self.client_scanner = line_scanner(clientlog)

        self.sent_pkts = 0
        def sent_pkts_h(m):
            self.sent_pkts += 1
        self.server_scanner.add_regex("data_server::handle_sent.*udp_pkt_sent",
                                      sent_pkts_h)
        self.sent_subblocks = []
        def sent_subblocks_h(m):
            a = json.loads(m.group(1))
            self.sent_subblocks.append(a)
        self.server_scanner.add_regex("uep_encoder::check_has_block.*"
                                      "new_block.*"
                                      "non_padding_pkts=(\[[\d, ]+?\])",
                                      sent_subblocks_h)

        self.recvd_pkts = 0
        def recvd_pkts_h(m):
            self.recvd_pkts += int(m.group(1))
        self.client_scanner.add_regex("data_client::handle_receive.*"
                                      "received_count=(\d+)",
                                      recvd_pkts_h)
        self.recvd_subblocks = []
        def recvd_subblocks_h(m):
            a = json.loads(m.group(1))
            self.recvd_subblocks.append(a)
        self.client_scanner.add_regex("uep_decoder::deduplicate_queued.*"
                                      "received_block.*"
                                      "pkt_counts=(\[[\d, ]+?\])",
                                      recvd_subblocks_h)

    def scan(self):
        self.server_scanner.scan()
        self.client_scanner.scan()

        self.udp_err_rate = 1 - self.recvd_pkts / self.sent_pkts
        print("Total UDP sent packets = {:d}".format(self.sent_pkts))
        print("Total UDP received packets = {:d}".format(self.recvd_pkts))
        print("UDP packet error rate = {:e}".format(self.udp_err_rate))

        self.tot_sent = list(functools.reduce(lambda t, a: map(sum, zip(t, a)),
                                         self.sent_subblocks,
                                         [0,0]))
        self.tot_recvd = list(functools.reduce(lambda t, a: map(sum, zip(t, a)),
                                         self.recvd_subblocks,
                                         [0,0]))
        self.uep_err_rates = list(map(lambda a: 1 - a[0] / a[1],
                                 zip(self.tot_recvd, self.tot_sent)))
        print("Total UEP source pkts = {!s}".format(self.tot_sent))
        print("Total UEP decoded pkts = {!s}".format(self.tot_recvd))
        print("UEP packet error rate: prio 0 = {:e}"
              ", prio 1 = {:e}".format(self.uep_err_rates[0],
                                       self.uep_err_rates[1]))

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: {!s} <server log> <client log>".format(sys.argv[0]),
              file=sys.stderr)
        sys.exit(2)
    serverlog = sys.argv[1]
    clientlog = sys.argv[2]
    bs = ber_scanner(serverlog, clientlog)
    bs.scan()
