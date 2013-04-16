#!/usr/bin/env python
# Generates LZMA signatures for each valid LZMA property in the properties list.


properties = [
	0x5D,
	0x01,
	0x02,
	0x03,
	0x04,
	0x09,
	0x0A,
	0x0B,
	0x0C,
	0x12,
	0x13,
	0x14,
	0x1B,
	0x1C,
	0x24,
	0x2D,
	0x2E,
	0x2F,
	0x30,
	0x31,
	0x36,
	0x37,
	0x38,
	0x39,
	0x3F,
	0x40,
	0x41,
	0x48,
	0x49,
	0x51,
	0x5A,
	0x5B,
	0x5C,
	0x5E,
	0x63,
	0x64,
	0x65,
	0x66,
	0x6C,
	0x6D,
	0x6E,
	0x75,
	0x76,
	0x7E,
	0x87,
	0x88,
	0x89,
	0x8A,
	0x8B,
	0x90,
	0x91,
	0x92,
	0x93,
	0x99,
	0x9A,
	0x9B,
	0xA2,
	0xA3,
	0xAB,
	0xB4,
	0xB5,
	0xB6,
	0xB7,
	0xB8,
	0xBD,
	0xBE,
	0xBF,
	0xC0,
	0xC6,
	0xC7,
	0xC8,
	0xCF,
	0xD0,
	0xD8,
]

common_properties = [0x5D, 0x6D]

dictionary_sizes = [
	65536,
	131072,
	262144,
	524288,
	1048576,
	2097152,
	4194304,
	8388608,
	16777216,
	33554432,
]

for fbyte in properties:
#	if fbyte not in common_properties:
#		fexclude = '{filter-exclude}'
#	else:
#		fexclude = ''
	fexclude = ''

	sig = '\n# ------------------------------------------------------------------\n'
	sig += '# Signature for LZMA compressed data with valid properties byte 0x%.2X\n' % fbyte
	sig += '# ------------------------------------------------------------------\n'
	sig += '0\t\tstring\t\\x%.2X\\x00\\x00\tLZMA compressed data, properties: 0x%.2X,%s\n' % (fbyte, fbyte, fexclude)

	sig += '\n# These are all the valid dictionary sizes supported by LZMA utils.\n'
	for i in range(0, len(dictionary_sizes)):
		if i < 6:
			indent = '\t\t'
		else:
			indent = '\t'

		if i == len(dictionary_sizes)-1:
			invalid = 'invalid'
		else:
			invalid = ''

		sig += '%s1%slelong\t!%d\t%s\n' % ('>'*(i+1), indent, dictionary_sizes[i], invalid)

	sig += '>1\t\tlelong\tx\t\tdictionary size: %d bytes,\n'
	
	sig += '\n# Assume that a valid size will be less than 1GB. This could technically be valid, but is unlikely.\n'
	sig += '>5\t\tlequad\t<1\t\tinvalid\n'
	sig += '>5\t\tlequad\t>0x40000000\tinvalid\n'

	sig += '\n# These are not 100%. The uncompressed size could be exactly the same as the dicionary size, but it is unlikely.\n'
	sig += '# Since most false positives are the result of repeating sequences of bytes (such as executable instructions),\n'
	sig += '# marking matches with the same uncompressed and dictionary sizes as invalid eliminates much of these false positives.\n'

	for dsize in dictionary_sizes:
		if dsize < 16777216:
			indent = '\t\t'
		else:
			indent = '\t'

		sig += '>1\t\tlelong\t%d\n' % dsize
		sig += '>>5\t\tlequad\t%d%sinvalid\n' % (dsize, indent)

	sig += '>5\t\tlequad\tx\t\tuncompressed size: %lld bytes\n'
	
	print sig
