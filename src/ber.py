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
        self.last_block_sent = 0
        def last_block_sent_h(m):
            self.last_block_sent += 1
        def last_block_sent_reset(m):
            self.last_block_sent = 0
        self.server_scanner.add_regex("uep_encoder::check_has_block.*"
                                      "new_block",
                                      last_block_sent_reset)
        self.server_scanner.add_regex("data_server::handle_sent.*udp_pkt_sent",
                                      last_block_sent_h)
        self.last_block_recvd = None
        def last_block_recvd_h(m):
            self.last_block_recvd = int(m.group(1))
        self.client_scanner.add_regex("mp_context::run.*"
                                 "output_size=(\d+)",
                                 last_block_recvd_h)

    def scan(self):
        self.server_scanner.scan()
        self.client_scanner.scan()

        # Exclude the last block
        self.recvd_pkts -= self.last_block_recvd
        self.sent_pkts -= self.last_block_sent
        print("Excluding the last block in the BER"
              " UDP sent = {:d}, recvd = {:d}".format(self.last_block_sent,
                                                      self.last_block_recvd))

        self.udp_err_rate = 1 - self.recvd_pkts / self.sent_pkts
        print("Total UDP sent packets = {:d}".format(self.sent_pkts))
        print("Total UDP received packets = {:d}".format(self.recvd_pkts))
        print("UDP packet error rate = {:e}".format(self.udp_err_rate))

        self.recvd_pkts += self.last_block_recvd
        self.sent_pkts += self.last_block_sent

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
