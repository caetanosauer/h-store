#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os, sys, subprocess, string

numHardwareThreads = 4
platform = os.uname()[0]
if platform == "Darwin":
    numHardwareThreads = 0
    output = subprocess.getstatusoutput("sysctl hw.ncpu")
    # numHardwareThreads = int(string.strip(string.split(output[1])[1])) # only works in python2
    numHardwareThreads = int(output[1].split()[1].strip())
elif platform == "Linux":
    numHardwareThreads = 0
    for line in open('/proc/cpuinfo').readlines():
        # name_value = map(string.strip, string.split(line, ':', 1)) # only works in python2
        name_value = [x.strip() for x in line.split(':', 1)]
        if len(name_value) != 2:
            continue
        name,value = name_value
        if name == "processor":
            numHardwareThreads = numHardwareThreads + 1
print(numHardwareThreads)
