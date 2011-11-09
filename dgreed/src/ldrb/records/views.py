from models import record
from django.http import HttpRequest, HttpResponse
import mml
import datetime


def put(request):
	
	print 'request: ', request , ' -- end'
	# pvz: "(record r (gid rthief) (uid awesomeWormie) (weight 5) (data fhdfhd5HBe74&*6))"
	tree = mml.deserialize(request.raw_post_data)	
	#TODO: tree validation

	r = record()
	r.gid = tree.children[0].value
	r.uid = tree.children[1].value
	r.time = datetime.datetime.now()
	r.weight = tree.children[2].value
	r.data = tree.children[3].value
	r.save()

	response = HttpResponse()
	response.status_code = 200 # OK
	return response


def query(request):

	req = mml.deserialize(request.raw_post_data)

	result = record.objects.filter(time > req[2].value and time < req[3].value).order_by('weight')
	result = result[ :req[1].value ]  # take first n values
	
	response = HttpResponse()
	# write results to response
	response.status_code = 200 # ok	
	return response

