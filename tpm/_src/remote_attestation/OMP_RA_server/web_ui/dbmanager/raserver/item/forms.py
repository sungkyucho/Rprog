from item.models import Item, History, Answer
from django import forms
from django.utils.translation import ugettext_lazy as _
import datetime

class EditForm(forms.ModelForm):
	my_choices = (('9', _('4+5+23')), ('10', _('4+6+23')), ('11', _('5+6+23')),)
	dev_id = forms.CharField(label='dev_id', max_length=16, required=False)
	ml_index = forms.ChoiceField(label='ml_index', choices=my_choices)
	fw_ver = forms.CharField(label='fw_ver', max_length=16)
	class Meta:
		model = Item 
		fields = ['dev_id', 'ml_index', 'fw_ver']
		labels = {
			'dev_id': _('Device ID (Length:16)'),
			'ml_index': _('Measurement List Index (0~22)'),
			'fw_ver': _('Firmware Version of Your Device.'),
		}
		help_texts = {
			'ml_index': _('PCR index of TPM to use Remote Attestation.'),
		}

class RecipeForm(forms.ModelForm):
	my_choices = (('9', _('4+5+23')), ('10', _('4+6+23')), ('11', _('5+6+23')),)
	dev_id = forms.CharField(max_length=16, required=False)
	ml_index = forms.ChoiceField(choices=my_choices)
	fw_ver = forms.CharField(max_length=16)
	class Meta:
		model = Item 
		fields = ['gw_id', 'dev_id', 'ml_index', 'fw_ver']
		labels = {
			'gw_id': _('Gateway ID (Length:16)'),
			'dev_id': _('Device ID (Length:16)'),
			'ml_index': _('Measurement List Index (0~22)'),
			'fw_ver': _('Firmware Version of Your Device.'),
		}
		help_texts = {
			'gw_id': _('You can find Gateway ID from Thing-Plug Page.'),
			'ml_index': _('PCR index of TPM to use Remote Attestation.'),
		}
		error_messages = {
			'gw_id': {
				'max_length': _("This writer's name is too long."),
			},
		}

class AnswerForm(forms.ModelForm):
	class Meta:
		model = Answer 
		fields = ['fw_ver', 'pcr9', 'pcr10', 'pcr11']
		labels = {
			'fw_ver': _('Firmware Version of Your Device.'),
		}

#    gw_id = forms.CharField(label='Gateway ID')
#    dev_id = forms.CharField(label='Device ID') 
#    ml_index = forms.IntegerField(label='Measurement List')
#    fw_ver = forms.CharField(label='Firmware Version') 
