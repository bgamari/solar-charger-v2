#!/usr/bin/python

import matplotlib.pyplot as pl
import matplotlib.dates as mdate
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
for l in open(sys.argv[1]):
    if l[0] != '<':
        continue
    t,_,l = l[1:].partition('\t')
    t = int(t)
    k,_,v = l.partition('=')
    if v is '':
        continue
    v = v.split()[0]
    try:
        v = float(v)
        data.setdefault(k, []).append((t, v))
    except:
        #print l
        pass

for i,k in enumerate(data):
    data[k] = np.array(data[k])
    label = signal_labels.get(k, k)
    pl.subplot(len(data), 1, i)
    t = mdate.epoch2num(data[k][:,0])
    xfmt = mdate.DateFormatter('%d-%m-%y %H:%M:%S')
    pl.plot(t, data[k][:,1], '+', label=label)
    pl.gca().xaxis.set_major_formatter(xfmt)
    pl.gcf().autofmt_xdate()
    pl.ylabel(label)

pl.show()
