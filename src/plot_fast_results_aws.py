import datetime
from dateutil.tz import tzutc
import lzma
import pickle
import sys
if '../python' not in sys.path:
    sys.path.append('../python')
if './python' not in sys.path:
    sys.path.append('./python')
import os
import matplotlib.pyplot as plt
import numpy as np
from uep_fast_run import *
from utils import *

s3 = boto3.client('s3')
iid_res = []
firstDate = datetime.datetime(2017, 11, 5, 1, 1, tzinfo=tzutc())
iid_req = s3.list_objects_v2(
    Bucket='uep.zanol.eu',
    Prefix='fast_data/iid_',
)

urls = [iid_req['Contents'][i]['Key'] for i in range(0,iid_req['KeyCount']) 
    if iid_req['Contents'][i]['LastModified']>firstDate]
print('Imported '+str(len(urls))+' datasets:')
#print(urls)
def getKey(item):
    return item['k']

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
        par_str += ';p='+'{:.2f}'.format(i_param[0].chan_pGB)+';q='+'{:.2f}'.format(i_param[0].chan_pBG)
        par_str += ';oh='+'{:.2f}'.format(i_param[0].overhead)+']'
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
            k = i_param[j].Ks[0]+i_param[j].Ks[1]
            mib_per = i_res[j].avg_pers[0]
            lib_per = i_res[j].avg_pers[1]
            weight = i_res[j].actual_nblocks / k
            if (len(iid_res[ind_i]['data'])>0):
                ks = [iid_res[ind_i]['data'][i]['k'] for i in range(0,len(iid_res[ind_i]['data']))]
                if k in ks:
                    ind_k = ks.index(k)
                    old_weight = iid_res[ind_i]['data'][ind_k]['weight']
                    mean_mib = iid_res[ind_i]['data'][ind_k]['mib_per']
                    mean_lib = iid_res[ind_i]['data'][ind_k]['lib_per']
                    iid_res[ind_i]['data'][ind_k]['weight'] = old_weight + weight
                    iid_res[ind_i]['data'][ind_k]['mib_per'] = update_average(mean_mib, old_weight, mib_per, weight)
                    iid_res[ind_i]['data'][ind_k]['lib_per'] = update_average(mean_lib, old_weight, lib_per, weight)
                else:
                    iid_res[ind_i]['data'].append({'k': k, 'mib_per': mib_per, 'lib_per': lib_per, 'weight': weight})
                    iid_res[ind_i]['data'] = sorted(iid_res[ind_i]['data'], key=getKey)

            else:
                iid_res[ind_i]['data'].append({'k': k, 'mib_per': mib_per, 'lib_per': lib_per, 'weight': weight})

my_plot = plt.figure(1)
plt.gca().set_yscale('log')
plt.gca().set_xscale('log')
for i in range(0,len(results)):
    print('parameter_set:' + results[i]['param_set'])
    data = results[i]['data']
    x = [data[j]['k'] for j in range(0,len(data))]
    y1 = [data[j]['mib_per'] for j in range(0,len(data))]
    y2 = [data[j]['lib_per'] for j in range(0,len(data))]
    plt.scatter(x, y1, s = np.ones((len(y1),1))*(20-2*i)	,label=('MIB:'+results[i]['param_set']))
    #plt.plot(x, y2,marker='o',linewidth=0.5,label=('LIB:'+results[i]['param_set']))
    plt.scatter(x, y2, s = np.ones((len(y1),1))*(14-2*i),	label=('LIB:'+results[i]['param_set']))


plt.ylim(1e-10, 1)
plt.xlabel('K')
plt.ylabel('UEP PER')
plt.legend()
plt.grid()
datestr = datetime.datetime.now().strftime("%Y-%m-%d_%H-%M-%S")
print_fig = my_plot
print_fig.set_size_inches(10,8)
print_fig.set_dpi(200)
os.makedirs('avg', 777, True)
print_fig.savefig('avg/aws_'+datestr+'.png', format='png')

        
        
        
        
        
