import concurrent.futures as cf
import datetime
import lzma
import math
import multiprocessing
import os
import os.path
import pickle
import random
import re
import sys

import boto3
import botocore
import matplotlib.pyplot as plt

from dateutil.tz import tzutc
from scipy import stats

if '../python' not in sys.path:
    sys.path.append('../python')
if './python' not in sys.path:
    sys.path.append('./python')

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

def update_average(m, sumw, s, w):
    #return m / (1 + w/sumw) + s / (w + sumw)
	return (m*sumw+s*w)/(sumw+w)



def getUrls(*arg):
    s3 = boto3.client('s3')
    fromDate = datetime.datetime(2017, 11, 5, 0, 0, tzinfo=tzutc())
    if (len(arg)>1):
        fromDate = arg[1]
    req = s3.list_objects_v2(
        Bucket='uep.zanol.eu',
        Prefix=arg[0],
    )
    urls = [req['Contents'][i]['Key'] for i in range(0,req['KeyCount']) 
        if req['Contents'][i]['LastModified']>fromDate]
    print('Imported '+str(len(urls))+' datasets since ',fromDate, ' with prefix '+arg[0])
    return urls

def save_plot(fname, key):
    s3 = boto3.client('s3')
    s3.upload_file(fname, 'uep.zanol.eu', key,
                   ExtraArgs={'ACL': 'public-read'})
    url = ("http://uep.zanol.eu."
           "s3.amazonaws.com/{!s}".format(key))
    return url

def save_data(key, **kwargs):
    s3 = boto3.client('s3')
    data = lzma.compress(pickle.dumps(kwargs))
    s3.put_object(Body=data,
                  Bucket='uep.zanol.eu',
                  Key=key,
                  ACL='public-read')
    url = ("http://uep.zanol.eu.s3"
           ".amazonaws.com/{!s}".format(key))
    return url

def mean(numbers):
    return float(sum(numbers)) / max(len(numbers), 1)

def save_plot_png(my_plot, name):
    print_fig = my_plot
    print_fig.set_size_inches(10,8)
    print_fig.set_dpi(200)
    fullname = "average/{}.png".format(name)
    os.makedirs(os.path.dirname(fullname), exist_ok=True)
    print_fig.savefig(fullname, format='png')

def load_data(key): 

    config = botocore.client.Config(read_timeout=300)
    #s3 = boto3.resource('s3', config=config, region_name='us-east-1')
    s3 = boto3.client('s3', config=config)
    bindata = s3.get_object(Bucket='uep.zanol.eu',
                        Key=key)
    data = lzma.decompress(bindata['Body'].read())
    return pickle.loads(data)
    
def load_data_local(filename):
    data = lzma.decompress(open(filename,'rb').read())
    return pickle.loads(data)

def send_notification(msg):
    sns = boto3.client('sns', region_name='us-east-1')

    sns.publish(TopicArn='arn:aws:sns:us-east-1:402432167722:NotifyMe',
                Message=msg)

def run_uep_parallel(param_matrix):
    result_futures = list()
    with cf.ProcessPoolExecutor() as executor:
        print("Running")
        for ps in param_matrix:
            result_futures.append(list())
            for p in ps:
                f = executor.submit(run_uep, p)
                result_futures[-1].append(f)
    print("Done")
    result_matrix = list()
    for fs in result_futures:
        result_matrix.append([f.result() for f in fs])
    return result_matrix
