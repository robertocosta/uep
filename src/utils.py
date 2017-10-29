import math
import re

import numpy as np

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

def read_trace_line(filename):
    for line in open(filename, "rt"):
        line = line.rstrip("\n")
        if not line: continue
        fields = filter(bool, line.split(" "))
        traceline = dict()
        try:
            traceline["startPos"] = int(next(fields), 16)
        except ValueError as e:
            continue
        traceline["len"] = int(next(fields))
        traceline["lid"] = int(next(fields))
        traceline["tid"] = int(next(fields))
        traceline["qid"] = int(next(fields))
        traceline["packet_type"] = next(fields)
        traceline["discardable"] = (next(fields) == "Yes")
        traceline["truncatable"] = (next(fields) == "Yes")
        yield traceline

def success_err(z, n):
    d = 1.96/n * math.sqrt(z*(1 - z/n))
    return (d, d)
