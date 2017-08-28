from django.conf.urls import include, url
from django.contrib.auth import views
from django.contrib import admin

urlpatterns = [
    url(r'^admin/', include(admin.site.urls)),
    url(r'^accounts/', include('accounts.urls', namespace='accounts')),
    url(r'^', include('item.urls')),
    url(r'^ralist/', include('item.urls')),
]
