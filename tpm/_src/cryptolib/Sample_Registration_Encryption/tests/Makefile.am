CHECK_LIB= -lcheck -lm -lpthread -lrt
AUTOMAKE_OPTIONS = subdir-objects
CLEANFILES = newsecret.key.blob secret.key.blob
TESTS = check_setting1
TESTS += check_encryption check_sign
TESTS += check_base64
TESTS += check_enc_setting
TESTS += check_sign_tpm
TESTS += check_secretkey
check_PROGRAMS = $(TESTS)
check_setting1_SOURCES = check_setting1.c
check_setting1_CFLAGS = -I$(top_srcdir) -I$(top_srcdir)/src
check_setting1_LDADD = -L$(top_srcdir)/src -lsettings -ljson-c -lcrypto
check_setting1_LDADD += $(CHECK_LIB)

check_secretkey_SOURCES = check_secretkey.c
check_secretkey_CFLAGS = -I$(top_srcdir) -I$(top_srcdir)/src
check_secretkey_LDADD = -L$(top_srcdir)/src -lsettings -ljson-c -lcrypto
check_secretkey_LDADD += $(CHECK_LIB)

check_encryption_SOURCES = check_encryption.c
check_encryption_CFLAGS = -I$(top_srcdir) -I$(top_srcdir)/src
check_encryption_LDADD = -L$(top_srcdir)/src -lsettings -ljson-c -lcrypto
check_encryption_LDADD += $(CHECK_LIB)

check_sign_SOURCES = check_sign.c
check_sign_CFLAGS = -I$(top_srcdir) -I$(top_srcdir)/src
check_sign_LDADD = -L$(top_srcdir)/src -lsettings -ljson-c -lcrypto
check_sign_LDADD += $(CHECK_LIB)

check_enc_setting_SOURCES = check_enc_setting.c
check_enc_setting_CFLAGS = -I$(top_srcdir) -I$(top_srcdir)/src
check_enc_setting_LDADD = -L$(top_srcdir)/src -lsettings -ljson-c -lcrypto
check_enc_setting_LDADD += $(CHECK_LIB)

check_base64_SOURCES = check_base64.c
check_base64_CFLAGS = -I$(top_srcdir) -I$(top_srcdir)/src
check_base64_LDADD = -L$(top_srcdir)/src -lsettings -ljson-c -lcrypto
check_base64_LDADD += $(CHECK_LIB)

check_sign_tpm_SOURCES = check_sign_tpm.c
check_sign_tpm_CFLAGS = -I$(top_srcdir) -I$(top_srcdir)/src
check_sign_tpm_LDADD = -L$(top_srcdir)/src -lsettings -ljson-c -lcrypto -ltspi
check_sign_tpm_LDADD += $(CHECK_LIB)
