#ifndef	_DEBUG_LOG_H_
#define	_DEBUG_LOG_H_

#include <stdio.h>
#include <openssl/x509.h>
#include <openssl/rsa.h>

#define	DLOG_DEBUG	0
#define	DLOG_INFO	10
#define	DLOG_NOTI	20
#define	DLOG_WARN	30
#define	DLOG_ERROR	40
#define	DLOG_FATAL	100

void init_dlog(char *lfname);
void fini_dlog(void);
FILE *get_lfp(void);
int set_level_dlog(char *lvstr, int level);
int get_level_dlog(void);
void dlog(int level, char *fmt, ...);
/* return # of written, -1 on error */
int dump_file(char *cbuf, int size, char *fname);
void hex_dump_fp(char *hexstr, int slen, FILE *ofp);
void hex_dump_dlog(int level, char *hexstr, int slen);
void ERR_print_errors_dlog(void);
/* return 1 on success, 0 on error */
int X509_print_dlog(int level, X509 *cert);
/* return 1 on success, 0 on error */
int RSA_print_dlog(int level, RSA *x, int offset);

#endif /* _DEBUG_LOG_H_ */
