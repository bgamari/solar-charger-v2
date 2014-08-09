#!/usr/bin/python

import matplotlib.pyplot as pl
import numpy as np
import sys

data = {}
for l in open(sys.argv[1]):
    k,_,v = l.partition('=')
    if v is '':
        continue
    v = v.split()[0]
    try:
        data.setdefault(k, []).append(float(v))   
    except:
        print l
        pass

pl.subplot(411)
pl.plot(data['o'], label='offset')
pl.ylabel('offset')

pl.subplot(412)
pl.plot(np.array(data['bat_v']) / 1000.)
pl.ylabel('battery voltage (V)')

pl.subplot(413)
pl.plot(data['pv_i'])
pl.ylabel('current (mA)')

pl.subplot(414)
pl.plot(data['power'])
pl.ylabel('power (mW)')

pl.show()
