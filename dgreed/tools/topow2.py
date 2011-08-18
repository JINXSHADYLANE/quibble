#!/usr/bin/env python

from PIL import Image
from optparse import OptionParser
import sys

def next_pow2(n):
	res = 1
	while res < n:
		res *= 2
	return res

def make_new_img(old_img, center):
	new_size = (next_pow2(old_img.size[0]), next_pow2(old_img.size[1]))
	new_img = Image.new('RGBA', new_size, (0, 0, 0, 0))
	if not center:
		new_img.paste(old_img, (0, 0))
	else:
		dx, dy = new_size[0] - old_img.size[0], new_size[1] - old_img.size[1]
		new_img.paste(old_img, (dx/2, dy/2))
	return new_img	

if __name__ == '__main__':
	usage = 'usage %prog [options] SOURCE DEST'
	parser = OptionParser(usage=usage)
	parser.add_option('-c', '--center',
		help='make transparent border around the old image at the center',
		action='store_true', dest='center', default=False)
	
	(opt, args) = parser.parse_args(sys.argv)
	if len(args) != 3:
		parser.error('You must specify source and destination')

	old_img = Image.open(args[1])
	new_img = make_new_img(old_img, opt.center)
	new_img.save(args[2], 'png')

