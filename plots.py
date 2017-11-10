import math

import matplotlib.pyplot as plt

class plots:

    def __init__(self):
        self.__plots = []
        self.__plot_ind = 0
        self.__nplots = 0
        self.minExp = -16
        self.__minY = math.pow(10,self.minExp)
        self.automaticXScale = True

    def __check_name(self,name):
        names = [self.__plots[i]['name'] for i in range(self.__nplots)]
        if (name in names):
            self.__plot_ind = names.index(name)
            return True
        else:
            return False

    @property
    def toStr(self):
        return str(self)

    def __str__(self):
        s = 'plots:\n'
        names = [self.__plots[i]['name'] for i in range(self.__nplots)]
        for i in range(len(names)):
            s += str(i) + ' : ' + names[i] + '\n'
        return s

    def add_plot(self, **params): # params: { plot_name, xlabel, ylabel, logy}
        plot_name = ''
        if ('plot_name' in params):
            plot_name = params['plot_name']
        else:
            print('Error: no field \'plot_name\' specified')
            exit()
        xlabel = ''
        ylabel = ''
        if ('xlabel' in params):
            xlabel = params['xlabel']
        else:
            print('Error: no field \'xlabel\' specified')
            exit()
        if ('ylabel' in params):
            ylabel = params['ylabel']
        else:
            print('Error: no field \'ylabel\' specified')
            exit()
        logy = False
        if ('logy' in params):
            if (params['logy']==True):
                logy = True


        self.__plots.append({   'name':plot_name,
                                'xmin': 0,
                                'xmax': 0,
                                'ymin': 0.1*logy,
                                'ymax': self.__minY,
                                'xlabel': xlabel,
                                'ylabel': ylabel,
                                'logy': logy,
                                'obj':plt.figure(plot_name)})
        self.__nplots += 1
        plt.xlabel(xlabel)
        plt.ylabel(ylabel)
        if (logy):
            plt.gca().set_yscale('log')

    def add_data(self, **params): #params = {'plot_name', 'x', 'y', 'label', 'type' in ['mib', 'lib']}
        p = self.__plots
        if ('plot_name' in params):
            plt_name = params['plot_name']
            if (self.__check_name(plt_name)):
                plt.figure(plt_name)
            else:
                print('Plot not yet created. Please use add_plot first')
                exit()
        else:
            print('Error: \'plot_name\' is necessary')
            exit()
        if ((('x' in params) * ('y' in params) * (len(params['x'])==len(params['y'])))==False):
            print('Error: x, y array are necessary and they must be of the same length')
            exit()
        if ('label' in params == False):
            print('Error: label is a necessary fields')
            exit()
        this_type = ''
        if ('type' in params):
            this_type = params['type']

        p = p[self.__plot_ind]

        y = params['y']
        x = params['x']

        if (p['logy'] == True):
            if (min(y) <= 0):
                print('Error: impossible to plot negative value in log scale.')
                print('approximating 0 with {:.1e}'.format(self.__minY))
                y_positive = [yi for yi in y if yi > 0]
                #assert(all(y_positive[i]>0 for i in range(len(y_positive))))
                #print('y_min = {:.1e}, p[\'y_min\'] = {:.1e}'.format(min(y_positive),p['ymin']))
                p['ymin'] = min([p['ymin'],min(y_positive)])
                y = [max(self.__minY, yi) for yi in y]
            else:
                p['ymin'] = min([p['ymin'],min(y)])
        else:
            p['ymin'] = min([p['ymin'],min(y)])

        if (self.automaticXScale == True):
            p['xmin'] = min([p['xmin'],min(x)])
            p['xmax'] = max([p['xmax'],max(x)])
        else:
            p['xmin'] = self.automaticXScale[0]
            p['xmax'] = self.automaticXScale[1]

        p['ymax'] = max([p['ymax'],max(y)])
        ls = '-'
        if (this_type == 'lib'): ls += '-'

        plt.plot(x, y, marker='.',linewidth=0.5,linestyle=ls,label=(this_type + params['label']))
        plt.xlim(p['xmin'], p['xmax']+math.fabs(p['xmax'])/100)
        plt.ylim(p['ymin'], p['ymax'] + math.fabs(p['ymax'])/100)
        plt.grid()
        plt.legend()

    def get_plot_obj(self, name):
        if (name in ['*', '']):
            return self.__plots
        if (self.__check_name(name)==True):
            return self.__plots[self.__plot_ind]
        else:
            print('No plot named '+name)
    def get_plot(self, name):
        if (name in ['*', '']):
            return self.__plots
        if (self.__check_name(name)==True):
            return self.__plots[self.__plot_ind]['obj']
        else:
            print('No plot named '+name)

    def show_plot(self, name):
        if (self.__check_name(name)==True):
            plt.show()
        else:
            print('No plot named '+name)

    def describe_plot(self, name):
        if (self.__check_name(name)==True):
            p = self.__plots[self.__plot_ind]
            return p['ylabel'] + ' VS ' + p['xlabel'] + ' '
        else:
            print('No plot named '+name)

if __name__ == '__main__':
    import numpy as np
    p = plots()
    p.add_plot({'plot_name':'first','xlabel':'Overhead', 'ylabel':'Packet error rate', 'logy':True})
    p.add_data({'plot_name':'first','label':'label1', 'type':'mib',
                'x':np.array([0.1, 0.2, 0.3]),
                'y':np.array([10, 103, 108])})
    p.add_data({'plot_name':'first','label':'label1', 'type':'lib',
                'x':np.array([0.1, 0.2, 0.3]),
                'y':np.array([1, 100, 104])})
    print('Plotting:' + p.describe_plot('first'))
    p.show_plot('first')
