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

def getKey(item):
    return item['k']
datestr = datetime.datetime.now().strftime("%Y-%m-%d_%H-%M-%S")

for filename in filenames:
    print(filename+':')
    #dictionary = pickle.loads(lzma.decompress(open(filename,'rb').read()))
    data = load_data_local(filename)
    par_mat = data['param_matrix']
    res_mat = data['result_matrix']
    n_parameters = len(par_mat)
    #print('   n_parameters tried = '+str(n_parameters))
    for i in range(0,n_parameters):
        data = lzma.compress(pickle.dumps({'result_matrix': [res_mat[i]],
                                   'param_matrix': [par_mat[i]]}))
        i_param = par_mat[i]
        #print('i_param=',i_param, 'dir(i_param=', str(dir(i_param)))
        par_str = '_p-'+'{:.2e}'.format(i_param[0].chan_pGB)+'_q-'+'{:.2e}'.format(i_param[0].chan_pBG)
        filename = datestr+'_per-vs-k'+par_str+'.xz'
        out_file = open(filename, 'wb')
        out_file.write(data)
