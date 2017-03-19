#!/usr/bin/env python
import sys

name = sys.argv[1]
fields = name.split('_')
out = ''.join([str.upper(field[:1]) + field[1:] for field in fields])
sys.stdout.write(out)
#for field in fields:
#    print str.upper(field[:1]) + field[1:]
