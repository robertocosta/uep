#!/usr/bin/env python3

import re

class line_scanner:
    def __init__(self, fname):
        self.filename = fname
        self.regexes = []

    def add_regex(self, re_str, handler):
        self.regexes.append({
            "regex": re.compile(re_str),
            "handler": handler
        })

    def scan(self):
        for line in open(self.filename, "rt"):
            for r in self.regexes:
                m = r["regex"].search(line)
                if m: r["handler"](m)

if __name__ == "__main__":
    import functools
    import json
    import matplotlib.pyplot as plt
    import numpy as np
    import sys

    if len(sys.argv) < 3:
        print("Usage: {!s} <server log> <client log>".format(sys.argv[0]),
              file=sys.stderr)
        sys.exit(2)
    serverlog = sys.argv[1]
    clientlog = sys.argv[2]

    server_scanner = line_scanner(serverlog)
    client_scanner = line_scanner(clientlog)

    sent_pkts = 0
    def sent_pkts_h(m):
        global sent_pkts
        sent_pkts += 1
    server_scanner.add_regex("data_server::handle_sent.*udp_pkt_sent",
                             sent_pkts_h)
    sent_subblocks = []
    def sent_subblocks_h(m):
        global sent_subblocks
        a = json.loads(m.group(1))
        sent_subblocks.append(a)
    server_scanner.add_regex("uep_encoder::check_has_block.*"
                             "new_block.*"
                             "non_padding_pkts=(\[[\d, ]+?\])",
                             sent_subblocks_h)

    recvd_pkts = 0
    def recvd_pkts_h(m):
        global recvd_pkts
        recvd_pkts += int(m.group(1))
    client_scanner.add_regex("data_client::handle_receive.*"
                             "received_count=(\d+)",
                             recvd_pkts_h)
    recvd_subblocks = []
    def recvd_subblocks_h(m):
        global recvd_subblocks
        a = json.loads(m.group(1))
        recvd_subblocks.append(a)
    client_scanner.add_regex("uep_decoder::deduplicate_queued.*"
                             "received_block.*"
                             "pkt_counts=(\[[\d, ]+?\])",
                             recvd_subblocks_h)


    server_scanner.scan()
    client_scanner.scan()

    udp_err_rate = 1 - recvd_pkts / sent_pkts
    print("Total UDP sent packets = {:d}".format(sent_pkts))
    print("Total UDP received packets = {:d}".format(recvd_pkts))
    print("UDP packet error rate = {:e}".format(udp_err_rate))

    tot_sent = list(functools.reduce(lambda t, a: map(sum, zip(t, a)),
                                     sent_subblocks,
                                     [0,0]))
    tot_recvd = list(functools.reduce(lambda t, a: map(sum, zip(t, a)),
                                     recvd_subblocks,
                                     [0,0]))
    print("Total UEP source pkts = {!s}".format(tot_sent))
    print("Total UEP decoded pkts = {!s}".format(tot_recvd))
