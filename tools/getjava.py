#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os, sys, re, string, subprocess

jvmVersion = None
output = subprocess.getoutput("java -version")

regex = re.compile("(?:java|openjdk) version \"(1\.\d)\..*?\"", re.IGNORECASE)
m = regex.search(output)
if m: jvmVersion = m.group(1)

assert not jvmVersion is None
print(jvmVersion)
