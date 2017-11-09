import datetime
import sys
if '../python' not in sys.path:
    sys.path.append('../python')
if './python' not in sys.path:
    sys.path.append('./python')
from uep_fast_run import *
from utils import *
# i.i.d. channel
# parameters:
plot_type = 'plot' # or 'scatter'
y_range_iid = [1e-4, 1]
x_range_iid = [0,0.4]
clipped = False
clipped_forward = True
urls = getUrls('fast_data/iid_',datetime.datetime(2017, 11, 5, 0, 0, tzinfo=tzutc()))
urls.remove('fast_data/iid_1510000281_14066231505785153409.pickle.xz')
urls.remove('fast_data/iid_1509990241_13281873338417605515.pickle.xz')
urls.remove('fast_data/iid_1510073199_10599483524714011910.pickle.xz')

iid_res = []

def getOverhead(item):
    return item['oh']
for url in urls:
    print('url: '+url)
    data = load_data(url)
    par_mat = data['param_matrix']
    res_mat = data['result_matrix']
    for i in range(0,len(par_mat)):
        i_param = par_mat[i]
        i_res = res_mat[i]
        par_str = '[UEP=[['+str(i_param[0].RFs[0])+','+str(i_param[0].RFs[1])+'],'
        par_str += str(i_param[0].EF)+'];L='+str(i_param[0].L)
        par_str += ';e='+'{:.3f}'.format(i_param[0].chan_pGB)
        par_str += ';k='+'{:d}'.format(i_param[0].Ks[0]+i_param[0].Ks[1])+']'
        ind_i = 0 # index
        if (len(iid_res)>0):
            all_params = [iid_res[i]['param_set'] for i in range(0,len(iid_res))]
            if par_str in all_params:
                ind_i = all_params.index(par_str)
            else:
                iid_res.append({'param_set': par_str, 'data': []})
                ind_i = len(iid_res)-1
        else:
            iid_res.append({'param_set': par_str, 'data': []})
        for j in range(0,len(i_res)):
            oh = i_param[j].overhead
            mib_per = i_res[j].avg_pers[0]
            lib_per = i_res[j].avg_pers[1]
            weight = i_res[j].actual_nblocks * (i_param[j].Ks[0]+i_param[j].Ks[1])
            nBlocks = i_res[j].actual_nblocks
            if (len(iid_res[ind_i]['data'])>0):
                ohs = [iid_res[ind_i]['data'][i]['oh'] for i in range(0,len(iid_res[ind_i]['data']))]
                if oh in ohs:
                    ind_oh = ohs.index(oh)
                    old_weight = iid_res[ind_i]['data'][ind_oh]['weight']
                    old_nBlocks = iid_res[ind_i]['data'][ind_oh]['nblocks']
                    mean_mib = iid_res[ind_i]['data'][ind_oh]['mib_per']
                    mean_lib = iid_res[ind_i]['data'][ind_oh]['lib_per']
                    iid_res[ind_i]['data'][ind_oh]['weight'] = old_weight + weight
                    iid_res[ind_i]['data'][ind_oh]['nblocks'] = old_nBlocks + nBlocks
                    iid_res[ind_i]['data'][ind_oh]['mib_per'] = update_average(mean_mib, old_weight, mib_per, weight)
                    iid_res[ind_i]['data'][ind_oh]['lib_per'] = update_average(mean_lib, old_weight, lib_per, weight)
                else:
                    iid_res[ind_i]['data'].append({'oh': oh, 'mib_per': mib_per, 'lib_per': lib_per, 'weight': weight, 'nblocks':nBlocks})
                    iid_res[ind_i]['data'] = sorted(iid_res[ind_i]['data'], key=getOverhead)

            else:
                iid_res[ind_i]['data'].append({'oh': oh, 'mib_per': mib_per, 'lib_per': lib_per, 'weight': weight, 'nblocks': nBlocks})
iid_plot = plt.figure(1)
plt.xlabel('Overhead')
plt.ylabel('UEP PER')
iid_weights = plt.figure(2)
plt.xlabel('Overhead')
plt.ylabel('n')
weights_max = 1
for i in range(0,len(iid_res)):
    print('parameter_set:' + iid_res[i]['param_set'])
    data = iid_res[i]['data']
    weights = [iid_res[i]['data'][j]['weight'] for j in range(0,len(iid_res[i]['data']))]
    x = [data[j]['oh'] for j in range(0,len(data))]
    plt.figure(2)
    weights = [iid_res[i]['data'][j]['weight'] for j in range(0,len(iid_res[i]['data']))]
    nBlocks = [iid_res[i]['data'][j]['nblocks'] for j in range(0,len(iid_res[i]['data']))]
    weights_max = max([weights_max, max(weights),max(nBlocks)])
    plot_params = { 'plt': plt, 'x' : x, 'y1' : nBlocks, 'y2' : weights,
        'legend' : ['nblocks ('+str(iid_res[i]['param_set'])+')','nCycles ('+str(iid_res[i]['param_set'])+')'],
        'plot_type' : plot_type, 'x_range' : x_range_iid, 'y_range':[1,weights_max]}
    draw2(plot_params)
    
    y1 = [data[j]['mib_per'] for j in range(0,len(data))]
    y2 = [data[j]['lib_per'] for j in range(0,len(data))]
    plt.figure(1)
    plot_params = { 'plt': plt, 'x' : x, 'y1' : y1, 'y2' : y2,
        'legend' : ['MIB='+iid_res[i]['param_set'],'LIB='+iid_res[i]['param_set']],
        'plot_type' : plot_type, 'x_range' : x_range_iid, 'y_range':y_range_iid,
        'clipped' : clipped, 'clipped_forward' : clipped_forward}
    draw2(plot_params)


datestr = datetime.datetime.now().strftime("%Y-%m-%d_%H-%M-%S")
save_plot(iid_plot,'aws_iid_'+plot_type+'_clipped-'+str(clipped)+'_'+datestr)
save_plot(iid_weights,'aws_iid_weights_'+plot_type+'_'+datestr)
 