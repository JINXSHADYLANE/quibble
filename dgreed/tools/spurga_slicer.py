#!/usr/bin/env python

from PIL import Image
from optparse import OptionParser
import mml
import sys
import os
import os.path

def parse_puzzle_def(defstr):
	h = defstr.count('\n')-1
	grid = defstr.split()
	w = len(grid) / h
	assert(w * h == len(grid))

	def f(c):
		if c == '#' or c == '@':
			return c
		return int(c)

	return w, h, map(f, grid)

def slice(grid, img):
	w, h = img.size[0], img.size[1]
	gw, gh = grid[0], grid[1]
	tw, th = w/gw, h/gh

	res = {}
	for i, tile in enumerate(grid[2]):
		if not tile in res:
			x, y = i % gw, i / gw
			res[tile] = img.crop((x*tw, y*th, x*tw+tw, y*th+th))
			res['s'+str(tile)] = res[tile].resize((tw+2, th+2), Image.NEAREST)

	return res

tiles_mml = None

atlas = None
atlas_name = None
atlas_cursor_y = 0 
atlas_name_counter = 0
def fits_into_atlas(slices):
	sw, sh = slices[0].size[0]+2, slices[0].size[1]+2
	if atlas != None:
		freespace_y = atlas.size[1] - atlas_cursor_y
		nw = atlas.size[0] / sw
		nh = len(slices) / nw
		if nw * nh != len(slices):
			nh += 1

		ph = nh * sh
		return freespace_y >= ph
	return False

def save_current_atlas():
	if atlas != None:
		atlas.save(atlas_name)

def make_new_atlas():
	global atlas, atlas_cursor_y, atlas_name, atlas_name_counter
	atlas = Image.new('RGBA', (512, 512), (0, 0, 0, 0))
	atlas_cursor_y = 0
	atlas_name = 'p_atlas' + str(atlas_name_counter) + '.png'
	atlas_name_counter += 1

def add_to_atlas(slices, puzzle_name):
	global atlas, atlas_cursor_y
	if not fits_into_atlas(slices):
		save_current_atlas()
		make_new_atlas()

	cursor_x = 0
	cursor_y = atlas_cursor_y
	node = mml.Node(puzzle_name, atlas_name)
	sw, sh = slices[0].size[0], slices[0].size[1]
	for tile in sorted(slices.keys()):
		if type(tile) == type('') and tile.startswith('s'):
			continue
		if cursor_x + sw+2 > atlas.size[0]:
			cursor_x = 0
			cursor_y += sh+2

		node.children.append(mml.Node(str(tile),
			'' + str(cursor_x+1) + ',' + str(cursor_y+1) + ',' + str(cursor_x+1 + sw)
			+ ',' + str(cursor_y+1 + sh))) 

		atlas.paste(slices['s'+str(tile)], (cursor_x, cursor_y))
		atlas.paste(slices[tile], (cursor_x+1, cursor_y+1))
		cursor_x += sw+2

	atlas_cursor_y = cursor_y
	if cursor_x != 0:
		atlas_cursor_y += sh+2

	tiles_mml.children.append(node)

def parse_puzzles(tree):
	for puzzle in desc_mml.children:
		img, grid = None, None

		for prop in puzzle.children:
			if prop.name == 'img':
				img = Image.open(prop.value)
				img = img.resize((img.size[0]/2, img.size[1]/2), Image.ANTIALIAS)
			if prop.name == 'def':
				grid = parse_puzzle_def(prop.value)

		if img == None or grid == None:
			raise RuntimeError('Bad puzzle ' + puzzle.value + ' description')

		slices = slice(grid, img)
		add_to_atlas(slices, puzzle.value)

if __name__ == '__main__':
	usage = '%prog puzzles.mml OUT'
	parser = OptionParser(usage=usage)

	(opt, args) = parser.parse_args(sys.argv)
	if len(args) != 3:
		parser.error('You must provide puzzles desc and output destination')

	desc_file = open(args[1], 'r')
	desc_mml = mml.deserialize(desc_file.read())

	tiles_mml = mml.Node('puzzle_tiles', '_')

	parse_puzzles(desc_mml)
	save_current_atlas()

	out = open(args[2], 'w')
	out.write(mml.serialize(tiles_mml))

