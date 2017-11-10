import matplotlib.pyplot as plt
import numpy as np

from uep import *

if __name__ == "__main__":
    data = load_data_prefix("uep_iid/uep_vs_oh_iid_")
    print("Using {:d} data packs".format(len(data)))

    # Old data is always error-free
    for d in data:
        if 'iid_per' not in d:
            d['iid_per'] = 0

    param_set = sorted(set((tuple(d['Ks']),
                            tuple(d['RFs']),
                            d['EF'],
                            d['c'],
                            d['delta'],
                            d['iid_per']) for d in data))

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
        for i, oh in enumerate(overheads):
            avg_counters = [AverageCounter() for k in Ks]
            for d in data_same_pars:
                for l, d_oh in enumerate(d['overheads']):
                    if d_oh != oh: continue
                    for j,k in enumerate(Ks):
                        avg_counters[j].add(d['avg_pers'][l][j],
                                            d['nblocks'])
            avg_pers[i,:] = [c.avg for c in avg_counters]
            nblocks[i] = avg_counters[0].total_weigth

        #if min(nblocks) < 10000: continue
        #if Ks[0] < 5000: continue
        if Ks != (100,900): continue
        if EF != 4: continue
        if RFs[0] not in [3,5,6]: continue

        legend_str = ("Ks={!s},"
                      "RFs={!s},"
                      "EF={:d},"
                      "c={:.3f},"
                      "delta={:.3f},"
                      "e={:.0e}").format(*params)

        plt.figure('per')
        plt.plot(overheads, avg_pers[:,0],
                 marker='.',
                 linestyle='-',
                 linewidth=1.5,
                 label="MIB " + legend_str)
        plt.plot(overheads, avg_pers[:,1],
                 marker='.',
                 linestyle='-',
                 linewidth=1.5,
                 label="LIB " + legend_str)

        plt.figure('nblocks')
        plt.plot(overheads, nblocks,
                 marker='.',
                 linestyle='-',
                 linewidth=1.5,
                 label=legend_str)

        the_oh_is = [i for i,oh in enumerate(overheads)
                     if math.isclose(oh, 0.24)]
        if len(the_oh_is) > 0:
            the_oh_i = the_oh_is[0]
            print("At overhead {:.2f}:".format(overheads[the_oh_i]))
            print(" " * 4 + legend_str, end="")
            print(" -> MIB={:e}, LIB={:e}".format(avg_pers[the_oh_i, 0],
                                                  avg_pers[the_oh_i, 1]))

    plt.figure('per')
    plt.gca().set_yscale('log')
    plt.ylim(1e-8, 1)
    plt.xlabel('Overhead')
    plt.ylabel('UEP PER')
    plt.legend()
    plt.grid()

    plt.savefig('uep_iid_per.pdf', format='pdf')

    plt.figure('nblocks')
    plt.xlabel('Overhead')
    plt.ylabel('nblocks')
    plt.legend()
    plt.grid()

    plt.savefig('uep_iid_nblocks.pdf', format='pdf')

    plt.show()
