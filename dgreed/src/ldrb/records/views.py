from models import record
from django.http import HttpRequest, HttpResponse
import mml
import datetime


def put(request):
	
	print 'request: ', request , ' -- end'
	#print 'POST:', request.META['POST']

	tree = mml.deserialize(request.META['POST'])	
	fields = dict( [(tree.children[i].name, tree.children[i].value) for i in range(4)] )

	r = record()
	r.gid = fields['gid']
	r.uid = fields['uid']
	r.time = datetime.datetime.now()
	r.weight = fields['weight']
	r.data = fields['data']
	r.save()
	
	response = HttpResponse()
	response.status_code = 200 # OK
	return response


def query(request):

	req = mml.deserialize(request.raw_post_data)

	result = record.objects.filter(time > req[2].value and time < req[3].value).order_by('weight')
	result = result[ :req[1].value ]  # take first n values
	
	response = HttpResponse()
	# write result to response
	response.status_code = 200 # ok	
	return response

