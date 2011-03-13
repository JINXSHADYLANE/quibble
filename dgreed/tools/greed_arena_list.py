#!/usr/bin/env python2

from PIL import Image, ImageDraw, ImageChops
from optparse import OptionParser
import os, os.path, re
import sys
import mml

ring_img, ring_shadow, core_img, core_shadow = None, None, None, None
matlas = None

if sys.platform == 'win32':
	separator = '\\'
else:
	separator = '/'

def filename(path):
	(folder, filename) = os.path.split(path)
	return filename
	
def scan():
	arenas_list = []
	arena_expr = re.compile('\S+\.(mml)\Z')
	for root, dirs, files in os.walk('.'):
		if root == '':
			path = '.' + separator
		else:
			path = root + separator
		
		for file in files:
			if arena_expr.match(file) != None:
				contents = open(path + file, 'r')
				tree = mml.deserialize(contents.read())
				contents.close()

				if tree.name == 'arena':
					arenas_list.append(path + file)
	
	# Group by chapter
	arenas_list.sort()
	chapters_list = []
	last_chapter = ''
	for arena in arenas_list:
		chapter = filename(arena)[:2]
		if chapter != last_chapter:
			last_chapter = chapter
			chapters_list.append([arena])
		else:
			chapters_list[len(chapters_list)-1].append(arena)

	return chapters_list			

def build_arena_list(chapters_list, outfile):
	root = mml.Node('arenas', '_')

	for chapter in chapters_list:
		name = filename(chapter[0])[:2]
		render_arenas(chapter, 'atlas_' + name + '.png')
		chapter_node = mml.Node('chapter', name)
		for arena in chapter:
			contents = open(arena, 'r')
			tree = mml.deserialize(contents.read())
			contents.close()

			max_players = '0'
			for child in tree.children:
				if child.name == 'max_players':
					max_players = child.value
			
			arena_node = mml.Node('greed_assets/' + filename(arena), max_players)
			name_node = mml.Node('name', tree.value)
			arena_node.children.append(name_node)
			chapter_node.children.append(arena_node) 
		root.children.append(chapter_node)	
	
	out = open(outfile, 'w')
	out.write(mml.serialize(root))
	out.close()

def parse_vector(string):
	s = string.split(',')
	return (float(s[0]), float(s[1]))

def render_arenas(chapter, outfile):
	img = Image.new('RGBA', (1024, 512), (0, 0, 0, 0))
	cursor_x, cursor_y = 1, 1
	for arena in chapter:
		contents = open(arena, 'r')
		tree = mml.deserialize(contents.read())
		contents.close()

		arena_img = render_arena(tree)
		
		if cursor_x + 338 >= 1024:
			cursor_x = 1
			cursor_y += 226
		
		img.paste(arena_img, (cursor_x, cursor_y))
		
		cursor_x += 339

	img.save(outfile)


def render_arena(desc):
	background, walls = None, None
	shadow_shift = (0, 0)
	platforms = []

	for child in desc.children:
		if child.name == 'background':
			background = Image.open(filename(child.value))
		if child.name == 'walls':
			walls = Image.open(filename(child.value))
		if child.name == 'shadow_shift':
			shadow_shift = parse_vector(child.value)
		if child.name == 'platforms':
			for platform in child.children:
				platforms.append(parse_vector(platform.value))
	
	if background == None or walls == None or len(platforms) == 0:
		raise RuntimeError('Unable to render arena - bad desc')

	img = Image.new('RGBA', (480, 320), (0, 0, 0, 0))
	img.paste(background.crop((0, 0, 480, 320)))
	
	walls_img = walls.crop((0, 0, 480, 320))
	shadow_img = walls.crop((0, 321, 240, 321 + 160))
	shadow_img = shadow_img.resize((480, 320), Image.BICUBIC)
	shadow_img = ImageChops.offset(shadow_img, int(shadow_shift[0]), int(shadow_shift[1]))
	
	b = background.crop((0, 0, 480, 320))
	img = Image.composite(img, shadow_img, ImageChops.invert(shadow_img))
	img.putalpha(b.split()[-1])
	
	# Platforms
	for p in platforms:
		x, y = int(p[0]) - 26, int(p[1]) - 26			
		r_img = ImageChops.offset(ring_img, x, y)
		c_img = ImageChops.offset(core_img, x, y)
		x, y = x + int(shadow_shift[0]), y + int(shadow_shift[1])
		r_shd = ImageChops.offset(ring_shadow, x, y)
		c_shd = ImageChops.offset(core_shadow, x, y)
		img = Image.composite(img, r_shd, ImageChops.invert(r_shd))
		img = Image.composite(img, c_shd, ImageChops.invert(c_shd))
		img = Image.composite(img, r_img, ImageChops.invert(r_img))
		img = Image.composite(img, c_img, ImageChops.invert(c_img))
		img.putalpha(b.split()[-1])
	
	img = Image.composite(img, walls_img, ImageChops.invert(walls_img))
	img.putalpha(b.split()[-1])


	img = img.resize((338, 225), Image.ANTIALIAS)
	b = img.copy()
	img = Image.composite(img, matlas, ImageChops.invert(matlas))
	img.putalpha(matlas.split()[-1])
	bands = img.split()
	max_alpha = bands[-1].getextrema()[1]
	bands_a = Image.eval(bands[-1], lambda x: int(x / float(max_alpha) * 255.0))
	img = Image.merge('RGBA', (bands[0], bands[1], bands[2], bands_a))
	return img	

if __name__ == '__main__':
	usage = '%prog DEST'
	parser = OptionParser(usage=usage)

	(opt, args) = parser.parse_args(sys.argv)
	if len(args) != 2:
		parser.error('You must provide destination file')

	atlas = Image.open('atlas1.png')
	matlas = Image.open('menu_atlas.png')
	matlas = matlas.crop((0, 0, 338, 225))

	ring_img = Image.new('RGBA', (480, 320), (0, 0, 0, 0))
	ring_shadow = Image.new('RGBA', (480, 320), (0, 0, 0, 0))
	core_img = Image.new('RGBA', (480, 320), (0, 0, 0, 0))
	core_shadow = Image.new('RGBA', (480, 320), (0, 0, 0, 0))

	ring_img.paste(atlas.crop((360, 360, 360 + 52, 360 + 52)), (0, 0))
	ring_shadow.paste(atlas.crop((433, 360, 433 + 52, 360 + 52)), (0, 0))
	core_img.paste(atlas.crop((1, 433, 1 + 40, 433 + 40)), (6, 6))
	core_shadow.paste(atlas.crop((144, 431, 144 + 45, 431 + 45)), (3, 3))

	chapters = scan()
	build_arena_list(chapters, args[1])

