#!/usr/bin/env python
import sys
if len(sys.argv) < 2:
	print('Usage: textpack_converter.py <input_file> [<output_file>=out_<input_file>]')
	exit()

def parse_coords(string):
	string = string.replace('<string>','').replace('</string>','').replace('{','').replace('}','')
	return string

filename = sys.argv[1]
if len(sys.argv) >= 3:
	outname = sys.argv[2]
else:
	outname = 'out_'+filename

use_centers = False

if len(sys.argv) > 3:
	if sys.argv[3] == "c":
		use_centers = True

fileobj = open(filename)
line = fileobj.readline().strip()
next = 0

coords = []
titles = []
crops = []
source = []

while line != '':
	if line.find('.png') >= 0 and line.find('<key>') >= 0:
		titles.append(line.replace('<key>','').replace('</key>','').replace('.png',''))
	
	if line == '<key>sourceColorRect</key>':
		line = fileobj.readline().strip()
		sep_line = parse_coords(line).split(',')
		for i in range(len(sep_line)):
			sep_line[i] = int(sep_line[i])
		crops.append(sep_line)
		
	if line == '<key>sourceSize</key>':
		line = fileobj.readline().strip()
		sep_line = parse_coords(line).split(',')
		for i in range(len(sep_line)):
			sep_line[i] = int(sep_line[i])
		source.append(sep_line)
		
	
	if line == '<key>frame</key>':
		line = fileobj.readline().strip()
		sep_line = parse_coords(line).split(',')
		for i in range(len(sep_line)):
			sep_line[i] = int(sep_line[i])
		coords.append(sep_line)
		
	line = fileobj.readline().strip()

fileobj.close()

outfile = open(outname, 'w')
	
for i in range(len(coords)):
	src = source[i]
	crp = crops[i]
	calc_center = [round(src[0]/2 - crp[0]+coords[i][0]), round(src[1]/2 - crp[1]+coords[i][1])]
	coords[i][2] = coords[i][2] + coords[i][0]
	coords[i][3] = coords[i][3] + coords[i][1]
	outfile.write('(img ' + titles[i] + '\n')
	outfile.write('\t(tex atlas.png)\n')
	outfile.write('\t(src ' + ",".join("%s" % el for el in coords[i]) + ')\n')

	if use_centers == True:
		outfile.write('\t(cntr ' + ",".join("%s" % el for el in calc_center) + ')\n')

	outfile.write(')\n')
outfile.close()