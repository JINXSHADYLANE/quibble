#!/usr/bin/env python

from optparse import OptionParser
import os, re
import sys

if sys.platform == 'win32':
	separator = '\\'
else:
	separator = '/'

def scan():
	tests_list = []
	file_expr = re.compile('\S+\.(tests|tg)\Z')
	for root, dirs, files in os.walk('.'):
		if root == '':
			path = '.' + separator
		else:
			path = root + separator

		for file in files:
			if file_expr.match(file) != None:
				tests_list.append(path + file)	
	return tests_list			

def path_to_name(path):
	return path[path.rfind(separator)+1:path.rfind('.')]

def build_test_data(tests_list, outfile):
	all_tests = []

	include_text = ''
	group_text = 'TestGroup groups[GROUP_COUNT] = {\n'
	test_text = 'fun_ptr tests[TEST_COUNT] = {\n'

	for test in tests_list:
		(setup_name, teardown_name, tests, include) = analyze_file(test)	

		include_text += include

		group_desc = '{"' + path_to_name(test) + '", '
		group_desc += (setup_name if setup_name != '' else 'NULL') + ', '
		group_desc += (teardown_name if teardown_name != '' else 'NULL') + ', '
		group_desc += str(len(all_tests)) + ', ' + str(len(tests)) + '}'
		group_text += '\t' + group_desc + ',\n'
		all_tests += tests
	group_text += '};\n\n'	
	
	for test in all_tests:
		test_text += '\t&' + test + ',\n'
	test_text += '};\n\n'	

	final_text = '#define GROUP_COUNT ' + str(len(tests_list)) + '\n'
	final_text += '#define TEST_COUNT ' + str(len(all_tests)) + '\n\n'
	final_text += 'uint group_count = ' + str(len(tests_list)) + ';\n'
	final_text += 'uint test_count = ' + str(len(all_tests)) + ';\n\n'
	final_text = '#include <test.h>\n' + include_text + '\n' + final_text + group_text + test_text

	out = open(outfile, 'w')
	out.write(final_text)
	out.close()

setup_def = re.compile('SETUP_')
teardown_def = re.compile('TEARDOWN_')
test_def = re.compile('TEST_\(\s*(\w+)\s*\)')
assert_def = re.compile('ASSERT_\s*\(')

def process_file(path, file_text, group_name):
	file_text = file_text.replace('SETUP_', 'void ' + group_name + '_setup(void)') 
	file_text = file_text.replace('TEARDOWN_', 'void ' + group_name + '_teardown(void)')

	result = '// --- ' + path + ' ---\n'
	test_name = 'ERROR'
	q = lambda x: '"' + x + '"'
	for i, line in enumerate(file_text.split('\n')):
		match = test_def.search(line)
		if match:
			test_name = match.group(1)
			new_test = 'void ' + group_name + '_' + match.group(1) + \
				'_test(void)'
			l = line[:match.start()] + new_test + line[match.end():]
			result += l + '\n'
			continue

		match = assert_def.search(line)	
		if match:
			new_assert = 'ASSERT__(' + group_name + ', ' + test_name + ', '
			npath = path.replace('.'+separator, '')
			npath = npath.replace('\\', '\\\\')
			new_assert += q(npath) + ', ' + str(i+1) + ', '
			l = line[:match.start()] + new_assert + line[match.end():]
			result += l + '\n'
			continue
		result += line + '\n'	
	return result		
		
def analyze_file(path):
	setup_name = ''
	teardown_name = ''
	tests = []

	group_name = path_to_name(path)

	file = open(path)
	file_text = file.read()

	match = setup_def.findall(file_text)
	if len(match) > 1:
		raise ValueError('More than one setup in ' + path)
	if len(match) == 1:
		setup_name = group_name + '_setup' 

	match = teardown_def.findall(file_text)
	if len(match) > 1:
		raise ValueError('More than one teardown in ' + path)
	if len(match) == 1:
		teardown_name = group_name + '_teardown'

	matches = test_def.findall(file_text)
	if len(matches) < 1:
		raise ValueError('No tests in ' + path)
	for match in matches:
		test_name = group_name + '_' + match + '_test'
		tests.append(test_name)
	
	file.close()

	include = process_file(path, file_text, group_name)

	return (setup_name, teardown_name, tests, include)

if __name__ == '__main__':
	parser = OptionParser()
	parser.add_option('-s', '--scan', dest='scan', action='store_true',
		default=False, help='recursively scan for .tests files')
	parser.add_option('-o', '--output', dest='output', action='store',
		type='string', help='name of output file', default='test_data.h')
	
	(options, args) = parser.parse_args()
	if options.scan and len(args) != 0:
		parser.error("don't provide files if scanning is used")
	if not options.scan and len(args) == 0:
		parser.error('provide at least one test file')
		
	files = scan() if options.scan else args	
	build_test_data(files, options.output)
