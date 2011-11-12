from django.test import TestCase
from records.models import record

class RecordsTestCase(TestCase):	
	def test_put(self):
		response = self.client.post('/records/put/', POST='(record r (gid rthief) (uid seklys_morka) (weight 3) (data fhdjh5254557y7765b550fhd5HBe74&*6))', content_type='text', follow=False, HTTP_X_REQUESTED_WITH='XMLHttpRequest')
		self.assertEqual(response.status_code, 200)
		

# run:  python manage.py test records
		

