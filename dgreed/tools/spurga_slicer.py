#!/usr/bin/env python2

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
		if len(c) == 1:
			if ord('a') <= ord(c) <= ord('z'):
				return c
			if ord('A') <= ord(c) <= ord('Z'):
				return c
		return int(c)

	return w, h, map(f, grid)

def slice(grid, img, img_hd):
	w, h = img.size[0], img.size[1]
	gw, gh = grid[0], grid[1]
	tw, th = w/gw, h/gh

	res = {}
	for i, tile in enumerate(grid[2]):
		if not tile in res:
			x, y = i % gw, i / gw
			res[tile] = img.crop((x*tw, y*th, x*tw+tw, y*th+th))
			res['.'+str(tile)] = res[tile].resize((tw+2, th+2), Image.NEAREST)
			res[':'+str(tile)] = img_hd.crop(((x*tw)*2, (y*th)*2, (x*tw+tw)*2, (y*th+th)*2))
			res[':.'+str(tile)] = res[':'+str(tile)].resize(((tw+2)*2, (th+2)*2), Image.NEAREST)

	return res

tiles_mml = None

atlas, atlas_hd = None, None
atlas_name, atlas_name_hd = None, None
atlas_used = 0 
atlas_name_counter = 0
cursor_x = 0
cursor_y = 0

def fits_into_atlas(slices):
	sw, sh = slices[0].size[0]+2, slices[0].size[1]+2
	if atlas != None:
		fits = (atlas.size[0] / sw) * (atlas.size[1] / sh)
		freespace = fits - atlas_used
		return freespace >= len(slices) / 4
	return False

def save_current_atlas():
	if atlas != None:
		atlas.save(atlas_name)
		atlas_hd.save(atlas_name_hd)

def make_new_atlas():
	global atlas, atlas_hd, atlas_used, atlas_name, atlas_name_hd, atlas_name_counter
	global cursor_x, cursor_y
	atlas = Image.new('RGBA', (512, 512), (0, 0, 0, 0))
	atlas_hd = Image.new('RGBA', (1024, 1024), (0, 0, 0, 0))
	atlas_used = 0
	atlas_name = 'p_atlas' + str(atlas_name_counter) + '.png'
	atlas_name_hd = 'p_atlas' + str(atlas_name_counter) + 'hd.png'
	atlas_name_counter += 1
	cursor_x = 0
	cursor_y = 0 

def add_to_atlas(slices, puzzle_name):
	global atlas, atlas_hd, atlas_used
	global cursor_x, cursor_y
	if not fits_into_atlas(slices):
		save_current_atlas()
		make_new_atlas()

	node = mml.Node(puzzle_name, atlas_name)
	sw, sh = slices[0].size[0], slices[0].size[1]
	for tile in sorted(slices.keys()):
		if type(tile) == type('') and tile.startswith('.'):
			continue

		if type(tile) == type('') and tile.startswith(':'):
			continue

		if cursor_x + sw+2 > atlas.size[0]:
			cursor_x = 0
			cursor_y += sh+2

		node.children.append(mml.Node(str(tile),
			'' + str(cursor_x+1) + ',' + str(cursor_y+1) + ',' + str(cursor_x+1 + sw)
			+ ',' + str(cursor_y+1 + sh))) 

		atlas.paste(slices['.'+str(tile)], (cursor_x, cursor_y))
		atlas.paste(slices[tile], (cursor_x+1, cursor_y+1))
		atlas_hd.paste(slices[':.'+str(tile)], (cursor_x*2, cursor_y*2))
		atlas_hd.paste(slices[':'+str(tile)], ((cursor_x+1)*2, (cursor_y+1)*2))
		cursor_x += sw+2
		atlas_used += 1

	tiles_mml.children.append(node)

def parse_puzzles(tree):
	for puzzle in desc_mml.children:
		img, img_hg, grid = None, None, None

		for prop in puzzle.children:
			if prop.name == 'img':
				img = Image.open(prop.value)
				img = img.resize((img.size[0]/2, img.size[1]/2), Image.ANTIALIAS)
				img_hd = Image.open(prop.value)
			if prop.name == 'def':
				grid = parse_puzzle_def(prop.value)

		if img == None or grid == None:
			raise RuntimeError('Bad puzzle ' + puzzle.value + ' description')

		slices = slice(grid, img, img_hd)
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

