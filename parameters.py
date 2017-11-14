class parameters:
    def __init__(self,params):
        all_good = ('ks' in params)
        all_good *= ('rfs' in params)
        all_good *= ('ef' in params)
        all_good *= ('c' in params)
        all_good *= ('delta' in params)
        all_good *= ('type' in params)
        if (all_good == False):
            print('Error: ks, rfs, ef, c, delta, type are all necessary parameters.\nFill with * for any.')
            exit()
        self.__ks = params['ks']
        self.__rfs = params['rfs']
        self.__rfind = 0
        self.__ef = params['ef']
        self.__c = params['c']
        self.__delta = params['delta']
        self.__type = params['type']
        #if ('overhead' in params and avg_per in params and avg_bad_run in params):
            
    @property
    def ks(self):
        if self.__ks == '*':
            return '*'
        else:
            return '('+str(self.__ks[0])+', '+str(self.__ks[1]) + ')'
    @ks.setter
    def ks(self,value):
        self.__ks = value
    @property
    def rfs(self):
        if self.__rfs == '*':
            return '*'
        else:
            rfs = self.__rfs
            s = []
            if hasattr(rfs[0], "__len__"):   # more than 1 condition for rfs [[]]
                for rfi in self.__rfs:
                    s.append('('+str(rfi[0])+', '+str(rfi[1]) + ')')
            else:
                s = '('+str(self.__rfs[0])+', '+str(self.__rfs[1]) + ')'

            return s
    @rfs.setter
    def rfs(self,value):
        self.__rfs = value
    @property
    def ef(self):
        return self.__ef
    @ef.setter
    def ef(self,value):
        self.__ef = value
    @property
    def c(self):
        return self.__c
    @c.setter
    def c(self,value):
        self.__c = value
    @property
    def delta(self):
        return self.__delta
    @delta.setter
    def delta(self,value):
        self.__delta = value
    @property
    def type(self):
        return self.__type
    @type.setter
    def type(self,value):
        self.__type = value

    def __ks2str(self):
        if (self.__ks != '*'):
            s = 'ks_'
            for i in range(len(self.__ks)-1):
                s += '{:d}-'.format(self.__ks[i])
            s += '{:d}'.format(self.__ks[-1])
            return s
        else: return ''
    def __rfs2str(self):
        if (self.__rfs != '*'):
            s = 'rfs_'
            if hasattr(self.__rfs[0], "__len__"):
                for rfi in self.__rfs:
                    for i in range(len(rfi)-1):
                        s += '{:d}-'.format(rfi[i])
                    s += '{:d}'.format(rfi[-1])+'_'
                s = s[0:len(s)-1]
            else:
                for i in range(len(self.__rfs)-1):
                    s += '{:d}-'.format(self.__rfs[i])
                s += '{:d}'.format(self.__rfs[-1])
            
            return s
        else: return ''
    def __ef2str(self):
        if (self.__ef != '*'):
            s = 'ef_'
            if hasattr(self.__ef, "__len__"):
                for ef in self.__ef:
                    s += str(ef)+'_'
                s = s[0:len(s)-1]
            else:
                s += str(self.__ef)
            return s
        else: return ''
    def __c2str(self):
        if (self.__c != '*'):
            return 'c_' + str(self.__c)
        else: return ''
    def __delta2str(self):
        if (self.__delta != '*'):
            return 'ef_' + str(self.__delta)
        else: return ''
    def __type2str(self):
        if (self.__type != '*'):
            return 'type_' + str(self.__type)
        else: return ''

    @property
    def toStr(self):
        cond = [self.__ks!='*', 
                self.__rfs!='*',
                self.__ef!='*',
                self.__c!='*',
                self.__delta!='*',
                self.__type!='*']
        values = [  self.__ks2str(),
                    self.__rfs2str(),
                    self.__ef2str(),
                    self.__c2str(),
                    self.__delta2str(),
                    self.__type2str()]
        n = sum([1 for i in range(len(cond)) if cond[i] == True])
        if n<1:
            return 'all'
        else:
            values = [values[i] for i in range(len(cond)) if cond[i] == True]
            s = ''
            for i in range(n-1):
                s += values[i] + '_'
            s += values[-1]
            return s
        
    
    def toString(self):
        return self.toStr

if __name__ == '__main__':
    print('parameters.py usage:')
    print('Constructor:')
    print('     parameters({\'ks\':[ks1_val, ks2_val],')
    print('                 \'rfs\':[rf1_val, rf2_val],')
    print('                 \'ef\':ef_val,')
    print('                 \'c\':c_val,')
    print('                 \'delta\':delta_val})')
    print('Properties:      ks, rfs, ef, c, delta, toStr')
    print('Methods:         toString()')
    print('\nExample of creation and toString():')
    ps = parameters({   'ks':[100,900],
                        'rfs':[3,1],
                        'ef':4,
                        'c':0.1,
                        'delta':0.5})
    ps.c = 0.14
    print(ps.toString())
