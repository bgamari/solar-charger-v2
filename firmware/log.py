#!/usr/bin/python

import sys
import threading
import time
import argparse
import serial

parser = argparse.ArgumentParser()

parser.add_argument('-d', '--device', help='Device node', required=True)
parser.add_argument('-b', '--baud', help='Baud rate', default=9600, type=int)
parser.add_argument('-o', '--output', help='Log output', default='-')
parser.add_argument('-a', '--append', help='Append to log', action='store_true')
args = parser.parse_args()

s = serial.Serial(args.device, baudrate=args.baud, timeout=None)

output = None
if args.output == '-':
    output = sys.stdout
else:
    mode = 'w+' if args.append else 'w'
    output = open(args.output, mode)

def log(direction, line):
    t = time.time()
    output.write('%s %10d\t%s\n' % (direction, t, line))
    output.flush()

def reader():
    while True:
        c = sys.stdin.read(1)
        log('>', c)
        s.write(c)
    
thrd = threading.Thread(target=reader)
thrd.daemon = True
thrd.start()

def read_until(file, delim):
    accum = ''
    while True:
        c = file.read(1)
        if c == delim:
            return accum
        else:
            accum += c

while True:
    l = read_until(s, '\n')
    log('<', l)

