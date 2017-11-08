import random
class iid_ch:
    def __init__(self, e):
        self.__e = e
        self.__piB = e
        self.__piG = 1-e
        if (random.random()<e):
            self.state = False # BAD channel
        else:
            self.state = True # GOOD channel

    @property
    def piG(self):
        return self.__piG
    @property
    def piB(self):
        return self.__piB
    @property
    def e(self):
        return self.__e

    def good_slot(self):
        if (self.state): # GOOD channel
            if (random.random()<self.e):
                self.state = False
            else:
                self.state = True
            return True
        else: # BAD channel
            if (random.random()<self.e):
                self.state = False
            else:
                self.state = True
            return False
    def toStr(self):
        s = 'I.i.d. channel parameters:\n'
        s += 'e = '+'{:.2e}'.format(self.e) + '\n'
        s += 'pi_G = '+'{:.2e}'.format(self.piG)+', pi_B = '+'{:.2e}'.format(self.piB)+'\n'
        #print(s)
        return s

class markov_ch:
    def __init__(self,p,q):
        self.__p = p
        self.__q = q
        self.__piG = q / (p+q)
        self.__piB = 1 - self.piG        
        if (random.random()<self.piB):
            self.state = False # BAD channel
        else:
            self.state = True # GOOD channel

    @property
    def piG(self):
        return self.__piG
    @property
    def piB(self):
        return self.__piB
    @property
    def p(self):
        return self.__p
    @property
    def q(self):
        return self.__q
    @property
    def toStr(self):
        return self.toString()

    def good_slot(self):
        if (self.state): # GOOD channel
            if (random.random()<self.p):
                self.state = False
            else:
                self.state = True
            return True
        else: # BAD channel
            if (random.random()<self.q):
                self.state = True
            else:
                self.state = False
            return False
    def toString(self):
        s = 'Markov channel parameters:\n'
        s += 'p = '+'{:.2e}'.format(self.p)+', q = '+'{:.2e}'.format(self.q) + '\n'
        s += 'pi_G = '+'{:.2e}'.format(self.piG)+', pi_B = '+'{:.2e}'.format(self.piB)+'\n'
        #print(s)
        return s


if __name__ == '__main__':
    EnG = 49
    EnB = 1
    e = 1/10
    n = 100
    out = 'channel.py -- TEST\nn='+str(n)

    iid_ch = iid_ch(e)
    out += '\n' + iid_ch.toStr()
    for i in range(0,n):
        if (iid_ch.good_slot()):
            out += 'G'
        else:
            out += 'B '

    m_ch = markov_ch(1/EnG,1/EnB)
    out += '\n' + m_ch.toStr()
    for i in range(0,n):
        if (m_ch.good_slot()):
            out += 'G'
        else:
            out += 'B '

    print(out)