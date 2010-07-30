#!/usr/bin/python

from PIL import Image
from optparse import OptionParser
import mml
import sys
import os
import os.path

def compress(arena_mml, mml_path, out_path):
	(folder, mml_filename) = os.path.split(mml_path)

	for child in arena_mml.children:
		if child.name == 'background' or child.name == 'walls':
			(img_directory, img_filename) = os.path.split(child.value)
			new_img_filename = img_filename.replace('tga', 'png')

			if not os.path.exists(folder + '/' + new_img_filename):
				img = Image.open(folder + '/' + img_filename)
				img.save(folder + '/' + new_img_filename, 'png')

			child.value = img_directory + '/' + new_img_filename

			os.remove(folder + '/' + img_filename)
	
	arena_file = open(out_path, 'w')
	arena_file.write(mml.serialize(arena_mml))
	arena_file.close()

if __name__ == '__main__':
	usage = '%prog SOURCE DEST'
	parser = OptionParser(usage=usage)
	
	(opt, args) = parser.parse_args(sys.argv)
	if len(args) != 3:
		parser.error('You must provide processed arena description file and output file')

	arena_file = open(args[1], 'r')
	arena_mml = mml.deserialize(arena_file.read())
	arena_file.close()

	compress(arena_mml, args[1], args[2])

