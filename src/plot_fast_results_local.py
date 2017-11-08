import datetime
import sys
if '../python' not in sys.path:
    sys.path.append('../python')
if './python' not in sys.path:
    sys.path.append('./python')
if './src' not in sys.path:
    sys.path.append('./src')
from uep_fast_run import *
from utils import *

import glob
filenames = glob.glob('*.xz')

results = []
plot_type = 'plot' # or 'scatter'
x_range_markov = [0, 15000]
y_range_markov = [1e-7, 1]
clipped = False
clipped_forward = True

def getKey(item):
    return item['k']

for filename in filenames:
    print(filename+':')
    #dictionary = pickle.loads(lzma.decompress(open(filename,'rb').read()))
    data = load_data_local(filename)
    par_mat = data['param_matrix']
    res_mat = data['result_matrix']
    n_parameters = len(par_mat)
    #print('   n_parameters tried = '+str(n_parameters))
    for i in range(0,n_parameters):
        i_param = par_mat[i]
        i_res = res_mat[i]
        par_str = '['
        #par_str = 'UEP=[['+str(i_param[0].RFs[0])+','+str(i_param[0].RFs[1])+'],'
        #par_str += str(i_param[0].EF)+'];L='+str(i_param[0].L) + ';'
        par_str += 'p='+'{:.2e}'.format(i_param[0].chan_pGB)+';q='+'{:.2e}'.format(i_param[0].chan_pBG)
        par_str += ';oh='+'{:.2f}'.format(i_param[0].overhead)+']'
        weight = i_param[0].nCycles
        n_k = len(i_res) # len(i_param) = len(i_res) = n_k
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
            if (len(results[ind_i]['data'])>0):
                ks = [results[ind_i]['data'][i]['k'] for i in range(0,len(results[ind_i]['data']))]
                if k in ks:
                    ind_k = ks.index(k)
                    
                    old_weight = results[ind_i]['data'][ind_k]['weight']
                    mean_mib = results[ind_i]['data'][ind_k]['mib_per']
                    mean_lib = results[ind_i]['data'][ind_k]['lib_per']
                    results[ind_i]['data'][ind_k]['weight'] = old_weight + weight
                    results[ind_i]['data'][ind_k]['mib_per'] = update_average(mean_mib, old_weight, mib_per, weight)
                    results[ind_i]['data'][ind_k]['lib_per'] = update_average(mean_lib, old_weight, lib_per, weight)
                else:
                    results[ind_i]['data'].append({'k': k, 'mib_per': mib_per, 'lib_per': lib_per, 'weight': weight})
                    results[ind_i]['data'] = sorted(results[ind_i]['data'], key=getKey)

            else:
                results[ind_i]['data'].append({'k': k, 'mib_per': mib_per, 'lib_per': lib_per, 'weight': weight})

my_plot = plt.figure(1)
plt.xlabel('K')
plt.ylabel('UEP PER')
markov_weights = plt.figure(2)
plt.xlabel('K')
plt.ylabel('n')
y_max = 1
for i in range(0,len(results)):
    print('parameter_set:' + results[i]['param_set'])
    data = results[i]['data']
    weights = [results[i]['data'][j]['weight'] for j in range(0,len(results[i]['data']))]
    #if (i==0): print(weights)
    x = [data[j]['k'] for j in range(0,len(data))]
    nblocks = [weights[j]/x[j] for j in range(0,len(x))]
    y_max = max([y_max, max(weights), max(nblocks)])
    plot_params = { 'plt': plt, 'x' : x, 'y1' : nblocks, 'y2' : weights,
        'legend' : ['nblocks ('+results[i]['param_set']+')','nCycles ('+results[i]['param_set']+')'],
        'plot_type' : plot_type, 'x_range' : x_range_markov, 'y_range':[1,y_max]}
    #print(plot_params)
    plt.figure(2)
    draw2(plot_params)
    y1 = [data[j]['mib_per'] for j in range(0,len(data))]
    y2 = [data[j]['lib_per'] for j in range(0,len(data))]
    plt.figure(1)
    plot_params = { 'plt': plt, 'x' : x, 'y1' : y1, 'y2' : y2,
        'legend' : ['MIB '+results[i]['param_set'],'LIB '+results[i]['param_set']],
        'plot_type' : plot_type, 'x_range' : x_range_markov, 'y_range':y_range_markov,
        'clipped' : clipped, 'clipped_forward' : clipped_forward}
    draw2(plot_params)
datestr = datetime.datetime.now().strftime("%Y-%m-%d_%H-%M-%S")
save_plot(my_plot,'local_markov_'+plot_type+'_'+datestr)
save_plot(markov_weights,'local_markov_weights_'+plot_type+'_'+datestr)

