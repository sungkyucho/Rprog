from django.shortcuts import render, redirect
from django.core.urlresolvers import reverse_lazy
from django.http import Http404
from item.models import Item, History, Answer
from item.forms import RecipeForm, EditForm, AnswerForm
from django.views.generic import DeleteView 
from django.core import serializers
from django.http import JsonResponse, HttpResponse
import socket
import subprocess
import os


def read_conf():
	host = None
	port = None
	devinfo = []
	with open('pcrcontroller/device.conf') as f:
		for line in f:
			devinfo.append([n for n in line.strip().split(':')])
	for pair in devinfo:
		try:
			if pair[0] == 'host':
				host = pair[1]
			elif pair[0] == 'port':
				port = pair[1]	
		except :
			print('configure is not valid')

	return host, port

def ra_result(request):
    """Pull the current list of all ra result and return them.
    """
    raresult = Item.objects.all().order_by('gw_id')
    context = {'raresult': raresult}
#    return render(request, 'radashboard.html', context)
    return render(request, 'raresult.html', context)

def ra_history(request):
	if request.method == 'POST':
		if 'gw_id' in request.POST:
			raresult = History.objects.filter(gw_id=request.POST.get('gw_id')).order_by('-id')[:500]
		else:
			raresult = []
	else:
		raresult = History.objects.all().order_by('-id')[:500]
	context = {'raresult': raresult}
	return render(request, 'rahistory.html', context)

def ra_answer(request):
    raanswer = Answer.objects.all().order_by('fw_ver')[:500]
    context = {'raanswer': raanswer}
    return render(request, 'raanswer.html', context)

def register(request):
    context = {'':''}
    return render(request, 'raregit.html', context)

def tp_device(request):
	if request.method == 'POST':
		if 'add' in request.POST: 
			vadd = request.POST.get('add') 
			if vadd is not None and vadd != '':
				popen = subprocess.Popen(['/usr/local/bin/ra_tool_tp', '-a', vadd, '-c', '/etc/conf/tpconf.conf'], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
				(stdoutdata, stderrdata) = popen.communicate()
			else:
				stdoutdata = 'invalid input'
			context = {'result': stdoutdata}
		elif 'del_gwid' in request.POST:
			vdel_gwid = request.POST.get('del_gwid')
			vdel_authid = request.POST.get('del_authid')
			vdel_authkey = request.POST.get('del_authkey') 
			if vdel_gwid is not None and vdel_gwid != '' and vdel_authid is not None and vdel_authid != '' and vdel_authkey is not None and vdel_authkey != '':
				popen = subprocess.Popen(['/usr/local/bin/del_gw_tp', '-c', '/etc/conf/tpconf.conf', vdel_gwid, vdel_authid, vdel_authkey], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
				(stdoutdata, stderrdata) = popen.communicate()
			else:
				stdoutdata = 'invalid input'
			context = {'result': stdoutdata}
		elif 'check' in request.POST:
			vcheck = request.POST.get('check')
			if vcheck is not None and vcheck != '': 
				popen = subprocess.Popen(['/usr/local/bin/ra_tool_tp', '-s', vcheck, '-c', '/etc/conf/tpconf.conf'], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
				(stdoutdata, stderrdata) = popen.communicate()
			else:
				stdoutdata = 'invalid input'
			context = {'result': stdoutdata}
		else:
			context = {'result':'error'}
		return render(request, 'raregit.html', context)
	else:
		context = {'result':'error'}
		return render(request, 'raregit.html', context)


def ra_list(request, ra_id):
	"""Render a specific ra_list, selected by ID 
	"""
	try:
		ra_list = Item.objects.get(gw_id=ra_id)
	except Item.DoesNotExist:
		ra_list = None

	try:
		ra_history= History.objects.filter(gw_id=ra_id).order_by('-id')[:500]
	except History.DoesNotExist:
		ra_history = None 

	try:
		ra_answer = Answer.objects.get(id=1)
	except Answer.DoesNotExist:
		ra_answer = None 

	context = {'ra_list': ra_list, 'ra_history':ra_history, 'ra_answer': ra_answer}
	return render(request, 'ralist.html', context)

def ra_edit(request, ra_id):
	if ra_id:
		try:
			ra_list = Item.objects.get(gw_id=ra_id)
		except Item.DoesNotExist:
			ra_list = None

		form = EditForm()
		context = {'ra_list': ra_list, 'form': form, 'ra_id': ra_id}
		return render(request, 'editra.html', context) 
		
	else:
		raresult = Item.objects.all().order_by('gw_id')[:100]
		context = {'raresult': raresult}
		return render(request, 'raresult.html', context) 


def resetpcr(request, uuid):
	try:
	    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	except socket.error:
	    print('Failed to create socket')
	     
	print('# Getting remote IP address') 
	host, port = read_conf()

	if host and port :
		try:
			s.connect((host, int(port)))
		except socket.error, exc:
			print "Caught exception socket.error : %s" % exc
			return redirect(ra_list, ra_id=uuid)

		print('# Sending data to server')
		try:
			s.sendall("RST16")
		except socket.error, exc:
			print "Caught exception socket.error : %s" % exc
			return redirect(ra_list, ra_id=uuid)

		reply = s.recv(4096)
		print reply
	else:
		print('invalid configure data')

	s.close()
	return redirect(ra_list, ra_id=uuid)

def addra(request):
	if request.method == 'POST':
		form = RecipeForm(request.POST)
		if form.is_valid():
			data = form.cleaned_data
			r = Item()
			r.gw_id = data['gw_id']
			r.dev_id = data['dev_id']
			r.ml_index = data['ml_index']
			r.fw_ver = data['fw_ver']
			r.save()
			return redirect(ra_result)
		else:
			form = RecipeForm()
			return render(request, 'addra.html', {'result': 'error', 'form':form})
	else:
		form = RecipeForm()
	return render(request, 'addra.html', {'form': form})

def addans(request):
    if request.method == 'POST':
        form = AnswerForm(request.POST)
        if form.is_valid():
            data = form.cleaned_data
            r = Answer()
            r.fw_ver = data['fw_ver']
            r.pcr9 = data['pcr9']
            r.pcr10 = data['pcr10']
            r.pcr11 = data['pcr11']
            r.save()
            return redirect(ra_answer)
    else:
        form = AnswerForm()
    return render(request, 'addanswer.html', {'form': form})

def editra(request, ra_id):
	if request.method == 'POST':
		r = Item.objects.get(gw_id=ra_id)

		form = EditForm(request.POST or None, instance=r)
		form.fields['dev_id'].required = False
		if form.is_valid():
			data = form.cleaned_data or None
			#r.gw_id = data['gw_id']
			r.dev_id = data['dev_id']
			r.ml_index = data['ml_index']
			r.fw_ver = data['fw_ver']
			r.save()
			return redirect(ra_result)
		else:
			form = RecipeForm()
			return render(request, 'editra.html', {'result': 'error', 'form':form})
	else:
		form = RecipeForm()
	return render(request, 'editra.html', {'form': form})

class DeleteItem(DeleteView):
	model = Item
	success_url = reverse_lazy('raresult')

class DeleteAnswer(DeleteView):
	model = Answer 
	success_url = reverse_lazy('raanswer')

def ajax_status(request):
	raresult = Item.objects.all()

	if not raresult :
		data = {'raresult': []}
		return JsonResponse(data)
	else:
		return HttpResponse(serializers.serialize('json', raresult), content_type="application/json")


def ajax_req(request):
	return JsonResponse({'result':'success'})
