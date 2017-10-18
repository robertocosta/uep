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
    recvd_pkts = 0
    def recvd_pkts_h(m):
        global recvd_pkts
        recvd_pkts += int(m.group(1))
    client_scanner.add_regex("data_client::handle_receive.*"
                             "received_count=(\d+)",
                             recvd_pkts_h)

    server_scanner.scan()
    client_scanner.scan()
    
    udp_err_rate = 1 - recvd_pkts / sent_pkts

    print("UDP packet error rate = {:e}".format(udp_err_rate))
