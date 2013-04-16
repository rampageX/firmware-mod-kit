#!/usr/bin/env python
# A hacky extraction utility for extracting the contents of BFF volume entries.
# It can't parse a BFF file itself, but expects the BFF volume entry to already 
# be extracted to a file; it then extracts the original file from the volume entry 
# file. Thus, it is best used with binwalk.

import os
import sys
import struct 
import subprocess

## {{{ http://code.activestate.com/recipes/82465/ (r4)
def _mkdir(newdir):
    """works the way a good mkdir should :)
        - already exists, silently complete
        - regular file in the way, raise an exception
        - parent directory(ies) does not exist, make them as well
    """
    if os.path.isdir(newdir):
        pass
    elif os.path.isfile(newdir):
        raise OSError("a file with the same name as the desired " \
                      "dir, '%s', already exists." % newdir)
    else:
        head, tail = os.path.split(newdir)
        if head and not os.path.isdir(head):
            _mkdir(head)
        #print "_mkdir %s" % repr(newdir)
        if tail:
            os.mkdir(newdir)
## end of http://code.activestate.com/recipes/82465/ }}}

HUFFMAN_MAGIC = 0xEA6C
MAGICS = [0xEA6B, HUFFMAN_MAGIC, 0xEA6D]
HEADER_SIZE = 64
POST_HEADER_SIZE = 40

script_path = os.path.dirname(os.path.realpath(__file__))

try:
	fd = open(sys.argv[1], 'rb')
except:
	print "Usage: %s <BFF volume entry file>" % sys.argv[0]
	sys.exit(1)

header = fd.read(HEADER_SIZE)

magic = struct.unpack("<H", header[2:4])[0]
file_size = struct.unpack("<L", header[56:60])[0]

if magic not in MAGICS:
	print "Unrecognized magic bytes! Quitting."
	sys.exit(1)

filename = ''
while True:
	byte = fd.read(1)
	if not byte or byte == '\x00':
		break
	else:
		filename += byte

filename_len = len(filename)
offset = HEADER_SIZE + POST_HEADER_SIZE + filename_len + (8 - (filename_len % 8))

if '..' in filename:
	print "Dangerous file path '%s'! Quitting." % filename
	sys.exit(1)

print "Extracting '%s'" % filename
_mkdir('./' + os.path.dirname(filename))
if file_size:
	fd.seek(offset)
	file_data = fd.read(file_size)
	fd.close()

	if len(file_data) != file_size:
		print "Warning: EOF encountered before the expected file size was reached!"

	if magic == HUFFMAN_MAGIC:
		fpout = open(filename + '.packed', 'wb')
	else:
		fpout = open(filename, 'wb')
	fpout.write(file_data)
	fpout.close()

	if magic == HUFFMAN_MAGIC:
		try:
			# Many thanks to Philipp for patching the huffman decoder to work!
			subprocess.call([script_path + "/bff_huffman_decompress", filename + '.packed', filename])
			os.remove(filename + '.packed')
		except:
			pass
else:
	_mkdir('./' + filename)

