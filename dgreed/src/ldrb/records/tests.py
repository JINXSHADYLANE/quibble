from django.test import TestCase
from records.models import record
from random import *
import datetime


def setUpDB(n):
	Entries = []
	seed(datetime.datetime.now())
	for i in range(n):
		rDate = datetime.datetime(2011, int(random()*10)+1, int(random()*27)+1, int(random()*23)+1, int(random()*59)+1, int(random()*59)+1 )
		Entries.append( record(gid='rthief', uid='dwarf01', weight=int(random()*50), time=rDate, data='adksf6565d55dddd5d64csd5sd4')  )		
	for e in Entries:
		e.save()

class RecordsTestCase(TestCase):	

	def test_put0(self):  # good dataString
		setUpDB(3)
		#print 'PUT TEST'
		dataString = '(entry _ (gid plesiku_urvas) (uid seklys_morka) (weight 3) (data fhdjh5254557y7765b550fhd5HBe74&*6))'
		response = self.client.post('/records/put/', raw_post_data=dataString, mimetype='text/plain', follow=False, HTTP_X_REQUESTED_WITH='XMLHttpRequest')
		#for i in record.objects.values_list():
			#print i
		#print '\n\n'
		self.assertEqual(response.status_code, 201)

	def test_put1(self):  # good dataString
		setUpDB(3)
		dataString = '(entry _ (gid guitarLoser) (uid prawn33) (weight 559) (data fhdjh5254557y7765b550fhd5HBe74&*6))'
		response = self.client.post('/records/put/', raw_post_data=dataString, mimetype='text/plain', follow=False, HTTP_X_REQUESTED_WITH='XMLHttpRequest')
		self.assertEqual(response.status_code, 201)

	def test_put2(self): # no dataString
		setUpDB(3)
		response = self.client.post('/records/put/' , mimetype='text/plain', follow=False, HTTP_X_REQUESTED_WITH='XMLHttpRequest')
		self.assertEqual(response.status_code, 400)


	def test_put3(self): # not enough fields
		setUpDB(3)
		dataString = '(entry _ (gid miau!) (weight 32800) (data fhdjh5254557y7765b550fhd5HBe74&*6))'
		response = self.client.post('/records/put/', raw_post_data=dataString, mimetype='text/plain', follow=False, HTTP_X_REQUESTED_WITH='XMLHttpRequest')
		self.assertEqual(response.status_code, 400)

	def test_put4(self): # messy dataString
		setUpDB(3)
		dataString = '(entry _ (gid miau! (uid seklys_morka) [weight 32800] (data fhdjh5254557y7765b550fhd5HBe74&*6) (baubas))'
		response = self.client.post('/records/put/', raw_post_data=dataString, mimetype='text/plain', follow=False, HTTP_X_REQUESTED_WITH='XMLHttpRequest')
		self.assertEqual(response.status_code, 400)

	def test_put5(self): # empty dataString
		setUpDB(3)
		dataString = ''
		response = self.client.post('/records/put/', raw_post_data=dataString, mimetype='text/plain', follow=False, HTTP_X_REQUESTED_WITH='XMLHttpRequest')
		self.assertEqual(response.status_code, 400)


	def test_query(self):
		setUpDB(10)
		#print 'QUERY TEST'
		queryString0 = '(query _ (gid rthief) (startEntry 0) (entryCount 10) (startTime 20111116113710) (endTime 20111116133710))'
		queryString1 = '(query _ (gid rthief) (startEntry 0) (entryCount 14) (startTime 20110716113710) (endTime 20111016133710))'
		queryString2 = '(query _ (gid rthief) (startEntry 0) (entryCount 2) (startTime 20110716113710) (endTime 20110716113710))'
		queryString3 = '(query _ (gid rthief) (startEntry 0) (entryCount 2) (endTime 20110716113710))'

		response = self.client.get('/records/query/', QUERY_STRING=queryString0, mimetype='text/plain', follow=False, HTTP_X_REQUESTED_WITH='XMLHttpRequest')

		print 'query response:\n', response
		self.assertEqual(response.status_code, 201)

# run:  python manage.py test records
		

