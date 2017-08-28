#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.63])
AC_INIT(RA_client_suite, 1.0, kang@securityplatform.co.kr)
AC_CONFIG_SRCDIR([ralib])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
LT_INIT([])
AC_PROG_LIBTOOL

case "$host" in 
arm*-*-*-*) TARGET=ARM;; 
x86_64-*-*) TARGET=X86;; 
esac

AM_CONDITIONAL([RPI], [test x$TARGET = xARM])
AM_CONDITIONAL([X86], [test x$TARGET = xX86])

AC_SEARCH_LIBS([floor], [m], [], [
    AC_MSG_ERROR([unable to find floor() function])
])

AC_SEARCH_LIBS([pthread_create], [pthread], [], [
    AC_MSG_ERROR([unable to find pthread_create() function])
])

AC_SEARCH_LIBS([timer_create], [rt], [], [
    AC_MSG_ERROR([unable to find timer_create() function])
])

AC_SEARCH_LIBS([BIO_new], [crypto], [], [
    AC_MSG_ERROR([unable to find BIO_new() function])
])

#AC_SEARCH_LIBS([BIO_new], [ssl], [], [
#    AC_MSG_ERROR([unable to find BIO_new() function])
#])

AC_SEARCH_LIBS([Tspi_Context_Create], [tspi], [], [
    AC_MSG_ERROR([unable to find Tspi_Context_Create() function])
])

if test $TARGET = xX86; then 
AC_SEARCH_LIBS([tcase_create], [check], [], [
    AC_MSG_ERROR([unable to find tcase_create() function])
])

AC_SEARCH_LIBS([mysql_query], [mariadbclient_r], [], [
    AC_MSG_ERROR([unable to find mysql_query() function])
])
fi 

AC_SEARCH_LIBS([curl_easy_init], [curl], [], [
    AC_MSG_ERROR([unable to find curl_easy_init() function])
])

AC_CONFIG_MACRO_DIR([m4])

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h fcntl.h stdlib.h string.h sys/socket.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
#AC_CHECK_HEADER_STDBOOL
AC_TYPE_MODE_T
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([gettimeofday memset socket])

AC_OUTPUT(  Makefile
		ralib/Makefile
		GMMP_lib/Makefile
		GMMP_lib/ErrorCode/Makefile
		GMMP_lib/Log/Makefile
		GMMP_lib/Network/Makefile
		GMMP_lib/Util/Makefile
		GMMP_lib/Operation/Makefile
		GMMP_lib/Operation/Control/Makefile
		GMMP_lib/Operation/Delivery/Makefile
		GMMP_lib/Operation/Encrypt/Makefile
		GMMP_lib/Operation/FTP/Makefile
		GMMP_lib/Operation/Heartbeat/Makefile
		GMMP_lib/Operation/LOB/Makefile
		GMMP_lib/Operation/LongSentence/Makefile
		GMMP_lib/Operation/Multimedia/Makefile
		GMMP_lib/Operation/Notification/Makefile
		GMMP_lib/Operation/ProfileInfo/Makefile
		GMMP_lib/Operation/Reg/Makefile
		GMMP_lib/Operation/Remote/Makefile
		src/Makefile
	    )
