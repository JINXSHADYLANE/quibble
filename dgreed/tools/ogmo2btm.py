#!/usr/bin/python

# Converts xml tilemaps made using Ogmo Editor to a much more efficient BTM
#
# BTM file format:
# 4 - 'BTM0'  
# 2 - tile width
# 2 - tile height
# 4 - number of tileset defs
# for each tileset def:
#   4 + len - texture filename
#   4 - number of animated tile defs
#   for each animated tile def:
#     4 - fps
#     4 - start tile idx
#     4 - end tile idx (inclusive)
# 4 - tilemap width
# 4 - tilemap height
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



