#!/usr/bin/python

from optparse import OptionParser
import os, os.path, re
import sys
import mml

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
			chapter_node.children.append(arena_node) 
		root.children.append(chapter_node)	
	
	out = open(outfile, 'w')
	out.write(mml.serialize(root))
	out.close()

if __name__ == '__main__':
	usage = '%prog DEST'
	parser = OptionParser(usage=usage)

	(opt, args) = parser.parse_args(sys.argv)
	if len(args) != 2:
		parser.error('You must provide destination file')

	chapters = scan()
	build_arena_list(chapters, args[1])

