from django.conf.urls import url
from item import views
from item.models import Item 
from item.views import DeleteItem, DeleteAnswer

urlpatterns = [
    url(r'^$', views.ra_result, name='raresult'),
    url(r'^resetpcr/(?P<uuid>([0-9a-zA-Z]{16}))', views.resetpcr, name='pcrreset'),
    url(r'^rahistory/', views.ra_history, name='rahistory'),
    url(r'^answer/', views.ra_answer, name='raanswer'),
    url(r'^register/', views.register, name='register'),
    url(r'^tp_device/', views.tp_device, name='tp_device'),
#   url(r'^(?P<ra_id>([0-9a-zA-Z]{16}))/$', views.ra_list, name='ralist'),
    url(r'^(?P<ra_id>([0-9a-zA-Z]{10,16}))/$', views.ra_edit, name='ralist'),
	url(r'^ajax_status/', views.ajax_status, name='ajax_status'),
	url(r'^ajax_req/', views.ajax_req, name='ajax_req'),
    url(r'^addra/', views.addra, name='addra'),
    url(r'^addans/', views.addans, name='addanswer'),
   url(r'^editra/(?P<ra_id>([0-9a-zA-Z]{8,16}))/$', views.editra, name='editra'),
    url(r'^delete/(?P<pk>([0-9a-zA-Z]{10,16}))/$', DeleteItem.as_view(), name='delete_item'),
    url(r'^deleteans/(?P<pk>([0-9\.]{2,10}))/$', DeleteAnswer.as_view(), name='delete_ans'),
]
