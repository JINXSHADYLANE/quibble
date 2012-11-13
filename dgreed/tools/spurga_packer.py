#!/usr/bin/env python

import sys, glob
import Image

if __name__ == "__main__":
	fnames = glob.glob("*.png")
	images = map(lambda x: Image.open(x).convert("RGBA"), fnames)

	print "found png images:"
	for name, img in zip(fnames, images):
		print name, img.size

	print "enter output image size:"
	width, height = map(int, sys.stdin.readline().strip().split())

	canvas = Image.new("RGBA", (width, height))

	x, y, h = 0, 0, 0
	for img in images:
		if x+img.size[0] > width:
			x, y = 0, h

		canvas.paste(img, (x, y, x+img.size[0], y+img.size[1]))

		x += img.size[0]
		h = max(h, y+img.size[1])

	canvas.show()
