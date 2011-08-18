#!/usr/bin/env python

from string import whitespace

class Node:
	def __init__(self):
		self.name = self.value = ''
		self.children = []
	
	def __init__(self, name, value):
		self.name = name
		self.value = value
		self.children = []

def filter_escape(string):
	'''Filters out escape sequences from string'''

	dict = {'n': '\n', 'r': '\r', 't': '\t', 'b': '\b'}
	decode_escape = lambda x: dict[x] if x in dict else x 
	result, i = '', 0
	while i < len(string):
		if string[i] == '\\':
			result += decode_escape(string[i+1])
			i += 2
			continue
		result += string[i]
		i += 1
	return result

def insert_escape(string):
	'''Inserts escapes sequences into string'''

	# repr escapes most chars, only " is left to be escaped
	res = repr(string)[1:-1].replace('"', '\\"')

	# HACK: repr escapes \b char to \x08, fix it	
	return res.replace('\\x08', '\\b')

def tokenize(input):
	'''Converts string into list of 3 types of tokens: (, ), and literals.'''

	result = []
	lit_start = 0 # literal starting position
	lit_len = 0   # length of literal found so far
	
	in_comment = False
	in_qliteral = False # in quoted literal

	for i, current in enumerate(input):

		# Comment, skip everything till line end
		if in_comment:
			if current == '\n':
				in_comment = False
			continue

		# Quoted literal, everything till end quote is single token
		if in_qliteral:
			if current == '"' and input[i-1] != '\\':
				in_qliteral = False
				result.append(filter_escape(input[lit_start:(lit_start+lit_len)]))
				lit_len = 0
				continue
			lit_len += 1	
			continue

		# Comment start
		if current == '#':
			in_comment = True
			continue

		# Quoted literal start
		if current == '"':
			in_qliteral = True
			lit_start = i+1
			continue
				
		# Whitespace, ends literal
		if current in whitespace:
			result.append(input[lit_start:(lit_start+lit_len)])
			lit_len = 0
			continue

		# Brace, ends literal
		if current == '(' or current == ')':
			result.append(input[lit_start:(lit_start+lit_len)])
			result.append(current)
			lit_len = 0
			continue

		if lit_len == 0:
			lit_start = i

		lit_len += 1

	if in_qliteral:
		raise ValueError, 'Unfinished quoted literal'
	if in_comment:
		raise ValueError, 'No newline after last comment'

	result.append(input[lit_start:(lit_start+lit_len)])
	result = filter(lambda x: len(x) > 0, result)

	return result		

def parse(tokens):
	'''Parses list of tokens to a tree of nodes'''

	is_brace = lambda x: x == '(' or x == ')'
	is_literal = lambda x: not is_brace(x)

	if len(tokens) < 4:
		raise ValueError, 'Node must be composed of atleast 4 tokens'
	if is_literal(tokens[0]) or is_literal(tokens[-1]):
		raise ValueError, 'Node must be surrounded with braces'
	if is_brace(tokens[1]) or is_brace(tokens[2]):
		raise ValueError, 'Node must have name and value'

	result = Node(tokens[1], tokens[2])
	i = 3
	while i < len(tokens):
		if is_literal(tokens[i]):
			raise ValueError, 'Unexpected literal'
		if tokens[i] == ')' and i != len(tokens)-1:
			raise ValueError, 'Unexpected closing brace'
		if tokens[i] == '(':
			depth, start = 1, i
			while depth != 0:
				i += 1
				if i == len(tokens)-1:
					raise ValueError, 'Reached node end while scanning for end of child node. Misplaced brace?'
				if tokens[i] == '(':
					depth += 1
				if tokens[i] == ')':
					depth -= 1
			result.children.append(parse(tokens[start:i+1]))
		i += 1
	return result

def deserialize(string):
	'''Deserializes mml string to a python tree'''

	return parse(tokenize(string))

def process(string):
	'''Inserts escapes sequences to literal, if neccessary'''

	strfind = lambda x, y: x.find(y) != -1
	quote = True if (strfind(string, ' ') or strfind(string, '\n') \
			or strfind(string, '\t') or strfind(string, '"')) else False
	return '"' + insert_escape(string) + '"' if quote else string

def serialize(tree, prefix=''):
	'''Converts tree of nodes to mml string'''

	result = prefix + '( ' + tree.name + ' ' + process(tree.value)
	if len(tree.children) == 0:
		return result + ' )\n'
	result += '\n'
	for child in tree.children:
		result += serialize(child, prefix + '    ')
	result += prefix + ')\n'
	return result

def serialize2(tree):
	'''Same as serialize, just makes shorter string'''

	result = '(' + tree.name + ' ' + process(tree.value)
	for child in tree.children:
		result += serialize2(child)
	result += ')'
	return result

def printtree(tree, prefix=''):
	print prefix + tree.name + tree.value
	for child in tree.children:
		printtree(child, prefix + ' ')


# --- Unit test ---
if __name__ == '__main__':
	import unittest
	import sys

	class TestMml(unittest.TestCase):
		input1 = '(name value)'
		input2 = '''( name value
    ( name2 value2 )
    ( name3 value3 )
)
'''
		input2c = ''' # comment
( name value # comment
( name2 value2) # comment
# long comment
(name3 value3)
) # comment
'''

		input3 = '(name value(name2 value2)(name3 value3))'
		#input4
		#input5
		input6 = '(name "long value...")'
		input7 = '(name "value\\nwith\\tescape \\"sequences\\"")'
		input8 = '(name "(misleading quoted (literal \\"with escapes\\"))")'
		input9 = '(name "#comment in literal")'
		
		def setUp(self):
			self.input4 = '(name value'
			for i in range(1000):
				self.input4 += '(node' + str(i) + ' ' + str(i) + ')'
			self.input4 += ')'

			#sys.setrecursionlimit(1500)
			self.input5 = '(name value'
			for i in range(500):
				self.input5 += '(node' + str(i) + ' ' + str(i)
			self.input5 += ')' * 501

		def testtokenize1(self):
			tokens = tokenize(self.input1)
			self.assert_(len(tokens) == 4)
			self.assert_(tokens[0] == '(')
			self.assert_(tokens[1] == 'name')
			self.assert_(tokens[2] == 'value')
			self.assert_(tokens[3] == ')')

		def testtokenize2(self):
			tokens = tokenize(self.input2)	
			self.assert_(len(tokens) == 12)
			self.assert_(tokens[0] == '(')
			self.assert_(tokens[-1] == ')')
			self.assert_(tokens[3] == '(')
			self.assert_(tokens[5] == 'value2')

		def testtokenize3(self):
			tokens1 = tokenize(self.input2)
			tokens2 = tokenize(self.input3)
			self.assert_(tokens1 == tokens2)

		def testtokenize4(self):
			tokens = tokenize(self.input4)
			self.assert_(len(tokens) == (4 + 4*1000))
			self.assert_(tokens[-3] == '999')

		def testtokenize5(self):
			tokens1 = tokenize(self.input2)
			tokens2 = tokenize(self.input2c)
			self.assert_(tokens1 == tokens2)

		def testtokenize6(self):
			tokens = tokenize(self.input6)
			self.assert_(len(tokens) == 4)
			self.assert_(tokens[2] == 'long value...')

		def testtokenize7(self):
			tokens = tokenize(self.input7)
			self.assert_(len(tokens) == 4)
			self.assert_(tokens[2] == 'value\nwith\tescape "sequences"')

		def testtokenize8(self):
			tokens = tokenize(self.input8)
			self.assert_(len(tokens) == 4)
			tokens = tokenize(tokens[2])
			self.assert_(len(tokens) == 8)
			self.assert_(tokens[5] == 'with escapes')

		def testtokenize9(self):
			tokens = tokenize(self.input9)
			self.assert_(len(tokens) == 4)
			self.assert_(tokens[2] == '#comment in literal')

		def testparse1(self):
			tree = deserialize(self.input1)
			self.assert_(tree.name == 'name')
			self.assert_(tree.value == 'value')
			self.assert_(len(tree.children) == 0)

		def testparse2(self):
			tree = deserialize(self.input2)
			self.assert_(tree.name == 'name')
			self.assert_(tree.value == 'value')
			self.assert_(len(tree.children) == 2)
			self.assert_(tree.children[0].name == 'name2')
			self.assert_(tree.children[0].value == 'value2')
			self.assert_(len(tree.children[0].children) == 0)
			self.assert_(tree.children[1].name == 'name3')
			self.assert_(tree.children[1].value == 'value3')
			self.assert_(len(tree.children[1].children) == 0)

		def testparse3(self):
			tree = deserialize(self.input4)
			self.assert_(tree.name == 'name')
			self.assert_(tree.value == 'value')
			self.assert_(len(tree.children) == 1000)
			self.assert_(tree.children[666].value == '666')
			self.assert_(tree.children[314].name == 'node314')

		def testparse4(self):
			tree = deserialize(self.input5)
			self.assert_(len(tree.children) == 1)
			self.assert_(len(tree.children[0].children[0].children) == 1)
			tree = tree.children[0]
			self.assert_(tree.name == 'node0')
			self.assert_(tree.value == '0')
			tree = tree.children[0].children[0].children[0].children[0].children[0]
			self.assert_(tree.value == '5')

		def testserialize(self):
			tree = deserialize(self.input2)
			self.assert_(serialize(tree) == self.input2)

		def testserialize2(self):
			tree = deserialize(self.input3) 
			self.assert_(serialize2(tree) == self.input3)

		def testserialize3(self):
			tree = deserialize(self.input7)
			string = serialize(tree)
			tree2 = deserialize(string)
			self.assert_(tree.name == tree2.name)
			self.assert_(tree.value == tree2.value)
			self.assert_(len(tree.children) == len(tree2.children))

		def testserialize4(self):
			tree = deserialize(self.input8)
			string = serialize(tree)
			tree2 = deserialize(string)
			self.assert_(tree.name == tree2.name)
			self.assert_(tree.value == tree2.value)
			self.assert_(len(tree.children) == len(tree2.children))

		def testserialize5(self):
			tree = deserialize(self.input8)
			string = serialize2(tree)
			tree2 = deserialize(string)
			self.assert_(tree.name == tree2.name)
			self.assert_(tree.value == tree2.value)
			self.assert_(len(tree.children) == len(tree2.children))

		def testfilterescape(self):
			orig = 'a \\\\\\n bc \\t\\tdef\\n\\b'
			filtered = filter_escape(orig)
			self.assert_(filtered == 'a \\\n bc \t\tdef\n\b')

		def testinsertescape(self):
			orig = ' \n\nqwe\trty \\\\\nui\b'
			inserted = insert_escape(orig)
			self.assert_(inserted == ' \\n\\nqwe\\trty \\\\\\\\\\nui\\b')

	unittest.main()

