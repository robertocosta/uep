import math
import matplotlib.pyplot as plt
import numpy as np
from utils import *
urls = getUrls('fast_data/time_',datetime.datetime(2017, 11, 5, 0, 0, tzinfo=tzutc()))

times_res = []

def getOverhead(item):
    return item['oh']

for url in urls:
    print('url: ',url)
    data = load_data(url)
    par_mat = data['param_matrix']
    res_mat = data['result_matrix']
    for j,rs in enumerate(res_mat)
        ps = par_mat[j]
        overheads = np.array([p.overhead for p in ps])
        enc_time = np.array([r.avg_enc_time for r in rs])
        dec_time = np.array([r.avg_dec_time for r in rs])
        
        k = sum(ps[0].Ks)
        assert(all(sum(p.Ks) == k for p in ps))
        
        enc_time *= k
        enc_time *= (1+overheads)

        ef = ps[0].EF
        assert(all(p.EF == ef for p in ps))

        par_str = 'EF = {:.d}'.format(ef)
        weight = rs[0].actual_nblocks * k
        assert(all(r.actual_nblocks * k == weight for r in rs))

        ind_i = 0
        if (len(time_res)>0):
            all_params = [time_res[i]['param_set'] for i in len(time_res)]
            if par_str in all_params:
                ind_i = all_params.index(par_str)
            else:
                times_res.append({'param_set': par_str, 'data': []})
                ind_i = len(times_res)-1
        else:
            times_res.append({'param_set': par_str, 'data': []})
        for n in range(0,len(rs)):
            oh = rs[n].overhead
            mib_time = rs[n]
		    if (len(time_res[ind_i]['data'])>0):
                ohs = [time_res[ind_i]['data'][i]['oh'] for i in range(0,len(time_res[ind_i]['data']))]
                if oh in ohs:
                    ind_oh = ohs.index(oh)
                    old_weight = times_res[ind_i]['data'][ind_oh]['weight']
                    mean_mib = times_res[ind_i]['data'][ind_oh]['mib_time']
                    mean_lib = times_res[ind_i]['data'][ind_oh]['lib_time']
                    times_res[ind_i]['data'][ind_oh]['weight'] = old_weight + weight
                    times_res[ind_i]['data'][ind_oh]['mib_time'] = update_average(mean_mib, old_weight, mib_time, weight)
                    times_res[ind_i]['data'][ind_oh]['lib_time'] = update_average(mean_lib, old_weight, lib_time, weight)




