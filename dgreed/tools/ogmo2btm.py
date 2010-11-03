#!/usr/bin/env python2

# Converts xml tilemaps made using Ogmo Editor to a much more efficient BTM
#
# BTM file format:
# 4 - 'BTM0'  
# 2 - tile width
# 2 - tile height
# 4 - tilemap width
# 4 - tilemap height
# 4 - number of tileset defs
# for each tileset def:
#   4 + len - texture filename
#   4 - number of animated tile defs
#   for each animated tile def:
#     4 - fps
#     4 - start tile idx
#     4 - end tile idx (inclusive)
# 4 - number of objects
# for each object:
#   4 - object id
#   4 - x
#   4 - y
# 4 - number of layers
# for each layer:
#   4 + len - layer name
#   4 - lzss compressed data size
#   x - compressed array of width*height uint16 tile ids
# 4 - lzss compressed collission data size
# x - compressed array of width*height/8 bytes

from __future__ import with_statement
from optparse import OptionParser
import struct
import xml.parsers.expat
import lzss
import sys
reload(sys)
sys.setdefaultencoding('latin-1')

TILESET_WIDTH = 32
TILESET_HEIGHT = 32
TILES_IN_TILESET = TILESET_WIDTH * TILESET_HEIGHT

prefix = ''
tile_width, tile_height = 0, 0
width, height = 0, 0
object_ids = {}
col, layers, tilesets, objects = [], [], [], []
state, col_name, obj_name, layer_state, tileset_state = '', '', '', -1, -1	
raw_col = ''

def _2dlist(e, w, h): 
	return [[e] * h for _ in xrange(w)]

def write_btm(filename):
	out = ['BTM0']
	out += [struct.pack('<HH', tile_width, tile_height)]
	out += [struct.pack('<II', width, height)]
	out += [struct.pack('<I', len(tilesets))]
	for tileset in tilesets:
		out += [struct.pack('<I', len(prefix+tileset[1])), prefix+tileset[1]]
		# TODO: Animated tile defs
		out += [struct.pack('<I', 0)]
	
	out += [struct.pack('<I', len(objects))]
	for object in objects:
		out += [struct.pack('<III', object[0], object[1], object[2])]
	
	out += [struct.pack('<I', len(layers))]
	for layer in layers:
		out += [struct.pack('<I', len(layer[0])), layer[0]]
		raw_tiles = []
		for y in xrange(height):
			for x in xrange(width):
				raw_tiles += [struct.pack('H', layer[1][x][y])]
		compr_tiles = lzss.compress(''.join(raw_tiles))
		out += [struct.pack('<I', len(compr_tiles)), compr_tiles]
	
	ser_col, raw_col = [], []
	for y in xrange(height):
		for x in xrange(width):
			ser_col.append(col[x][y])
	for i in xrange(0, len(ser_col), 8):
		mask = 0
		for b in xrange(8):
			if i + b == len(ser_col):
				break
			if ser_col[i + b] == 1:
				mask |= 1 << (7-b)
		raw_col += [struct.pack('B', mask)]		

	compr_col = lzss.compress(''.join(raw_col))
	out += [struct.pack('<I', len(compr_col)), compr_col]
	
	with open(filename, 'wb') as outfile:
		data = ''.join(out)
		outfile.write(data)		

def read_ogmo(level_filename, project_filename):

	def proj_start(name, attrs):
		global tile_width, tile_height, col_name, layers
		if name == 'tileset':
			tilesets.append([attrs['name'], attrs['image'], []])
			tw, th = int(attrs['tileWidth']), int(attrs['tileHeight'])
			if tile_width == 0 and tile_height == 0:
				tile_width, tile_height = tw, th 
			elif tile_width != tw or tile_height != th:
				raise RuntimeError('conflicting tile size')	
		if name == 'tiles':
			layers.append([attrs['name'], []])
			if int(attrs['gridSize']) != tile_height:
				raise RuntimeError('bad layer grid size')
		if name == 'grid':
			col_name = attrs['name']
			if int(attrs['gridSize']) != tile_height:
				raise RuntimeError('bad collission grid size')
		if name == 'object':
			if attrs['name'] not in object_ids:
				object_ids[attrs['name']] = len(object_ids)
		if name == 'objects':
			if 'name' not in attrs:
				return
			objs_name = attrs['name']
			if int(attrs['gridSize']) != tile_height:
				raise RuntimeError('bad object grid size')

	def proj_end(name):
		pass

	with open(project_filename) as proj_file:
		parser = xml.parsers.expat.ParserCreate()
		parser.StartElementHandler = proj_start
		parser.EndElementHandler = proj_end
		parser.ParseFile(proj_file)

	if col_name == '' or len(layers) == 0 or len(tilesets) == 0 or \
		tile_width == 0 or tile_height == 0:
		raise RuntimeError('required data not found in project file')

	def lvl_start(name, attrs):
		global tile_width, tile_height, width, height, state, layers, col
		global layer_state, tileset_state
		if name == 'tile':
			tw, th = tile_width, tile_height
			tx, ty = int(attrs['tx']), int(attrs['ty'])
			x, y = int(attrs['x']), int(attrs['y'])
			tx, ty, x, y = tx/tw, ty/th, x/tw, y/th
			tile_id = tileset_state * TILES_IN_TILESET + (ty*TILESET_WIDTH+tx)
			layers[layer_state][1][x][y] = tile_id
			return

		if name == 'rect':
			tw, th = tile_width, tile_height
			tx, ty = int(attrs['tx']), int(attrs['ty'])
			x, y = int(attrs['x']), int(attrs['y'])
			w, h = int(attrs['w']), int(attrs['h'])
			tx, ty, x, y, w, h = tx/tw, ty/th, x/tw, y/th, w/tw, h/th
			tile_id = tileset_state * TILES_IN_TILESET + (ty*TILESET_WIDTH+tx)
			for ix in xrange(x, x+w):
				for iy in xrange(y, y+h):
					layers[layer_state][1][ix][iy] = tile_id
			return

		if name == 'width' or name == 'height':
			state = name
			return

		if name == col_name:
			state = 'col'
			if width == 0 or height == 0:
				raise RuntimeError('no width/height data found in level')
			col = _2dlist(0, width, height)
			return

		if name == obj_name:
			state = 'obj'

		if name in object_ids:
			objects.append([object_ids[str(name)], 
				int(attrs['x']) / tile_width, int(attrs['y']) / tile_height])

		for i, layer in enumerate(layers):
			if name == layer[0]:
				state = 'layer'
				layer_state = i
				if width == 0 or height == 0:
					raise RuntimeError('no width/height data found in level')
				if layer[1] == []:
					layer[1] = _2dlist(0, width, height)
				for i, tileset in enumerate(tilesets):
					if attrs['set'] == tileset[0]:
						tileset_state = i
						break
				else:
					raise RuntimeError('unidentified tileset')
				return
	
	def lvl_end(name):
		global state
		state = ''	

	def lvl_char(data):
		global width, height, state, col, raw_col
		if state == 'width':
			width = int(data) / tile_width
		if state == 'height':
			height = int(data) / tile_height
		if state == 'col':
			raw_col += filter(lambda x: x == '0' or x == '1', data)

	with open(level_filename) as lvl_file:
		parser = xml.parsers.expat.ParserCreate()
		parser.StartElementHandler = lvl_start
		parser.EndElementHandler = lvl_end
		parser.CharacterDataHandler = lvl_char
		parser.ParseFile(lvl_file)

	if len(raw_col) != width * height:
		raise RuntimeError('bad collission data size')
	i = 0
	for y in xrange(height):
		for x in xrange(width):
			if raw_col[i] == '1':
				col[x][y] = 1
			i += 1	

	for layer in layers:
		if layer[1] == []:
			layer[1] = _2dlist(0, width, height)

if __name__ == '__main__':
	parser = OptionParser()
	parser.add_option('-o', '--output', dest='output', action='store',
			type='string', help='name of output file', default='out.btm')
	parser.add_option('-p', '--prefix', dest='prefix', action='store',
			type='string', help='tileset texture filename prefix', default='')

	(options, args) = parser.parse_args()
	if len(args) != 2:
		parser.error('please provide level and project filenames')

	prefix = options.prefix	

	read_ogmo(args[0], args[1])
	write_btm(options.output)

