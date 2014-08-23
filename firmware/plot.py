#!/usr/bin/python

import matplotlib.pyplot as pl
import numpy as np
import numpy.ma as ma
import sys

signal_labels = {
    'o': 'offset',
    'bat_v': 'battery voltage (mV)',
    'pv_i': 'input current (mA)',
    'in_v': 'input voltage (mV)',
    'power': 'power (mW)',
}

data = {}
idx = 0
for l in open(sys.argv[1]):
    if l.startswith('iterate'):
        idx += 1
        continue

    k,_,v = l.partition('=')
    if v is '':
        continue
    v = v.split()[0]
    try:
        v = float(v)
        data.setdefault(k, []).append((idx, v))
    except:
        #print l
        pass

for i,k in enumerate(data):
    data[k] = np.array(data[k])
    label = signal_labels.get(k, k)
    pl.subplot(len(data), 1, i)
    pl.plot(data[k][:,0], data[k][:,1], '+', label=label)
    pl.ylabel(label)

pl.show()
