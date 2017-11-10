import datetime
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
    for j,rs in enumerate(res_mat):
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

        par_str = 'K={:d}, RF0={:d}, EF={:d}'.format(k,ps[0].RFs[0],ef)
        weight = rs[0].actual_nblocks * k
        assert(all(r.actual_nblocks * k == weight for r in rs))

        ind_i = 0
        if (len(times_res)>0):
            all_params = [times_res[i]['param_set'] for i in range(len(times_res))]
            if par_str in all_params:
                ind_i = all_params.index(par_str)
            else:
                times_res.append({'param_set': par_str, 'data': []})
                ind_i = len(times_res)-1
        else:
            times_res.append({'param_set': par_str, 'data': []})
        for n in range(len(enc_time)):
            # overheads[n]
            # enc_time[n]
            # dec_time[n]
            if (len(times_res[ind_i]['data'])>0):
                ohs = [times_res[ind_i]['data'][i]['oh'] for i in range(0,len(times_res[ind_i]['data']))]
                if overheads[n] in ohs:
                    ind_oh = ohs.index(overheads[n])
                    old_weight = times_res[ind_i]['data'][ind_oh]['weight']
                    mean_enc = times_res[ind_i]['data'][ind_oh]['enc_time']
                    mean_dec = times_res[ind_i]['data'][ind_oh]['dec_time']
                    times_res[ind_i]['data'][ind_oh]['weight'] = old_weight + weight
                    times_res[ind_i]['data'][ind_oh]['enc_time'] = update_average(mean_enc, old_weight, enc_time[n], weight)
                    times_res[ind_i]['data'][ind_oh]['dec_time'] = update_average(mean_dec, old_weight, dec_time[n], weight)
                else:
                    times_res[ind_i]['data'].append({'oh': overheads[n],
                                                     'enc_time': enc_time[n], 'dec_time': dec_time[n],
                                                     'weight': weight})
                    times_res[ind_i]['data'] = sorted(times_res[ind_i]['data'], key=getOverhead)
            else:
                times_res[ind_i]['data'].append({'oh': overheads[n],
                                                 'enc_time': enc_time[n], 'dec_time': dec_time[n],
                                                 'weight': weight})	


enc_plot = plt.figure('enc')
y_lim = [0, 0]
for i, res in enumerate(times_res):
    x = [res['data'][j]['oh'] for j in range(len(res['data']))]
    y = np.array([res['data'][j]['enc_time'] for j in range(len(res['data']))]) * 1000
    y_lim[0] = min([y_lim[0], min(y)])
    y_lim[1] = max([y_lim[1], max(y)])
    plt.plot(x, y, marker='.',linewidth=1,linestyle='-',label=res['param_set'])

#plt.gca().set_yscale('log')
plt.ylim(y_lim)
plt.xlabel('Overhead')
plt.ylabel('Encoding time [ms]')
plt.legend()
plt.grid()
#plt.show()

dec_plot = plt.figure('dec')
y_lim = [0, 0]
for i, res in enumerate(times_res):
    x = [res['data'][j]['oh'] for j in range(len(res['data']))]
    y = np.array([res['data'][j]['dec_time'] for j in range(len(res['data']))]) * 1000
    y_lim[0] = min([y_lim[0], min(y)])
    y_lim[1] = max([y_lim[1], max(y)])
    plt.plot(x, y, marker='.',linewidth=1,linestyle='-',label=res['param_set'])

#plt.gca().set_yscale('log')
plt.ylim(y_lim)
plt.xlabel('Overhead')
plt.ylabel('Decoding time [ms]')
plt.legend()
plt.grid()
#plt.show()

datestr = datetime.datetime.now().strftime("%Y-%m-%d_%H-%M-%S")
save_plot(enc_plot,'aws_enc_times_'+datestr)
save_plot(dec_plot,'aws_dec_times_'+datestr)

