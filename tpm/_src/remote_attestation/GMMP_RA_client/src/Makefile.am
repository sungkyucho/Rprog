AM_CFLAGS = -O2 -Wall -I../GMMP_lib/ -I../GMMP_lib/Define/ -I../GMMP_lib/ErrorCode/ \
			-I../GMMP_lib/Log/ -I../GMMP_lib/Network/ -I../GMMP_lib/Operation/ \
			-I../GMMP_lib/Operation/Control/ -I../GMMP_lib/Operation/Delivery/ \
			-I../GMMP_lib/Operation/Encrypt/ -I../GMMP_lib/Operation/FTP/ \
			-I../GMMP_lib/Operation/Heartbeat/ -I../GMMP_lib/Operation/LOB/ \
			-I../GMMP_lib/Operation/LongSentence/ -I../GMMP_lib/Operation/Multimedia/ \
			-I../GMMP_lib/Operation/Notification/ -I../GMMP_lib/Operation/ProfileInfo/ \
			-I../GMMP_lib/Operation/Reg/ -I../GMMP_lib/Operation/Remote/ \
			-I../GMMP_lib/Util/ -I../ralib/include/

bin_PROGRAMS = tp_attestant dummy_client show_gw
tp_attestant_SOURCES = Sample_All.c
tp_attestant_LDADD = ../GMMP_lib/libGMMP.la ../ralib/libra.la
dummy_client_SOURCES = dummy_gw.c
dummy_client_LDADD = ../GMMP_lib/libGMMP.la
show_gw_SOURCES = show_gw.c
show_gw_LDADD = ../GMMP_lib/libGMMP.la
