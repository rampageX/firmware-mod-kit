# Common functions.
import os
import re

def file_size(filename):
	'''
	Obtains the size of a given file.

	@filename - Path to the file.

	Returns the size of the file.
	'''
	# Using open/lseek works on both regular files and block devices
	fd = os.open(filename, os.O_RDONLY)
	try:
		return os.lseek(fd, 0, os.SEEK_END)
	except Exception, e:
		raise Exception("file_size failed to obtain the size of '%s': %s" % (filename, str(e)))
	finally:
		os.close(fd)

def str2int(string):
	'''
	Attempts to convert string to a base 10 integer; if that fails, then base 16.

	@string - String to convert to an integer.

	Returns the integer value on success.
	Throws an exception if the string cannot be converted into either a base 10 or base 16 integer value.
	'''
	try:
		return int(string)
	except:
		return int(string, 16)

def strip_quoted_strings(string):
	'''
	Strips out data in between double quotes.
	
	@string - String to strip.

	Returns a sanitized string.
	'''
	# This regex removes all quoted data from string.
	# Note that this removes everything in between the first and last double quote.
	# This is intentional, as printed (and quoted) strings from a target file may contain 
	# double quotes, and this function should ignore those. However, it also means that any 
	# data between two quoted strings (ex: '"quote 1" you won't see me "quote 2"') will also be stripped.
	return re.sub(r'\"(.*)\"', "", string)

def get_quoted_strings(string):
	'''
	Returns a string comprised of all data in between double quotes.

	@string - String to get quoted data from.

	Returns a string of quoted data on success.
	Returns a blank string if no quoted data is present.
	'''
	try:
		# This regex grabs all quoted data from string.
		# Note that this gets everything in between the first and last double quote.
		# This is intentional, as printed (and quoted) strings from a target file may contain 
		# double quotes, and this function should ignore those. However, it also means that any 
		# data between two quoted strings (ex: '"quote 1" non-quoted data "quote 2"') will also be included.
		return re.findall(r'\"(.*)\"', string)[0]
	except:
		return ''
