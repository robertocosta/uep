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
        self.base_layer = {"sizes": [], "padding": []}
        self.enhancement_layers = {"sizes": [], "padding": []}
        self.uep_pktsize = None
        def nal_sizes_h(m):
            size = int(m.group(1))
            prio = int(m.group(2))
            pad = int(m.group(3))
            pktsize = int(m.group(4))

            if prio == 0:
                self.base_layer["sizes"].append(size)
                self.base_layer["padding"].append(pad)
            else:
                self.enhancement_layers["sizes"].append(size)
                self.enhancement_layers["padding"].append(pad)
                if self.uep_pktsize is None:
                    self.uep_pktsize = pktsize
        self.server_scanner.add_regex("nal_reader::pack_nals.*after_padding.*"
                                      "packed_size=(\d+).*"
                                      "priority=(\d+).*"
                                      "padding=(\d+).*"
                                      "pkt_size=(\d+)",
                                      nal_sizes_h)
        self.uep_padding_count = 0
        def uep_padding_count_h(m):
            self.uep_padding_count += int(m.group(1))
        self.server_scanner.add_regex("uep_encoder::pad_partial_block.*"
                                      "pad_cnt=(\d+)",
                                      uep_padding_count_h)
        self.uep_src_count = 0
        def uep_src_count_h(m):
            self.uep_src_count += 1
        self.server_scanner.add_regex("uep_encoder::push.*"
                                      "new_packet",
                                      uep_src_count_h)


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

        self.tot_bl_size = sum(self.base_layer["sizes"])
        self.tot_bl_pad = sum(self.base_layer["padding"])
        self.tot_el_size = sum(self.enhancement_layers["sizes"])
        self.tot_el_pad = sum(self.enhancement_layers["padding"])
        print("Total original size: BL = {:f} Mbyte, EL = {:f} MByte".format(self.tot_bl_size / 1024 /1024,
                                                                             self.tot_el_size /1024 / 1024))
        print("Total size = {:f} MByte".format((self.tot_bl_size + self.tot_el_size) / 1024 /1024))
        self.tot_after_pad = (self.tot_bl_size + self.tot_el_size +
                              self.tot_bl_pad + self.tot_el_pad)
        (self.uep_pkt_count, pkt_count_rem) = divmod(self.tot_after_pad, self.uep_pktsize)
        if (pkt_count_rem != 0):
            print("tot_after_pad is not an integer number of UEP pkts")
        print("Total after padding {:f} MByte."
              " UEP source packets = {:d} ({:d} bytes)."
              " Padding = {:f} Mbyte.".format(self.tot_after_pad / 1024 / 1024,
                                              self.uep_pkt_count,
                                              self.uep_pktsize,
                                              (self.tot_bl_pad + self.tot_el_pad) / 1024 / 1024))
        print("NAL padding overhead = {:f} %".format((self.tot_bl_pad + self.tot_el_pad) /
                                                   (self.tot_bl_size + self.tot_el_size) * 100))

        self.avg_bl_size = np.mean(self.base_layer["sizes"])
        self.avg_el_size = np.mean(self.enhancement_layers["sizes"])

        self.avg_bl_pad = np.mean(self.base_layer["padding"])
        self.avg_el_pad = np.mean(self.enhancement_layers["padding"])

        self.bl_frac = self.avg_bl_size / (self.avg_bl_size + self.avg_el_size)
        self.el_frac = self.avg_el_size / (self.avg_bl_size + self.avg_el_size)

        print("Base layer: avg. size = {:e},"
              " avg. padding = {:e}".format(self.avg_bl_size,
                                            self.avg_bl_pad))
        print("Enhancement layers: avg. size = {:e},"
              " avg.padding = {:e}".format(self.avg_el_size,
                                           self.avg_el_pad))
        print("BL fraction = {:e},"
              " EL fraction = {:e}".format(self.bl_frac, self.el_frac))

        self.uep_padding_oh = self.uep_padding_count / self.uep_src_count
        self.uep_header_oh = 4 / self.uep_pktsize
        self.uep_tot_oh = ((self.uep_padding_count + self.uep_src_count) * (4 + self.uep_pktsize) /
                           (self.uep_src_count * self.uep_pktsize))
        print("UEP overhead: padding = {:f}%,"
              " header = {:f}%,"
              " total = {:f}%".format(self.uep_padding_oh * 100,
                                      self.uep_header_oh * 100,
                                      (self.uep_tot_oh - 1) * 100))

        print("LT pkt overhead = {:f}%".format((self.udp_pktsize / self.lt_pktsize - 1) * 100))

        self.total_server_oh = self.tot_udp_size / (self.tot_bl_size + self.tot_el_size) - 1
        print("Total overhead NALs -> UDP = {:f}%".format(self.total_server_oh * 100))

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: {!s} <server log> <client log>".format(sys.argv[0]),
              file=sys.stderr)
        sys.exit(2)
    serverlog = sys.argv[1]
    clientlog = sys.argv[2]
    ohs = overhead_scanner(serverlog)
    ohs.scan()

    plt.figure()
    plt.hist(ohs.base_layer["sizes"])
    plt.title("Chunk sizes - Base Layer")
    plt.figure()
    plt.hist(ohs.enhancement_layers["sizes"])
    plt.title("Chunk sizes - Enhanc. Layers")
    plt.figure()
    plt.hist(ohs.base_layer["padding"])
    plt.title("Padding - Base Layer")
    plt.figure()
    plt.hist(ohs.enhancement_layers["padding"])
    plt.title("Padding - Enhanc. Layers")

    plt.show()
