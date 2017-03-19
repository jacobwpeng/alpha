#!/usr/bin/env python

import os
import sys

filename = sys.argv[1]

with open(filename, 'r') as fp:
    lines = fp.readlines()

guard = ''
toDelLines = []
for lineno, line in enumerate(lines):
    if '#ifndef __' in line:
        if guard == '':
            guard = line.split()[1]
            lines[lineno] = '#pragma once'
        else:
            toDelLines.append(lineno)
    elif guard != '' and guard in line:
        if line.startswith('#define {}'.format(guard)):
            lines[lineno] = os.linesep
        else:
            toDelLines.append(lineno)

outLines = []
for lineno, line in enumerate(lines):
    if lineno in toDelLines:
        continue
    outLines.append(line)

with open(filename, 'w') as fp:
    fp.write(''.join(outLines))
