#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT(GMMP_Sample, 1.0, kkim@securityplatform.co.kr)
AC_CONFIG_SRCDIR([src/Sample_Registration_Encryption.c])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
LT_INIT([])
AC_PROG_LIBTOOL

AC_SEARCH_LIBS([floor], [m], [], [
    AC_MSG_ERROR([unable to find floor() function])
])

AC_SEARCH_LIBS([pthread_create], [pthread], [], [
    AC_MSG_ERROR([unable to find pthread_create() function])
])

AC_SEARCH_LIBS([timer_create], [rt], [], [
    AC_MSG_ERROR([unable to find timer_create() function])
])

AC_SEARCH_LIBS([tcase_create], [check], [], [
    AC_MSG_ERROR([unable to find tcase_create() function])
])

AC_SEARCH_LIBS([json_object_from_file], [json-c], [], [
    AC_MSG_ERROR([unable to find json_object_from_file() function])
])

AC_SEARCH_LIBS([BIO_new], [crypto], [], [
    AC_MSG_ERROR([unable to find BIO_new() function])
])

AC_SEARCH_LIBS([RSA_new], [crypto], [], [
    AC_MSG_ERROR([unable to find RSA_new() function])
])

AC_SEARCH_LIBS([Tspi_Context_Create], [tspi], [], [
    AC_MSG_ERROR([unable to find Tspi_Context_Create() function])
])

AC_CONFIG_MACRO_DIR([m4])

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h fcntl.h stdlib.h string.h sys/socket.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_TYPE_MODE_T
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([gettimeofday memset socket])

AC_CHECK_PROGS([DOXYGEN], [doxygen])
if test -z "$DOXYGEN";
    then AC_MSG_WARN([Doxygen not found - continuing without Doxygen support])
fi

AM_CONDITIONAL([HAVE_DOXYGEN], [test -n "$DOXYGEN"])
AM_COND_IF([HAVE_DOXYGEN], [AC_CONFIG_FILES([Doxyfile])])

AC_OUTPUT(  Makefile
	    src/Makefile
	    GMMP_lib/Makefile
	    tests/Makefile
	    samples/Makefile
	    )
#	    GMMP_lib/Network/Makefile
#	    GMMP_lib/Operation/Makefile
#	    GMMP_lib/Operation/Control/Makefile
#	    GMMP_lib/Operation/Delivery/Makefile
#	    GMMP_lib/Operation/Encrypt/Makefile
#	    GMMP_lib/Operation/FTP/Makefile
#	    GMMP_lib/Operation/Heartbeat/Makefile
#	    GMMP_lib/Operation/LOB/Makefile
#	    GMMP_lib/Operation/LongSentence/Makefile
#	    GMMP_lib/Operation/Multimedia/Makefile
#	    GMMP_lib/Operation/Notification/Makefile
#	    GMMP_lib/Operation/ProfileInfo/Makefile
#	    GMMP_lib/Operation/Reg/Makefile
#	    GMMP_lib/Operation/Remote/Makefile
#	    GMMP_lib/Util/Makefile
