SUBDIRS=tests
AM_CFLAGS = -O2 -Wall -I./include/  -I../tp_ralib  -L/usr/lib64/mysql
AM_LDFLAGS = -lcrypto -ltspi -lcurl
bin_PROGRAMS=TPServer 

confdir=/etc/conf
conf_DATA = conf/tp_server.conf 
initconfdir=/etc/init
initconf_DATA=conf/uwsgi.conf
iniconfdir=/etc/uwsgi/vassals
iniconf_DATA=conf/sp_django.ini

TPServer_SOURCES= tp_server.c parse_util.c ra_com.c verify_data.c socket_util.c mariadb_util.c tp_mariadb_util.c dbg_log.c timer.c 
TPServer_LDADD= -lrt -lmysqlclient -lpthread ../tp_ralib/libtp_ra.a 
