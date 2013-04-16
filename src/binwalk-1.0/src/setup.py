#!/usr/bin/env python
from os import listdir, path
from distutils.core import setup

# Generate a new magic file from the files in the magic directory
print "generating binwalk magic file"
magic_files = listdir("magic")
magic_files.sort()
fd = open("binwalk/magic/binwalk", "wb")
for magic in magic_files:
	fpath = path.join("magic", magic)
	if path.isfile(fpath):
		fd.write(open(fpath).read())
fd.close()

# The data files to install along with the binwalk module
install_data_files = ["magic/*", "config/*"]

# Install the binwalk module, script and support files
setup(	name = "binwalk",
	version = "1.0",
	description = "Firmware analysis tool",
	author = "Craig Heffner",
	url = "http://binwalk.googlecode.com",
	
	packages = ["binwalk"],
	package_data = {"binwalk" : install_data_files},
	scripts = ["bin/binwalk"],
)
