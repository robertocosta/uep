import datetime

import matplotlib.pyplot as plt
import numpy as np

from parameters import *
from plots import *
from uep import *
from utils import *

def pf_all(RFs, EF, c, delta, overhead, avg_per, avg_bad_run, Ks_frac):
    return True

def pf_51(RFs, EF, c, delta, overhead, avg_per, avg_bad_run, Ks_frac):
    return (Ks == (100,900) and
            RFs == (5,1) and
            EF == 1)
def load_data_prefix_2(prefix, filter_func=None):
    s3 = boto3.client('s3')
    resp = s3.list_objects_v2(Bucket='uep.zanol.eu',
                              Prefix=prefix)
    items = list()
    print("Found {:d} packs".format(len(resp['Contents'])))
    for obj in resp['Contents']:
        if filter_func is None or filter_func(obj):
            items.append((load_data(obj['Key']), obj['Key']))
    return items


if __name__ == "__main__":
    url = "uep_markov_final/"
    data = load_data_prefix(url)
    #data = load_data_prefix("uep_markov_rc_17_11_12/uep_vs_k_markov_")
    print("Using {:d} data packs".format(len(data)))

    param_set = sorted(set((tuple(d['RFs']),
                            d['EF'],
                            d['c'],
                            d['delta'],
                            d['overhead'],
                            d['avg_per'],
                            d['avg_bad_run'],
                            tuple(d['Ks_frac'])) for d in data))
    #for item in param_set:
    # for j, k in load_data_prefix_2(url):
    #     pass
    #     legend_str = ("Ks={!s},"
    #                 "RFs={!s},"
    #                 "EF={:d},"
    #                 "c={:.2f},"
    #                 "delta={:.2f},"
    #                 "pib={:.1e}"
    #                 "enb={:.1e}").format(j.get('Ks', 'none'),
    #                                     j.get('RFs', 'none'),
    #                                     j.get('EF', -1),
    #                                     j.get('c', -1),
    #                                     j.get('delta', -1),
    #                                     j.get('avg_per',-1),
    #                                     j.get('avg_bad_run',-1))
    #     print("Key = " + k)
    #     print("  " + legend_str)
            
    param_filter = pf_all
    p = plots()
    #p.automaticXScale = True
    p.add_plot(plot_name='per',xlabel='K',ylabel='PER',logy=True)
    #p.add_plot(plot_name='nblocks',xlabel='K',ylabel='nblocks',logy=False)
    #param_filter = pf_all
    nPlots = 1 # how many different plot do you want for each condition? per, nbocks
    conditions = []
    four_plots = True
    ks = [100,1900]
    if (four_plots == True):
        #conditions.append(parameters({'ks':'*','rfs':'*', 'ef':1, 'c':'*', 'delta':'*', 'type':'*'}))
        #conditions.append(parameters({'ks':'*','rfs':'*', 'ef':3, 'c':'*', 'delta':'*', 'type':'*'}))
        #conditions.append(parameters({'ks':'*','rfs':'*', 'ef':4, 'c':'*', 'delta':'*', 'type':'*'}))
        #conditions.append(parameters({'ks':'*','rfs':[3,1], 'ef':'*', 'c':'*', 'delta':'*', 'type':'*'}))
        #conditions.append(parameters({'ks':'*','rfs':[4,1], 'ef':'*', 'c':'*', 'delta':'*', 'type':'*'}))
        #conditions.append(parameters({'ks':'*','rfs':[1,1], 'ef': 1, 'c':'*', 'delta':'*', 'type':'*'}))
        #conditions.append(parameters({'ks':'*','rfs':[3,1], 'ef': 1, 'c':'*', 'delta':'*', 'type':'*'}))
        #conditions.append(parameters({'ks':'*','rfs':[1,1], 'ef': 3, 'c':'*', 'delta':'*', 'type':'*'}))
        #conditions.append(parameters({'ks':'*','rfs':[3,1], 'ef': 3, 'c':'*', 'delta':'*', 'type':'*'}))
        #conditions.append(parameters({'ks':'*','rfs':[[1,1],[3,1]], 'ef':1, 'c':'*', 'delta':'*', 'type':'*'}))
        #conditions.append(parameters({'ks':'*','rfs':[[1,1],[3,1]], 'ef':4, 'c':'*', 'delta':'*', 'type':'*'}))
        #conditions.append(parameters({'ks':'*','rfs':[[1,1],[5,1]], 'ef':1, 'c':'*', 'delta':'*', 'type':'*'}))
        #conditions.append(parameters({'ks':'*','rfs':[[1,1],[5,1]], 'ef':4, 'c':'*', 'delta':'*', 'type':'*'}))
        #conditions.append(parameters({'ks':'*','rfs':'*', 'ef':'*', 'c':'*', 'delta':'*', 'type':'*'}))
       # conditions.append(parameters({  'ks':ks,
        #                                 'rfs':[[5,1],[1,1]], 
        #                                 'ef':1, 
        #                                 'c':'*', 
        #                                 'delta':'*', 
        #                                 'type':'*', 
        #                                 'pib':0.01,
        #                                 'enb':[50,500]}))
        # conditions.append(parameters({  'ks':ks,
        #                                 'rfs':[[5,1],[1,1]], 
        #                                 'ef':1, 
        #                                 'c':'*', 
        #                                 'delta':'*', 
        #                                 'type':'*', 
        #                                 'pib':[0, 0.1],
        #                                 'enb':[50,500]}))
        # conditions.append(parameters({  'ks':ks,
        #                                 'rfs':[[5,1],[3,1],[1,1]], 
        #                                 'ef':1, 
        #                                 'c':'*', 
        #                                 'delta':'*', 
        #                                 'type':'*', 
        #                                 'pib':[0.01, 0.1],
        #                                 'enb':5}))
        # conditions.append(parameters({  'ks':ks,
        #                                 'rfs':[[5,1],[3,1],[1,1]], 
        #                                 'ef':1, 
        #                                 'c':'*', 
        #                                 'delta':'*', 
        #                                 'type':'*', 
        #                                 'pib':[0.01, 0.1],
        #                                 'enb':10}))
        # conditions.append(parameters({  'ks':ks,
        #                                 'rfs':[[5,1],[3,1],[1,1]], 
        #                                 'ef':1, 
        #                                 'c':'*', 
        #                                 'delta':'*', 
        #                                 'type':'*', 
        #                                 'pib':[0.01, 0.1],
        #                                 'enb':50}))
        # for pib in [0.01, 0.1]:
        #     for enb in [5, 10, 50]:
        #         conditions.append(parameters({  'ks':ks,
        #                                         'rfs':[[5,1],[1,1]], 
        #                                         'ef':1, 
        #                                         'c':'*', 
        #                                         'delta':'*', 
        #                                         'type':'*', 
        #                                         'pib':pib,
        #                                         'enb':enb}))
        # conditions.append(parameters({  'ks':ks,
        #                                 'rfs':[[5,1],[3,1],[1,1]], 
        #                                 'ef':1, 
        #                                 'c':'*', 
        #                                 'delta':'*', 
        #                                 'type':'*', 
        #                                 'pib':[0.01],
        #                                 'enb':5}))
        # conditions.append(parameters({  'ks':ks,
        #                                 'rfs':[[5,1],[3,1],[1,1]], 
        #                                 'ef':1, 
        #                                 'c':'*', 
        #                                 'delta':'*', 
        #                                 'type':'*', 
        #                                 'pib':[0.1],
        #                                 'enb':[10]}))
        
        conditions.append(parameters({  'ks':ks,
                                        'rfs':[[5,1],[1,1]], 
                                        'ef':1, 
                                        'c':'*', 
                                        'delta':'*', 
                                        'type':'*', 
                                        'pib':[0],
                                        'enb':[1]}))        
        conditions.append(parameters({  'ks':ks,
                                        'rfs':[[5,1],[1,1]], 
                                        'ef':1, 
                                        'c':'*', 
                                        'delta':'*', 
                                        'type':'*', 
                                        'pib':[0.01],
                                        'enb':[5, 10, 50]}))
        conditions.append(parameters({  'ks':ks,
                                        'rfs':[[5,1],[1,1]], 
                                        'ef':1, 
                                        'c':'*', 
                                        'delta':'*', 
                                        'type':'*', 
                                        'pib':[0.1],
                                        'enb':[5, 10, 50]}))
        conditions.append(parameters({  'ks':ks,
                                        'rfs':[[5,1],[1,1]], 
                                        'ef':1, 
                                        'c':'*', 
                                        'delta':'*', 
                                        'type':'*', 
                                        'pib':[0.1],
                                        'enb':[50, 500]}))
 
    else:
        conditions.append(parameters({'ks':[100,900],'rfs':'*', 'ef':'*', 'c':'*', 'delta':'*', 'type':'*'}))

    datestr = datetime.datetime.now().strftime("%Y-%m-%d_%H-%M-%S")
    RF_max = 5
    EF_max = 8
    p = plots()
        #p.cleanup()
    for condN, cond in enumerate(conditions):
        plot_name = 'per_' + cond.toStr
        plot_name_nbl = 'nblocks' + cond.toStr
        # p.automaticXScale = True
        
        # if (condN < 2):
        #     p.automaticXScale = [1,6200]
        # if (condN == 2):
        #     p.automaticXScale = [1,10200]
        # if (condN == 4):
        #     p.automaticXScale = [1,15000]
        # if hasattr(cond.pib,'__len__'):
        #     p.automaticXScale = [1,6200]
        # else:
        #     if cond.pib == 0:
        #         p.automaticXScale = [1,10200]
        #     else:
        #         if cond.pib == 0.01:
        #             p.automaticXScale = [1, 6200]
        
        if nPlots>0:
            p.add_plot(plot_name = plot_name,xlabel='K',ylabel='PER',logy=True)
        if nPlots>1:
            p.add_plot(plot_name=plot_name_nbl,xlabel='K',ylabel='nblocks',logy=False)
        if nPlots>2:
            p.add_plot(plot_name=plot_name[2],xlabel='Overhead',ylabel='ripple',logy=False)
        if nPlots>3:
            p.add_plot(plot_name=plot_name[3],xlabel='Overhead',ylabel='drop_rate',logy=False)

        for params in param_set:
            if (condN < 2):
                p.automaticXScale = [1,3100]
                p.automaticYScale = [math.pow(10,-8), 1]
            if (condN == 2):
                p.automaticXScale = [1,10100]
                p.automaticYScale = [math.pow(10,-6), 1]
            if (condN == 3):
                p.automaticXScale = [1,15100]
                p.automaticYScale = [math.pow(10,-6), 1]
            # p.automaticYScale = [math.pow(10,-8), 1]
            data_same_pars = [d for d in data if (tuple(d['RFs']),
                                                d['EF'],
                                                d['c'],
                                                d['delta'],
                                                d['overhead'],
                                                d['avg_per'],
                                                d['avg_bad_run'],
                                                tuple(d['Ks_frac'])) == params]
            #print(params)
            RFs = params[0]
            EF = params[1]
            c = params[2]
            delta = params[3]
            overhead = params[4]
            avg_per = params[5]
            avg_bad_run = params[6]
            Ks_frac = params[7]
            k_blocks = sorted(set(k for d in data_same_pars for k in d['k_blocks']))
            avg_pers = np.zeros((len(k_blocks), len(Ks_frac)))
            nblocks = np.zeros(len(k_blocks))
            for i, k_block in enumerate(k_blocks):
                avg_counters = [AverageCounter() for k in Ks_frac]
                for d in data_same_pars:
                    for l, d_k_block in enumerate(d['k_blocks']):
                        if d_k_block != k_block: continue
                        if hasattr(d['nblocks'], '__len__'):
                            d_nblocks = d['nblocks'][l]
                        else:
                            d_nblocks = d['nblocks']
                        for j, _ in enumerate(Ks_frac):
                            avg_counters[j].add(d['avg_pers'][l][j],
                                                d_nblocks)
                avg_pers[i,:] = [c.avg for c in avg_counters]
                nblocks[i] = avg_counters[0].total_weigth
            #if not param_filter(*params): continue
            #if (RFs[0]>RF_max):                           continue
            #if (EF>EF_max):                              continue
            #if ((str(Ks) != cond.ks)*(cond.ks != '*')):         continue
            #if ((str(RFs) != cond.rfs)*(cond.rfs != '*')):      continue
            #if ((EF != cond.ef)*(cond.ef != '*')):              continue
            #if ((c != cond.c)*(cond.c != '*')):                 continue
            #if ((delta != cond.delta)*(cond.delta != '*')):     continue
            
            if (RFs[0]>RF_max):                         
                continue
            if (EF>EF_max):                             
                continue
            # overhead check
            if (cond.rfs != '*'):
                if hasattr(cond.rfs[0], "__len__"):
                    if (list(RFs) not in cond.rfs):    
                        #print('RFs=',list(RFs),' not in ',cond.rfs,'=cond.rf')  
                        continue
                else:
                    if (list(RFs) != cond.rfs):          
                        continue
            if (cond.ef != '*'):
                if hasattr(cond.ef, "__len__"):
                    if (EF not in cond.ef):            
                        continue
                else:
                    if (EF != cond.ef):                 
                        continue
            if (cond.pib != '*'):
                if hasattr(cond.pib, "__len__"):
                    # many channels
                    if avg_per not in cond.pib:
                        #print('avg_per=',avg_per,' not in ', cond.pib,'=cond.pib')   

                        continue
                else:
                    if (avg_per != cond.pib):  
                        continue
            if (cond.enb != '*'):
                if hasattr(cond.enb, "__len__"):
                    # many channels
                    if avg_bad_run not in cond.enb: 
                        #print('avg_bad_run=',avg_bad_run,' != ', cond.enb,'=cond.enb')   
                        continue
                else:
                    if (avg_bad_run != cond.enb):  
                        continue

            if ((c != cond.c)*(cond.c != '*')):        
                print('wrong c')
                continue
            if (cond.delta != '*'):
                if (delta != cond.delta):
                    print('wrong delta')
                    continue




            legend_str = (  "RFs={!s},"
                            "EF={:d},"
                            #"c={:.2f},"
                            #"delta={:.2f},"
                            #"oh={:.2f},"
                            "pi_b={:.0e},"
                            "EnB={:.2f},"
                            "Ks_frac={!s}").format(*params)
            eng = float('inf')
            if (avg_per!=0): eng = (avg_bad_run*(1-avg_per)/avg_per)
            legend_str = ( 
                            "RFs={!s},"
                            "EF={:d},"
                            "EnG={:.0f},"
                            "EnB={:.0f},"
                            "E[nblocks]={:.0e}").format(RFs[0], 
                                                        EF, 
                                                        eng,
                                                        avg_bad_run, 
                                                        np.mean(nblocks))
            if (RFs[0]>1):
                mibline = p.add_data(plot_name=plot_name,label=legend_str,type='mib',
                                x=k_blocks, y=avg_pers[:,0])
                if len(Ks_frac) > 1:
                    p.add_data(plot_name=plot_name,label=legend_str,type='lib',
                            x=k_blocks, y=avg_pers[:,1],
                            color=mibline.get_color())
            else:
                p.add_data(plot_name=plot_name,label=legend_str,type='eep',
                                x=k_blocks, y=np.mean(avg_pers,1))
            p.automaticYScale = True

            if nPlots > 1:
                p.add_data(plot_name=plot_name_nbl,label=legend_str,
                        x=k_blocks, y=nblocks)
            
            #p.add_data(plot_name='nblocks',label=legend_str,
            #        x=k_blocks, y=nblocks)

            #the_oh_is = [i for i,oh in enumerate(overheads)
            #             if math.isclose(oh, 0.24)]
            #if len(the_oh_is) > 0:
            #    the_oh_i = the_oh_is[0]
            #    print("At overhead {:.2f}:".format(overheads[the_oh_i]))
            #    print(" " * 4 + legend_str, end="")
            #    print(" -> MIB={:e}, LIB={:e}".format(avg_pers[the_oh_i, 0],
            #                                          avg_pers[the_oh_i, 1]))
            print(p.describe_plot(plot_name)+ ' : ' + legend_str)
        

        if nPlots>0:
            save_plot_png(p.get_plot(plot_name),datestr + '_' + p.describe_plot(plot_name))
        if nPlots>1:
            save_plot_png(p.get_plot(plot_name_nbl),datestr + '_' + p.describe_plot(plot_name_nbl))

    #save_plot_png(p.get_plot('per'),'iid '+p.describe_plot('per')+datestr)
    #save_plot_png(p.get_plot('nblocks'),'iid '+p.describe_plot('nblocks')+datestr)


    #plt.show()
