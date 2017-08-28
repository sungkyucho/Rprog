# coding=utf-8
from django.db import models
from django.contrib.auth.models import User
from django.core.validators import MaxValueValidator, MinValueValidator

class UserProfile(models.Model):
    user = models.OneToOneField(User)
    phone = models.CharField(
        max_length=12,
        blank=True,
    )

    def __unicode__(self):
        return self.user.username

class Item(models.Model):
#	gw_name = models.CharField(max_length=64)
	gw_id = models.CharField(max_length=16, primary_key=True)
	dev_id = models.CharField(max_length=16, null=True, blank=True)
	ml_index = models.PositiveSmallIntegerField(blank=True, null=True,
				validators=[MaxValueValidator(22), MinValueValidator(0)] )
	nonce = models.CharField(max_length=40) 
	fw_ver= models.CharField(max_length=10) 
	rsp_time = models.DateTimeField(blank=True, null=True)
	ra_result = models.TextField()
	ra_reason = models.IntegerField(blank=True, null=True)
#	mac_addr = models.CharField(max_length=17)
#	ip = models.CharField(max_length=15, blank=True, null=True)
#	port = models.IntegerField(blank=True, null=True)
#	s_time = models.DateTimeField(blank=True, null=True)
#	retry_cnt = models.IntegerField(blank=True, null=True)
#	recv_pcr = models.CharField(max_length=40, blank=True, null=True)
#	citizen = models.TextField(blank=True, null=True)  # This field type is a guess.

	@property
	def checkbox_character(self):
		return '☑' if self.ra_result else '☐'

	@property
	def check_result(self):
		return 'SUCCESS' if self.ra_result and self.ra_result[3] =='1' else 'FAILED'
	@property
	def check_reason(self):
		if self.ra_reason == -1:
			return 'UNINITIALIZED'
		elif self.ra_reason == 1:
			return 'OMP ACCEPT'
		elif self.ra_reason == 2:
			return 'FAILED TO SEND CMD TO CLIENT'
		elif self.ra_reason == 3:
			return 'ACK FROM CLIENT'
		elif self.ra_reason == 4:
			return 'RA TIMEOUT'
		elif self.ra_reason == 5:
			return 'FAILED WITH UNKNOWN REASON'
		
	@property
	def get_ml_index(self):
		return  "{0:b}".format(self.ml_index) if self.ml_index else '-'

	class Meta:
#		managed = False
		db_table = 'ra_item'

class History(models.Model):
	id = models.AutoField(primary_key=True)
	gw_id = models.CharField(max_length=16)
	dev_id = models.CharField(max_length=16, null=True, blank=True)
	rsp_time = models.DateTimeField(blank=True, null=True)
	ra_result = models.TextField()
	ra_reason = models.IntegerField(blank=True, null=True)
#	gw_name = models.CharField(max_length=64)
#	mac_addr = models.CharField(max_length=17)
#	ip = models.CharField(max_length=15, blank=True, null=True)
#	port = models.IntegerField(blank=True, null=True)
#	s_time = models.DateTimeField(blank=True, null=True)
#	ml_index = models.IntegerField(blank=True, null=True)
#	retry_cnt = models.IntegerField(blank=True, null=True)
#	recv_pcr = models.CharField(max_length=40, blank=True, null=True)
#	citizen = models.TextField(blank=True, null=True)  # This field type is a guess.

	@property
	def checkbox_character(self):
		return '☑' if self.ra_result else '☐'

	@property
	def check_result(self):
		return 'SUCCESS' if self.ra_result and self.ra_result[3] =='1' else 'FAILED'
	@property
	def check_reason(self):
		if self.ra_reason == -1:
			return 'UNINITIALIZED'
		elif self.ra_reason == 1:
			return 'OMP ACCEPT'
		elif self.ra_reason == 2:
			return 'FAILED TO SEND CMD TO CLIENT'
		elif self.ra_reason == 3:
			return 'ACK FROM CLIENT'
		elif self.ra_reason == 4:
			return 'RA TIMEOUT'
		elif self.ra_reason == 5:
			return 'FAILED WITH UNKNOWN REASON'
	@property
	def check_verify(self):
		return 'SUCCESS' if self.ra_result and self.ra_result[0] =='1' else 'FAILED'
	@property
	def check_nonce(self):
		return 'SUCCESS' if self.ra_result and self.ra_result[1] =='1' else 'FAILED'
	@property
	def check_pcr(self):
		return 'SUCCESS' if self.ra_result and self.ra_result[2] =='1' else 'FAILED'
	@property
	def get_ml_index(self):
		return  "{0:b}".format(self.ml_index) if self.ml_index else '-'

	class Meta:
#		managed = True 
		db_table = 'ra_history'

class Answer(models.Model):
	fw_ver= models.CharField(max_length=10, primary_key=True)
	pcr0 = models.CharField(max_length=40, blank=True, null=True) 
	pcr1 = models.CharField(max_length=40, blank=True, null=True) 
	pcr2 = models.CharField(max_length=40, blank=True, null=True) 
	pcr3 = models.CharField(max_length=40, blank=True, null=True)  #use
	pcr4 = models.CharField(max_length=40, blank=True, null=True)  #use
	pcr5 = models.CharField(max_length=40, blank=True, null=True)  #use
	pcr6 = models.CharField(max_length=40, blank=True, null=True) 
	pcr7 = models.CharField(max_length=40, blank=True, null=True) 
	pcr8 = models.CharField(max_length=40, blank=True, null=True) 
	pcr9 = models.CharField(max_length=40, blank=True, null=True) 
	pcr10 = models.CharField(max_length=40, blank=True, null=True) #use 
	pcr11 = models.CharField(max_length=40, blank=True, null=True)
	pcr23= models.CharField(max_length=40, blank=True, null=True)  #use

	class Meta:
#		managed = False
		db_table = 'ra_answer'
