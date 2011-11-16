from django.db import models
import datetime

class record(models.Model):
	gid = models.CharField(max_length=50)
	uid = models.CharField(max_length=50)
	time = models.DateTimeField('date published')
	weight = models.IntegerField()
	data = models.TextField()

	def __unicode__(self):
		return self.uid


