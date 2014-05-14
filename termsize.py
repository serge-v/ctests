#!/usr/bin/python

import termios
import struct
import fcntl
import sys

row, col, xpix, ypix = 0, 0, 0, 0
winsize = struct.pack("HHHH", row, col, xpix, ypix)
print len(winsize)
cr = struct.unpack('hh', fcntl.ioctl(sys.stdout, termios.TIOCGWINSZ, '1234'))
print cr
