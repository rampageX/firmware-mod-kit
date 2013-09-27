#!/usr/bin/env python

import os
import sys
import zlib

try:
	zlib_file = sys.argv[1]
except:
	print "Usage: %s <zlib compressed file>" % sys.argv[0]
	sys.exit(1)

plaintext_file = os.path.splitext(zlib_file)[0]

try:
	plaintext = zlib.decompress(open(zlib_file, 'rb').read())
	open(plaintext_file, 'wb').write(plaintext)
except Exception, e:
	print "Failed to decompress data:", str(e)
	sys.exit(1)
