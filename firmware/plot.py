#!/usr/bin/python

import matplotlib.pyplot as pl
import numpy as np
import sys

data = {}
i = 0
for l in open(sys.argv[1]):
    if l.startswith('iterate'):
        i += 1
        continue

    k,_,v = l.partition('=')
    if v is '':
        continue
    v = v.split()[0]
    try:
        data.setdefault(k, []).append((i,float(v)))
    except:
        #print l
        pass

signals = {
    'o': 'offset',
    'bat_v': 'battery voltage (mV)',
    'pv_i': 'input current (mA)',
    'power': 'power (mW)',
}

for i,k in enumerate(data):
    data[k] = np.array(data[k])

    if k not in signals:
        continue
    label = signals[k]
    pl.subplot(len(data), 1, i)
    pl.plot(data[k][:,0], data[k][:,1], label=label)
    pl.ylabel(label)

pl.show()
