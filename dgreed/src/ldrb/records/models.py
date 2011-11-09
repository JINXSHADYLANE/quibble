from django.db import models
import base64
import datetime

class record(models.Model):
	gid = models.CharField(max_length=50)
	uid = models.CharField(max_length=50)
	time = models.DateTimeField('date published')
	weight = models.IntegerField()
	data = models.TextField()

	'''def set_data(self, data):
		self._data = base64.encodestring(data)
	def get_data(self):
		return base64.decodestring(self._data)
	data = property(get_data, set_data)'''

	def __unicode__(self):
		return self.uid


'''class log(models.Model):
	gid = models.CharField(max_length=10)
	uid = models.CharField(max_length=20)
	time = models.DateTimeField('date published')
	action = models.CharField(max_length=50)
	def __unicode__(self):
		return self.action'''


