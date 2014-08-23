#!/usr/bin/python

import matplotlib.pyplot as pl
import numpy as np
import sys

signal_labels = {
    'o': 'offset',
    'bat_v': 'battery voltage (mV)',
    'pv_i': 'input current (mA)',
    'in_v': 'input voltage (mV)',
    'power': 'power (mW)',
}

signals = set()
data = []
cur = {}
for l in open(sys.argv[1]):
    if l.startswith('iterate'):
        data.append(cur)
        cur =  {}
        continue

    k,_,v = l.partition('=')
    if v is '':
        continue
    v = v.split()[0]
    signals.add(k)
    try:
        cur[k] = float(v)
    except:
        #print l
        pass

have = set(signal_labels.keys()).intersection(signals)
for i,k in enumerate(have):
    data[k] = np.array(data[k])
    label = signals[k]
    pl.subplot(len(have), 1, i)
    pl.plot(data[k][:,0], data[k][:,1], '-o', label=label)
    pl.ylabel(label)

pl.show()
