#!/usr/bin/python

# slow and simple lzss compresion/decompression,
# compatible with C version in dgreed utils.c

import struct

WINDOW_BITS = 11
LENGTH_BITS  = 5
MIN_MATCH = 3
WINDOW_SIZE = (1<<WINDOW_BITS)
MAX_MATCH = MIN_MATCH + (1<<LENGTH_BITS) - 1

def compress(input):
	out = [struct.pack('<I', len(input))]
	read_ptr = 0
	while read_ptr < len(input):
		flag_ptr = len(out)
		flag, bit = 0, -1
		out += [[]]
		while bit < 7 and read_ptr < len(input):
			bit += 1
			window_ptr = max(0, read_ptr - WINDOW_SIZE)
			best_match, best_match_ptr = 0, 0 
			window_str = input[window_ptr:read_ptr]
			match = input[read_ptr:read_ptr + MIN_MATCH]
			i = window_str.find(match)
			while i != -1 and best_match < MAX_MATCH and len(match) > best_match:
				best_match = len(match)
				best_match_ptr = i
				match = input[read_ptr : read_ptr + len(match) + 1]
				i = window_str.find(match)
			if best_match >= MIN_MATCH:
				flag |= 1 << bit
				pair = best_match_ptr 
				pair |= ((best_match-MIN_MATCH) << (16 - LENGTH_BITS))
				out += [struct.pack('<H', pair)]
				read_ptr += best_match
			else:
				out += [input[read_ptr]]
				read_ptr += 1
		out[flag_ptr] = struct.pack('B', flag)
	return ''.join(out)

def decompress(input):
	out = [[]] * struct.unpack('<I', input[:4])[0]
	read_ptr, write_ptr = 4, 0
	while write_ptr < len(out):
		flag, bit = struct.unpack('B', input[read_ptr])[0], -1
		read_ptr += 1
		while bit < 7 and write_ptr < len(out):
			bit += 1
			if flag & (1 << bit) == 0:
				out[write_ptr] = input[read_ptr]
				write_ptr += 1
				read_ptr += 1
				continue
			window_ptr = max(0, write_ptr - WINDOW_SIZE)
			pair = struct.unpack('<H', input[read_ptr:read_ptr+2])[0]
			read_ptr += 2
			offset = pair & (WINDOW_SIZE-1)
			length = (pair >> (16 - LENGTH_BITS)) + MIN_MATCH
			for i in xrange(length):
				out[write_ptr + i] = out[window_ptr + offset + i]
			write_ptr += length
	return ''.join(out)		

# --- Unit test ---
if __name__ == '__main__':
	import unittest

	class TestLzss(unittest.TestCase):
		input1 = ''
		input2 = 'a'
		input3 = 'linksminkimos, linksminkimos, kol dar jauni esmi'

		def setUp(self):
			self.input4 = 'x = '
			x = 7
			while len(self.input4) < 4*WINDOW_SIZE+17:
				self.input4 += str(x)
				x = (x * 1531)%47

		def testconstants(self):
			self.assert_(WINDOW_BITS + LENGTH_BITS == 16)
			self.assert_(MIN_MATCH < MAX_MATCH)
			self.assert_(MAX_MATCH - MIN_MATCH == (1<<LENGTH_BITS)-1)
			self.assert_(WINDOW_SIZE < (1<<16))

		def test1(self):
			compr = compress(self.input1)
			self.assert_(len(compr) == 4)
			self.assert_(decompress(compr) == self.input1)

		def test2(self):
			compr = compress(self.input2)
			self.assert_(len(compr) == 6)
			self.assert_(decompress(compr) == self.input2)

		def test3(self):
			compr = compress(self.input3)
			self.assert_(len(compr) < len(self.input3))
			self.assert_(decompress(compr) == self.input3)
		
		def test4(self):
			compr = compress(self.input4)
			self.assert_(len(compr) < len(self.input4))
			self.assert_(decompress(compr) == self.input4)
	
	unittest.main()


