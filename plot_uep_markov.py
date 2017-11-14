import datetime

import matplotlib.pyplot as plt
import numpy as np

from plots import *
from uep import *
from utils import *

def pf_all(RFs, EF, c, delta, overhead, avg_per, avg_bad_run, Ks_frac):
    return True

if __name__ == "__main__":
    data = load_data_prefix("uep_markov_final/")

    git_sha1_set = sorted(set(d.get('git_sha1') or 'None' for d in data))
    print("Found {:d} commits:".format(len(git_sha1_set)))
    for s in git_sha1_set:
        print("  - " + s)

    # wanted_commits = [
    #     "921f5e0f5bf82591ec93f215cd490f8bdd9473fe",
    #     "a616ad6478ac1f80dc3f7983cbf5b4958bf9fcfb",
    # ]

    # data = [d for d in data if d.get('git_sha1') in wanted_commits]

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
    p.automaticXScale = True
    #p.automaticXScale = [0,0.3]
    p.add_plot(plot_name='per',xlabel='K',ylabel='PER',logy=True)
    p.add_plot(plot_name='nblocks',xlabel='K',ylabel='nblocks',logy=False)
    p.add_plot(plot_name='drop_rate',xlabel='K',ylabel='drop_rate',logy=False)

    for params in param_set:
        data_same_pars = [d for d in data if (tuple(d['RFs']),
                                              d['EF'],
                                              d['c'],
                                              d['delta'],
                                              d['overhead'],
                                              d['avg_per'],
                                              d['avg_bad_run'],
                                              tuple(d['Ks_frac'])) == params]
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
        avg_drop_rates = np.zeros(len(k_blocks))
        for i, k_block in enumerate(k_blocks):
            avg_counters = [AverageCounter() for k in Ks_frac]
            avg_drop = AverageCounter()
            for d in data_same_pars:
                for l, d_k_block in enumerate(d['k_blocks']):
                    if d_k_block != k_block: continue
                    for j, _ in enumerate(Ks_frac):
                        per = (d['error_counts'][l][j] /
                               (d['used_Ks'][l][j] * d['nblocks']))
                        avg_counters[j].add(per, d['nblocks'])
                    avg_drop.add(d['avg_drops'][l], d['nblocks'])
            avg_pers[i,:] = [c.avg for c in avg_counters]
            nblocks[i] = avg_counters[0].total_weigth
            avg_drop_rates[i] = avg_drop.avg

        if not param_filter(*params): continue

        legend_str = ("RFs={!s},"
                      "EF={:d},"
                      "c={:.2f},"
                      "delta={:.2f},"
                      "oh={:.2f},"
                      "avg_per={:.0e},"
                      "avg_bad_run={:.2f},"
                      "Ks_frac={!s}").format(*params)

        mibline = p.add_data(plot_name='per',label=legend_str,type='mib',
                           x=k_blocks, y=avg_pers[:,0])
        if len(Ks_frac) > 1:
            p.add_data(plot_name='per',label=legend_str,type='lib',
                       x=k_blocks, y=avg_pers[:,1],
                       color=mibline.get_color())
        plt.grid()

        p.add_data(plot_name='nblocks',label=legend_str,
                   x=k_blocks, y=nblocks)
        plt.autoscale(enable=True, axis='y', tight=False)

        p.add_data(plot_name='drop_rate',label=legend_str,
                   x=k_blocks, y=avg_drop_rates)
        plt.autoscale(enable=True, axis='y', tight=False)

        #the_oh_is = [i for i,oh in enumerate(overheads)
        #             if math.isclose(oh, 0.24)]
        #if len(the_oh_is) > 0:
        #    the_oh_i = the_oh_is[0]
        #    print("At overhead {:.2f}:".format(overheads[the_oh_i]))
        #    print(" " * 4 + legend_str, end="")
        #    print(" -> MIB={:e}, LIB={:e}".format(avg_pers[the_oh_i, 0],
        #                                          avg_pers[the_oh_i, 1]))

    # datestr = datetime.datetime.now().strftime("%Y-%m-%d_%H-%M-%S")
    # save_plot_png(p.get_plot('per'),'iid '+p.describe_plot('per')+datestr)
    # save_plot_png(p.get_plot('nblocks'),'iid '+p.describe_plot('nblocks')+datestr)

    plt.figure(p.get_plot('per'))
    plt.savefig('uep_markov_per.pdf', format='pdf')

    plt.show()
