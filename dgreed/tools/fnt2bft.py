#!/usr/bin/python

from __future__ import with_statement
import sys 
reload(sys)
sys.setdefaultencoding('latin-1')
import struct, xml.parsers.expat

output = "FONT"

def put_byte(val):
    global output
    output += struct.pack('<h', val)

def start(name, attrs):
    global output
    if name == 'common':
        put_byte(int(attrs['lineHeight']))
    if name == 'page':
        put_byte(len(attrs['file']))
        output += attrs['file']
    if name == 'chars':
        put_byte(int(attrs['count']))
    if name == 'char':
        put_byte(int(attrs['id']))
        put_byte(int(attrs['x']))
        put_byte(int(attrs['y']))
        put_byte(int(attrs['width']))
        put_byte(int(attrs['height']))
        put_byte(int(attrs['xoffset']))
        put_byte(int(attrs['yoffset']))
        put_byte(int(attrs['xadvance']))
        
def end(name):
    pass

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print "Provide file to convert and output file"
    else:
        parser = xml.parsers.expat.ParserCreate()
        parser.StartElementHandler = start
        parser.EndElementHandler = end
        with open(sys.argv[1]) as infile:
            parser.ParseFile(infile)
        with open(sys.argv[2], 'wb') as outfile:
            outfile.write(output)
        
            
            
