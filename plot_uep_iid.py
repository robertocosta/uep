import datetime

import matplotlib.pyplot as plt
import numpy as np

from parameters import *
from plots import *
from uep import *
from utils import *

def pf_all(Ks, RFs, EF, c, delta, iid_per):
    return True

def pf_EF_1000(Ks, RFs, EF, c, delta, iid_per):
    return (Ks == (100,900) and
            RFs == (1,1))

def pf_RFs(Ks, RFs, EF, c, delta, iid_per):
    return (Ks == (100,900) and
            EF == 4)

def pf_EF_20000(Ks, RFs, EF, c, delta, iid_per):
    return (Ks == (20000,) and
            RFs == (1,))

def pf_EEP(Ks, RFs, EF, c, delta, iid_per):
    return ((RFs == (1,) or RFs == (1,1)) and
            EF == 1)

def pf_EF_EEP(Ks, RFs, EF, c, delta, iid_per):
    return RFs == (1,)

if __name__ == "__main__":
    #data = load_data_prefix("uep_iid_other_deg_fix/uep_vs_oh_iid_")
    data = load_data_prefix("uep_iid_other/uep_vs_oh_iid_")
    print("Using {:d} data packs".format(len(data)))
    datestr = datetime.datetime.now().strftime("%Y-%m-%d_%H-%M-%S")

    param_set = sorted(set((tuple(d['Ks']),
                            tuple(d['RFs']),
                            d['EF'],
                            d['c'],
                            d['delta'],
                            d['iid_per']) for d in data))

    # Old results no not have the avg_ripples
    for d in data:
        if 'avg_ripples' not in d:
            d['avg_ripples'] = [float('nan') for o in d['overheads']]

    #param_filter = pf_all

    nPlots = 1 # how many different plot do you want? per, nblocks, ripple, drop_rate

    conditions = []
    six_plots = True
    ks = [100,1900]
    if (six_plots == True):
        #conditions.append(parameters({'ks':ks,'rfs':'*', 'ef':1, 'c':'*', 'delta':'*', 'type':'*'}))
        #conditions.append(parameters({'ks':[100,1900],'rfs':'*', 'ef':2, 'c':'*', 'delta':'*', 'type':'*'}))
        #conditions.append(parameters({'ks':[100,1900],'rfs':'*', 'ef':4, 'c':'*', 'delta':'*', 'type':'*'}))
        #conditions.append(parameters({'ks':[100,1900],'rfs':[3,1], 'ef':'*', 'c':'*', 'delta':'*', 'type':'*'}))
        #conditions.append(parameters({'ks':ks,'rfs':[[5,1],[3,1],[1,1]], 'ef':1, 'c':'*', 'delta':'*', 'type':'*'}))
        #conditions.append(parameters({'ks':ks,'rfs':[[3,1],[1,1]], 'ef':1, 'c':'*', 'delta':'*', 'type':'*'}))
        #conditions.append(parameters({'ks':ks,'rfs':[[3,1],[1,1]], 'ef':2, 'c':'*', 'delta':'*', 'type':'*'}))
        conditions.append(parameters({'ks':ks,'rfs':[[5,1],[1,1]], 'ef':1, 'c':'*', 'delta':'*', 'type':'*'}))
        #conditions.append(parameters({'ks':ks,'rfs':[[5,1],[1,1]], 'ef':2, 'c':'*', 'delta':'*', 'type':'*'}))
        #conditions.append(parameters({'ks':[100,1900],'rfs':[4,1], 'ef':'*', 'c':'*', 'delta':'*', 'type':'*'}))
        #conditions.append(parameters({'ks':ks,'rfs':[[5,1],[1,1]], 'ef':[1,2], 'c':'*', 'delta':'*', 'type':'*'}))
    else:
        conditions.append(parameters({'ks':'*','rfs':'*', 'ef':'*', 'c':'*', 'delta':'*', 'type':'*'}))

    RF_max = 5
    EF_max = 8

    for cond in conditions:
        plot_name = [   'per_' + cond.toStr,
                        'nblocks_' + cond.toStr, 
                        'ripple_' + cond.toStr, 
                        'drop_rate_'+ cond.toStr]
        p = plots()
        p.automaticXScale = True
        #p.automaticXScale = [0,0.3]

        if nPlots>0:
            p.add_plot(plot_name=plot_name[0],xlabel='overhead',ylabel='PER',logy=True)
        if nPlots>1:
            p.add_plot(plot_name=plot_name[1],xlabel='overhead',ylabel='nblocks',logy=False)
        if nPlots>2:
            p.add_plot(plot_name=plot_name[2],xlabel='overhead',ylabel='ripple',logy=False)
        if nPlots>3:
            p.add_plot(plot_name=plot_name[3],xlabel='overhead',ylabel='drop_rate',logy=False)

        for params in param_set:
            p.automaticYScale = [math.pow(10,-7), 1]

            data_same_pars = [d for d in data if (tuple(d['Ks']),
                                                tuple(d['RFs']),
                                                d['EF'],
                                                d['c'],
                                                d['delta'],
                                                d['iid_per']) == params]
            Ks = params[0]
            RFs = params[1]
            EF = params[2]
            c = params[3]
            delta = params[4]
            iid_per = params[5]

            overheads = sorted(set(o for d in data_same_pars for o in d['overheads']))

            avg_pers = np.zeros((len(overheads), len(Ks)))
            nblocks = np.zeros(len(overheads))
            avg_ripples = np.zeros(len(overheads))
            avg_drop_rates = np.zeros(len(overheads))
            for i, oh in enumerate(overheads):
                avg_counters = [AverageCounter() for k in Ks]
                avg_ripple = AverageCounter()
                avg_drop = AverageCounter()
                for d in data_same_pars:
                    for l, d_oh in enumerate(d['overheads']):
                        if d_oh != oh: continue

                        for j,k in enumerate(Ks):
                            per = d['error_counts'][l][j] / (k * d['nblocks'])
                            avg_counters[j].add(per, d['nblocks'])
                        if not math.isnan(d['avg_ripples'][l]):
                            avg_ripple.add(d['avg_ripples'][l],
                                        d['nblocks'])
                        avg_drop.add(d['avg_drops'][l], d['nblocks'])

                avg_ripples[i] = avg_ripple.avg
                avg_drop_rates[i] = avg_drop.avg
                avg_pers[i,:] = [c.avg for c in avg_counters]
                nblocks[i] = avg_counters[0].total_weigth

            #if not param_filter(*params): continue
            if (RFs[0]>RF_max):                                     continue
            if (EF>EF_max):                                         continue
            if ((str(Ks) != cond.ks)*(cond.ks != '*')):             continue
            #print(dir(cond.rfs))
            if hasattr(cond.rfs[0], "__len__"):
                if ((str(RFs) not in cond.rfs)*(cond.rfs != '*')):  continue
            else:
                if ((str(RFs) != cond.rfs)*(cond.rfs != '*')):      continue
            if (cond.rfs != '*')*hasattr(cond.ef, "__len__"):
                #print(cond.ef)
                if ((cond.rfs != '*')*(EF not in cond.ef)):         continue
            else:
                if ((EF != cond.ef)*(cond.ef != '*')):              continue
            
            if ((c != cond.c)*(cond.c != '*')):                     continue
            if ((delta != cond.delta)*(cond.delta != '*')):         continue

            #legend_str = ("Ks={!s},"
            #            "RFs={!s},"
            #            "EF={:d},"
            #            "c={:.2f},"
            #            "delta={:.2f},"
            #            "e={:.0e}").format(*params)
            legend_str = (  "Ks={!s},"
                            "RFs={!s},"
                            "EF={:d},"
                            "E[nbl.]={:.0e},"
                            "pi_b={:.1e}").format(  params[0], 
                                                    params[1], 
                                                    params[2], 
                                                    np.mean(nblocks),
                                                    iid_per)

            if (cond.type == '*'):
                # MIB PLOT
                mibline = p.add_data(plot_name=plot_name[0],label=legend_str,type='mib',
                        x=overheads, y=avg_pers[:,0])
                # LIB PLOT
                p.add_data(plot_name=plot_name[0],label=legend_str,type='lib',
                        x=overheads, y=avg_pers[:,1],
                        color=mibline.get_color())
            else:
                if (cond.type == 'lib'):
                    p.add_data(plot_name=plot_name[0],label=legend_str,type='lib',
                        x=overheads, y=avg_pers[:,1])
                else:
                    p.add_data(plot_name=plot_name[0],label=legend_str,type='mib',
                        x=overheads, y=avg_pers[:,0])

            p.automaticYScale = True
            # nblocks
            if nPlots>1:
                p.add_data( plot_name=plot_name[1],label=legend_str, 
                            x=overheads, y=nblocks)
            # ripples
            if nPlots>2:
                p.add_data( plot_name=plot_name[2],label=legend_str,
                            x=overheads, y=avg_ripples)
            # drops
            if nPlots>3:
                p.add_data(plot_name=plot_name[3],label=legend_str,
                    x=overheads, y=avg_drop_rates)

            print(p.describe_plot(plot_name[0])+ ':' + legend_str)
        if nPlots>0:
            save_plot_png(p.get_plot(plot_name[0]),datestr + '_' + p.describe_plot(plot_name[0]))
        if nPlots>1:
            save_plot_png(p.get_plot(plot_name[1]),datestr + '_' + p.describe_plot(plot_name[1]))

    #plt.show()
