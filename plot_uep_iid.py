import matplotlib.pyplot as plt
import numpy as np

from uep import *

if __name__ == "__main__":
    data = load_data_prefix("uep_iid/uep_vs_oh_iid_")
    print("Using {:d} data packs".format(len(data)))

    Ks_set = sorted(set(tuple(d['Ks']) for d in data))

    for Ks in Ks_set:
        data_sameKs = [d for d in data if tuple(d['Ks']) == Ks]

        overheads = sorted(set(o for d in data_sameKs for o in d['overheads']))
        avg_pers = np.zeros((len(overheads), len(Ks)))
        nblocks = np.zeros(len(overheads))
        for i, oh in enumerate(overheads):
            avg_counters = [AverageCounter() for k in Ks]
            for d in data_sameKs:
                for l, d_oh in enumerate(d['overheads']):
                    if d_oh != oh: continue
                    for j,k in enumerate(Ks):
                        avg_counters[j].add(d['avg_pers'][l][j],
                                            d['nblocks'])
            avg_pers[i,:] = [c.avg for c in avg_counters]
            nblocks[i] = avg_counters[0].total_weigth

        plt.figure('per')
        plt.plot(overheads, avg_pers[:,0],
                 marker='.',
                 linestyle='-',
                 linewidth=1.5,
                 label="MIB Ks={!s}".format(Ks))
        plt.plot(overheads, avg_pers[:,1],
                 marker='.',
                 linestyle='-',
                 linewidth=1.5,
                 label="LIB Ks={!s}".format(Ks))

        plt.figure('nblocks')
        plt.plot(overheads, nblocks,
                 marker='.',
                 linestyle='-',
                 linewidth=1.5,
                 label="Ks={!s}".format(Ks))

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
