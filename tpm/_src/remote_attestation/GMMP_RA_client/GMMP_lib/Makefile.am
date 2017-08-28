SUBDIRS = ErrorCode  Log  Network  Operation  Util

lib_LTLIBRARIES = libGMMP.la 
libGMMP_la_SOURCES = GMMP.c
libGMMP_la_LIBADD = ./ErrorCode/liberrorCode.la ./Log/liblog.la \
					./Network/libnetwork.la ./Util/libutil.la \
					./Operation/libGMMP_Operation.la 
libGMMP_la_LDFLAGS = -lpthread -lcrypto -ltspi -lcurl
libGMMP_la_CFLAGS = -O2 -Wall -I./Define/  -I./ErrorCode -I./Log -I./Network -I./Operation -I./Util
if RPI 
endif
