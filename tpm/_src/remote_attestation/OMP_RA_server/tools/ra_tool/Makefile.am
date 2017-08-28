AM_CFLAGS = -O2 -g -Wall -I../../tp_ralib
AM_LDFLAGS = -lcurl 
bin_PROGRAMS=ra_tool_tp del_gw_tp 

confdir=/etc/conf
conf_DATA=tpconf.conf

ra_tool_tp_SOURCES=ra_tool_tp.c test_util.c 
ra_tool_tp_LDADD= ../../tp_ralib/libtp_ra.a

del_gw_tp_SOURCES=del_gw_tp.c test_util.c apr_base64.c 
