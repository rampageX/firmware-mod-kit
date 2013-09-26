#!/usr/bin/env python
# Utility for extracting WDK "filesystems", such as those found in the DIR-100.

import os
import sys
import struct
import shutil
import subprocess

# Default to big endian
ENDIANESS = "big"

# Unpack size bytes of data starting at offset
def unpack(data, offset, size):
	sizes = {
		2 : "H",
		4 : "L",
	}

	if ENDIANESS == "big":
		fmt = ">" + sizes[size]
	else:
		fmt = "<" + sizes[size]

	return struct.unpack(fmt, data[offset:offset+size])[0]

# Get the next file entry in the WDK header table
def get_entry(data):
	file_name = ""
	file_size = unpack(data, 2, 2)
	
	i = 4
	while data[i] != "\x00":
		file_name += data[i]
		i += 1

	return (file_size, file_name)

def extract(fs_image_file):	
	fs_img = open(fs_image_file, 'rb').read()
	if len(fs_img) < 32:
		print "WDK image too small!"
		sys.exit(1)
	
	entries = {}
	order = []
	# This offset appears to be a fixed value
	entry_offset = 0x1A
	data_offset = unpack(fs_img, 16, 2)
	# The offset of the version string appears to be fixed as well
	fs_version = fs_img[18:25]
	
	if fs_version != 'WDK 2.0':
		print "Unknown WDK image:", fs_version
		sys.exit(1)
	else:
		os.mkdir("wdk-root")
		os.chdir("wdk-root")

	# Process all the file entries in the header	
	while entry_offset < data_offset:
		(file_size, file_name) = get_entry(fs_img[entry_offset:])
		entries[file_name] = file_size
		order.append(file_name)
	
		entry_offset += 4 + len(file_name) + 1
		# File entries are padded to be a multiple of 2 in length
		if (entry_offset % 2):
			entry_offset += 1
	
	total_size = 0
	for file_name in order:
		size = entries[file_name]
		file_data = fs_img[data_offset+total_size:data_offset+total_size+size]
		total_size += size
		out_file_name = file_name + '.7z'
	
		# This assumes there is only one sub directory in any given file name
		if os.path.basename(file_name) != file_name and not os.path.exists(os.path.dirname(file_name)):
			os.mkdir(os.path.dirname(file_name))
	
		open(out_file_name, 'wb').write(file_data)
		
		# Try to decompress the file
		subprocess.call(["p7zip", "-d", out_file_name])
		if os.path.exists(out_file_name):
			shutil.move(out_file_name, out_file_name[:-3])
	
		print file_name

def main():
	# First command line argument: WDK image file
	# Second command line argument: endianess (default: big)
	try:
		fs_image_file = sys.argv[1]
	except:
		print "Usage: %s <WDK image file> [big | little]" % sys.argv[0]
		sys.exit(1)
	try:
		ENDIANESS = sys.argv[2]
	except:
		pass

	extract(fs_image_file)


if __name__ == '__main__':
	main()

