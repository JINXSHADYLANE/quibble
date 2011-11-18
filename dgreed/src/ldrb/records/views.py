from models import *
from django.http import HttpRequest, HttpResponse
import datetime
import time
import mml


def put(request):
	response = HttpResponse()
	#print 'Put: request: ', request , ' -- end\n'
	try:
		tree = mml.deserialize(request.META['raw_post_data'])
	except Exception:
		response.status_code = 400 # Bad Request
		response.write('Bad entry format')
		return response 

	if len(tree.children) != 4:	
		response.status_code = 400 # Bad Request
		response.write('Bad entry format. 4 fields required, got ') 
		response.write(len(tree.children))
		return response

	fields = dict( [(tree.children[i].name, tree.children[i].value) for i in range(4)] )

	#TODO: validate fields

	r = record()
	r.gid = fields['gid']
	r.uid = fields['uid']
	r.time = datetime.datetime.now()
	r.weight = fields['weight']
	r.data = fields['data']
	r.save()
	
	response.status_code = 201 # Created
	return response 



def query(request):
	#print '\nQuery: request: ', request , ' -- end\n'
	response = HttpResponse(mimetype="text/plain")
	
	try:
		tree = mml.deserialize(request.META['QUERY_STRING'])
	except Exception:
		response.status_code = 400 # Bad Request
		response.write('Bad query format')
		return response 

	if len(tree.children) != 5:	
		response.status_code = 400 # Bad Request
		response.write('Bad query format. 5 fields required, got ') 
		response.write(len(tree.children))
		return response

	#for i in record.objects.values_list():
	#	print i
	#print '\n'

	fields = dict( [(tree.children[i].name, tree.children[i].value) for i in range(5)] )
	
	startTime = datetime.datetime(*time.strptime(fields['startTime'], '%Y%m%d%H%M%S')[0:6] )
	endTime = datetime.datetime(*time.strptime(fields['endTime'], '%Y%m%d%H%M%S')[0:6] ) 
	
	results = record.objects.filter(gid=fields['gid'])
	if startTime < endTime:
		results = results.filter(time__gt=startTime).filter(time__lt=endTime)
	elif startTime > endTime:
		print 'startTime > endTime'
	results = results.order_by('weight')[fields['startEntry'] : fields['startEntry'] + fields['entryCount']]

	resp = mml.Node( 'entries', str(len(results)) )
	for r in results:
		entry = (mml.Node( 'entry', '_'))
		entry.children.append(mml.Node('gid', r.gid))
		entry.children.append(mml.Node('uid', r.uid))
		entry.children.append(mml.Node('time', r.time.strftime('%Y%m%d%H%M%S')))
		entry.children.append(mml.Node('weight', str(r.weight)) )
		entry.children.append(mml.Node('data', r.data))
		resp.children.append(entry)

	response.write(mml.serialize(resp))
	response.status_code = 201 # Created	
	return response

