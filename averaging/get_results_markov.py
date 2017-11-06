import datetime
import lzma
import pickle
import sys
if '../python' not in sys.path:
    sys.path.append('../python')
if './python' not in sys.path:
    sys.path.append('./python')

import matplotlib.pyplot as plt
import numpy as np
from uep_fast_run import *

import glob
filenames = glob.glob('*.xz');

results = []

def getKey(item):
    return item['k']

for filename in filenames:
    print(filename+':')
    dictionary = pickle.loads(lzma.decompress(open(filename,'rb').read()))
    par_mat = dictionary['param_matrix']
    res_mat = dictionary['result_matrix']
    n_parameters = len(par_mat)
    print('   n_parameters tried = '+str(n_parameters))
    for i in range(0,n_parameters):
        i_param = par_mat[i]
        #print(dir(i_param[0])) #'EF', 'Ks', 'L', 'RFs', 'c', 'chan_pBG', 'chan_pGB', 'delta', 'nCycles', 'nblocks', 'overhead'
        par_str = '[UEP=[['+str(i_param[0].RFs[0])+','
        par_str += str(i_param[0].RFs[1])+'],'+str(i_param[0].EF)+'];L='+str(i_param[0].L)
        par_str += ';p='+'{:.2f}'.format(i_param[0].chan_pGB)+';q='+'{:.2f}'.format(i_param[0].chan_pBG)
        par_str += ';oh='+'{:.2f}'.format(i_param[0].overhead)+']'
        #par_str += ';K=['+str(i_param[0].Ks[0])+','+str(i_param[0].Ks[1])+'] ]]'
        weight = i_param[0].nCycles
        #print('      '+par_str)
        #print(len(i_param))
        #print(dir(i_param)) #append, clear, copy, count, extend, index, insert, pop, remove, sort, reverse
        i_res = res_mat[i]
        n_k = len(i_res) # len(i_param) = len(i_res) = n_k
        #print(dir(i_res[0])) # 'avg_enc_time', 'avg_pers', 'dropped_count', 'rec_counts'
        #print('         n_simulations = '+str(n_k))
        ind_i = 0 # index
        if (len(results)>0):
            all_params = [results[i]['param_set'] for i in range(0,len(results))]
            if par_str in all_params:
                ind_i = all_params.index(par_str)
            else:
                results.append({'param_set': par_str, 'data': []})
                ind_i = len(results)-1
        else:
            results.append({'param_set': par_str, 'data': []})
        for j in range(0,n_k):
            k = i_param[j].Ks[0]+i_param[j].Ks[1]
            mib_per = i_res[j].avg_pers[0]
            lib_per = i_res[j].avg_pers[1]
            j_str = '            k '+str(k)+', mib_per '+'{:.5f}'.format(mib_per)+', lib_per {:.5f}'.format(lib_per)
            if (len(results[ind_i]['data'])>0):
                ks = [results[ind_i]['data'][i]['k'] for i in range(0,len(results[ind_i]['data']))]
                if k in ks:
                    ind_k = ks.index(k)
                    old_weight = results[ind_i]['data'][ind_k]['weight']
                    results[ind_i]['data'][ind_k]['weight'] = old_weight + weight
                    old_val = results[ind_i]['data'][ind_k]['mib_per']
                    results[ind_i]['data'][ind_k]['mib_per'] = (old_val*old_weight + mib_per*weight)/(old_weight + weight)
                    old_val = results[ind_i]['data'][ind_k]['lib_per']
                    results[ind_i]['data'][ind_k]['lib_per'] = (old_val*old_weight + lib_per*weight)/(old_weight + weight)
                else:
                    results[ind_i]['data'].append({'k': k, 'mib_per': mib_per, 'lib_per': lib_per, 'weight': weight})
                    results[ind_i]['data'] = sorted(results[ind_i]['data'], key=getKey)

            else:
                results[ind_i]['data'].append({'k': k, 'mib_per': mib_per, 'lib_per': lib_per, 'weight': weight})

my_plot = plt.figure(1)
plt.gca().set_yscale('log')
#plt.gca().set_xscale('log')
for i in range(0,len(results)):
    print('parameter_set:')
    print(results[i]['param_set'])
    data = results[i]['data']
    x = [data[j]['k'] for j in range(0,len(data))]
    y1 = [data[j]['mib_per'] for j in range(0,len(data))]
    y2 = [data[j]['lib_per'] for j in range(0,len(data))]
    plt.plot(x, y1,marker='o',linewidth=0.5,label=('MIB:'+results[i]['param_set']))
    plt.plot(x, y2,marker='o',linewidth=0.5,label=('LIB:'+results[i]['param_set']))


plt.ylim(1e-8, 1)
#plt.xlim(1000, 2000)
plt.xlabel('K')
plt.ylabel('UEP PER')
plt.legend()
plt.grid()
datestr = datetime.datetime.now().strftime("%Y-%m-%d_%H-%M-%S")
print_fig = my_plot
print_fig.set_size_inches(10,8)
print_fig.set_dpi(200)
print_fig.savefig('averaging/'+datestr+'_avg.png', format='png')
