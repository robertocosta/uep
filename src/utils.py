import math
import re

import numpy as np

from scipy import stats

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

def phi(n, z, alpha):
    assert(n >= 2)
    assert(z >= 0 and z <= n)
    assert(alpha >= 0 and alpha <= 1)
    n1 = 2*(z+1)
    n2 = 2*(n-z)
    f = stats.f.ppf(1-alpha, n1, n2)
    return n1*f / (n2 + n1*f)

def bernoulli_ci(z, n, gamma):
    def L(z):
        if z == 0:
            return 0
        else:
            return phi(n, z-1, (1+gamma) / 2)

    l = L(z)
    u = 1 - L(n-z)
    return (l, u)

def success_err(z, n):
    (l,u) = bernoulli_ci(z, n, 0.95)
    return (-(l - z/n), u - z/n)
