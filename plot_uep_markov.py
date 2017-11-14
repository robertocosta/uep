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

if __name__ == "__main__":
    data = load_data_prefix("uep_17_11_14_3/uep_vs_k_markov_")
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
    param_filter = pf_all
    p = plots()
    #p.automaticXScale = True
    p.add_plot(plot_name='per',xlabel='K',ylabel='PER',logy=True)
    #p.add_plot(plot_name='nblocks',xlabel='K',ylabel='nblocks',logy=False)
    #param_filter = pf_all
    nPlots = 1 # how many different plot do you want for each condition? per, nbocks
    conditions = []
    six_plots = True
    if (six_plots == True):
        #conditions.append(parameters({'ks':'*','rfs':'*', 'ef':1, 'c':'*', 'delta':'*', 'type':'*'}))
        #conditions.append(parameters({'ks':'*','rfs':'*', 'ef':3, 'c':'*', 'delta':'*', 'type':'*'}))
        #conditions.append(parameters({'ks':'*','rfs':'*', 'ef':4, 'c':'*', 'delta':'*', 'type':'*'}))
        #conditions.append(parameters({'ks':'*','rfs':[3,1], 'ef':'*', 'c':'*', 'delta':'*', 'type':'*'}))
        #conditions.append(parameters({'ks':'*','rfs':[4,1], 'ef':'*', 'c':'*', 'delta':'*', 'type':'*'}))
        #conditions.append(parameters({'ks':'*','rfs':[1,1], 'ef': 1, 'c':'*', 'delta':'*', 'type':'*'}))
        #conditions.append(parameters({'ks':'*','rfs':[3,1], 'ef': 1, 'c':'*', 'delta':'*', 'type':'*'}))
        #conditions.append(parameters({'ks':'*','rfs':[1,1], 'ef': 3, 'c':'*', 'delta':'*', 'type':'*'}))
        #conditions.append(parameters({'ks':'*','rfs':[3,1], 'ef': 3, 'c':'*', 'delta':'*', 'type':'*'}))
        conditions.append(parameters({'ks':'*','rfs':[[1,1],[3,1]], 'ef':1, 'c':'*', 'delta':'*', 'type':'*'}))
        conditions.append(parameters({'ks':'*','rfs':[[1,1],[3,1]], 'ef':4, 'c':'*', 'delta':'*', 'type':'*'}))
        conditions.append(parameters({'ks':'*','rfs':[[1,1],[5,1]], 'ef':1, 'c':'*', 'delta':'*', 'type':'*'}))
        conditions.append(parameters({'ks':'*','rfs':[[1,1],[5,1]], 'ef':4, 'c':'*', 'delta':'*', 'type':'*'}))
        #conditions.append(parameters({'ks':'*','rfs':[5,1], 'ef':'*', 'c':'*', 'delta':'*', 'type':'*'}))
    else:
        conditions.append(parameters({'ks':[100,900],'rfs':'*', 'ef':'*', 'c':'*', 'delta':'*', 'type':'*'}))
    datestr = datetime.datetime.now().strftime("%Y-%m-%d_%H-%M-%S")
    RF_max = 5
    EF_max = 8
    p = plots()
        #p.cleanup()
    for cond in conditions:
        plot_name = 'per_' + cond.toStr
        plot_name_nbl = 'nblocks' + cond.toStr
        p.automaticXScale = True
        p.automaticXScale = [1,6200]

        if nPlots>0:
            p.add_plot(plot_name = plot_name,xlabel='K',ylabel='PER',logy=True)
        if nPlots>1:
            p.add_plot(plot_name=plot_name_nbl,xlabel='K',ylabel='nblocks',logy=False)
        if nPlots>2:
            p.add_plot(plot_name=plot_name[2],xlabel='Overhead',ylabel='ripple',logy=False)
        if nPlots>3:
            p.add_plot(plot_name=plot_name[3],xlabel='Overhead',ylabel='drop_rate',logy=False)

        for params in param_set:
            p.automaticYScale = [math.pow(10,-7), 1]
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
                        for j, _ in enumerate(Ks_frac):
                            avg_counters[j].add(d['avg_pers'][l][j],
                                                d['nblocks'])
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
            if (RFs[0]>RF_max):                                     continue
            if (EF>EF_max):                                         continue
            #if ((str(Ks) != cond.ks)*(cond.ks != '*')):             continue
            #print(dir(cond.rfs))
            if hasattr(cond.rfs[0], "__len__"):
                if ((str(RFs) not in cond.rfs)*(cond.rfs != '*')):  continue
            else:
                if ((str(RFs) != cond.rfs)*(cond.rfs != '*')):      continue
            if hasattr(cond.ef, "__len__"):
                #print(cond.ef)
                if ((EF not in cond.ef)*(cond.rfs != '*')):         continue
            else:
                if ((EF != cond.ef)*(cond.ef != '*')):              continue
            
            if ((c != cond.c)*(cond.c != '*')):                     continue
            if ((delta != cond.delta)*(cond.delta != '*')):         continue

            legend_str = (  "RFs={!s},"
                            "EF={:d},"
                            #"c={:.2f},"
                            #"delta={:.2f},"
                            #"oh={:.2f},"
                            "pi_b={:.0e},"
                            "EnB={:.2f},"
                            "Ks_frac={!s}").format(*params)
            legend_str = ( 
                            "RFs={!s},"
                            "EF={:d},"
                            "EnG={:.0f},"
                            "EnB={:.0f},"
                            "E[nblocks]={:.0e}").format(RFs[0], 
                                                        EF, 
                                                        avg_bad_run*(1-avg_per)/avg_per,
                                                        avg_bad_run, 
                                                        np.mean(nblocks))
            mibline = p.add_data(plot_name=plot_name,label=legend_str,type='mib',
                            x=k_blocks, y=avg_pers[:,0])
            if len(Ks_frac) > 1:
                p.add_data(plot_name=plot_name,label=legend_str,type='lib',
                        x=k_blocks, y=avg_pers[:,1],
                        color=mibline.get_color())
            
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
