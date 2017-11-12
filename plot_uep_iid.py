import datetime

import matplotlib.pyplot as plt
import numpy as np

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
    data = load_data_prefix("uep_iid_mpfix/uep_vs_oh_iid_")
    print("Using {:d} data packs".format(len(data)))

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

    param_filter = pf_all

    p = plots()
    p.automaticXScale = True
    #p.automaticXScale = [0,0.3]
    p.add_plot(plot_name='per',xlabel='Overhead',ylabel='PER',logy=True)
    p.add_plot(plot_name='nblocks',xlabel='Overhead',ylabel='nblocks',logy=False)
    p.add_plot(plot_name='ripple',xlabel='Overhead',ylabel='ripple',logy=False)
    p.add_plot(plot_name='drop_rate',xlabel='Overhead',ylabel='drop_rate',logy=False)

    for params in param_set:
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

        if not param_filter(*params): continue

        legend_str = ("Ks={!s},"
                      "RFs={!s},"
                      "EF={:d},"
                      "c={:.2f},"
                      "delta={:.2f},"
                      "e={:.0e}").format(*params)

        mibline = p.add_data(plot_name='per',label=legend_str,type='mib',
                           x=overheads, y=avg_pers[:,0])
        if len(Ks) > 1:
            p.add_data(plot_name='per',label=legend_str,type='lib',
                       x=overheads, y=avg_pers[:,1],
                       color=mibline.get_color())

        p.add_data(plot_name='nblocks',label=legend_str,
                   x=overheads, y=nblocks)

        p.add_data(plot_name='ripple',label=legend_str,
                   x=overheads, y=avg_ripples)

        p.add_data(plot_name='drop_rate',label=legend_str,
                   x=overheads, y=avg_drop_rates)

        #the_oh_is = [i for i,oh in enumerate(overheads)
        #             if math.isclose(oh, 0.24)]
        #if len(the_oh_is) > 0:
        #    the_oh_i = the_oh_is[0]
        #    print("At overhead {:.2f}:".format(overheads[the_oh_i]))
        #    print(" " * 4 + legend_str, end="")
        #    print(" -> MIB={:e}, LIB={:e}".format(avg_pers[the_oh_i, 0],
        #                                          avg_pers[the_oh_i, 1]))

    datestr = datetime.datetime.now().strftime("%Y-%m-%d_%H-%M-%S")
    #save_plot_png(p.get_plot('per'),'iid '+p.describe_plot('per')+datestr)
    #save_plot_png(p.get_plot('nblocks'),'iid '+p.describe_plot('nblocks')+datestr)

    plt.show()
