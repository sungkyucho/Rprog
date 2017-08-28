CHECK_LIB= -lcheck -lm -lpthread -lrt
TESTS = check_ra_com check_parse_util check_verify_data check_mariadb_util 
#TESTS += check_base64
check_PROGRAMS = $(TESTS)
check_ra_com_SOURCES = check_ra_com.c  
check_ra_com_SOURCES += ../ra_com.c  ../dbg_log.c 
check_ra_com_CFLAGS = -I$(top_srcdir) -I../include/
check_ra_com_LDADD = $(CHECK_LIB)

check_verify_data_SOURCES = check_verify_data.c  
check_verify_data_SOURCES +=../verify_data.c  ../ra_com.c ../dbg_log.c 
check_verify_data_CFLAGS = -I$(top_srcdir) -I../include/
check_verify_data_LDADD = $(CHECK_LIB)

check_parse_util_SOURCES = check_parse_util.c  
check_parse_util_SOURCES +=../parse_util.c ../dbg_log.c ../ra_com.c 
check_parse_util_CFLAGS = -I$(top_srcdir) -I../include/
check_parse_util_LDADD = $(CHECK_LIB)

check_mariadb_util_SOURCES = check_mariadb_util.c  
check_mariadb_util_SOURCES += ../mariadb_util.c   ../ra_com.c  ../dbg_log.c 
check_mariadb_util_CFLAGS = -I$(top_srcdir) -I../include/ -L/usr/lib64/mysql
check_mariadb_util_LDADD = $(CHECK_LIB) -lmysqlclient
