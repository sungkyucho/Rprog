SUBDIRS = Control  Delivery  Encrypt  FTP  Heartbeat  LOB  LongSentence  Multimedia  Notification  ProfileInfo  Reg  Remote  

#AM_LDFLAGS = -lpthread -lcrypto 
AM_CFLAGS = -O2 -Wall -I./Control -I./Delivery -I./Encrypt -I./FTP -I./Heartbeat -I./LOB -I./LongSentence -I./Multimedia -I./Notification -I./ProfileInfo -I./Reg -I./Remote  

noinst_LTLIBRARIES = libGMMP_Operation.la 
libGMMP_Operation_la_SOURCES = GMMP_Operation.c
libGMMP_Operation_la_LIBADD = ./Control/libGMMP_Control.la ./Delivery/libGMMP_Delivery.la \
							./Encrypt/libGMMP_Encrypt.la ./FTP/libGMMP_FTP.la \
							./Heartbeat/libGMMP_Heartbeat.la ./LOB/libGMMP_LOB.la \
							./LongSentence/libGMMP_LSentence.la ./Multimedia/libGMMP_Multimedia.la \
							./Notification/libGMMP_Notification.la ./ProfileInfo/libGMMP_ProfileInfo.la \
							./Reg/libGMMP_Reg.la ./Remote/libGMMP_Remote.la
if RPI 
endif 
