from django.db import models
import datetime

ID_LENGTH=50

class record(models.Model):
	gid = models.CharField(max_length=ID_LENGTH)
	uid = models.CharField(max_length=ID_LENGTH)
	time = models.DateTimeField('date published')
	weight = models.IntegerField()
	data = models.TextField()

	def __unicode__(self):
		return self.uid


