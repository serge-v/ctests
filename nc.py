#!/usr/bin/python

import sys
from socket import *

address = ('226.94.1.1', 50109)

s = socket(AF_INET, SOCK_DGRAM)

cmd = 'c'

if len(sys.argv) > 1:
   cmd = " ".join(sys.argv[1:])

print '>' + cmd
s.sendto(cmd, address)
recv_data, addr = s.recvfrom(2048)

print recv_data



