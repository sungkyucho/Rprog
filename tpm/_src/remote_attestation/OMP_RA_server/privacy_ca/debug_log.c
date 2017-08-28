#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <openssl/rsa.h>
#include <openssl/err.h>
#include <openssl/x509.h>

#include "debug_log.h"

/* For logging messages in PoC system
 * - basic log is written to error log of apache
 * - data should be dumped to explicitly given file
 */


/* FILE* for logging - for each process */
static FILE *lfp = NULL;
static int dlog_level = 0;

/* init. file pointer for logging
 * - use [stderr] when [filename] is not valid
 * - log written to [stderr] will be stored in error log (apache)
 */
void init_dlog(char *lfname)
{
	if ((lfp != NULL) && (lfp != stderr)) {
		fclose(lfp); /* stop previous loggig */
		lfp = stderr; /* (re)set to default */
	} else if (NULL == lfp) {
		lfp = stderr; /* (re)set to default */
	}

	if (lfname != NULL) {
		lfp = fopen(lfname, "w"); /* NOTE: not append */
		if (NULL == lfp) {
			fprintf(stderr, "fopen(%s) failed: %m\n", lfname);
			lfp = stderr; /* (re)set to default */
		} else {
			fprintf(stderr, "logging to <%s>\n", lfname);
		}
	}
}


void fini_dlog(void)
{
	if (lfp != NULL) {
		fflush(lfp);
		if (lfp != stderr)
			fclose(lfp);
	}
	lfp = NULL;
}


FILE *get_lfp(void)
{
	return lfp;
}


int set_level_dlog(char *lvstr, int level)
{
	dlog_level = level; /* default */

	if (NULL == lvstr) {
		return dlog_level;
	}

	if (!strcasecmp("debug", lvstr)) {
		dlog_level = DLOG_DEBUG;
	} else if (!strcasecmp("info", lvstr)) {
		dlog_level = DLOG_INFO;
	} else if (!strcasecmp("noti", lvstr)) {
		dlog_level = DLOG_NOTI;
	} else if (!strcasecmp("warn", lvstr)) {
		dlog_level = DLOG_WARN;
	} else if (!strcasecmp("error", lvstr)) {
		dlog_level = DLOG_ERROR;
	} else if (!strcasecmp("fatal", lvstr)) {
		dlog_level = DLOG_FATAL;
	} /* else - default */

	return dlog_level;
}


int get_level_dlog(void)
{
	return dlog_level;
}


void dlog(int level, char *fmt, ...)
{
	va_list ap;

	if (lfp && (level >= dlog_level)) {
		va_start(ap, fmt);
		//fprintf(lfp, "Lv[%d] ", level);
		vfprintf(lfp, fmt, ap);
		va_end(ap);
	}
}


/* return # of written, -1 on error
 * NOTE - return error, if [fname] is not valid file
 */
int dump_file(char *cbuf, int size, char *fname)
{
	int ofd = -1, wcnt;

	if (size < 0) {
		fprintf(lfp, "Invalid dumping size<%d>\n", size);
		return -1; /* error */
	}

	if (fname != NULL) {
		ofd = open(fname, O_CREAT|O_WRONLY|O_TRUNC, 0644);
		if (ofd < 0) {
			fprintf(lfp, "open(%s) failed: %m\n", fname);
		}
	}

	if (ofd >= 0) {
		wcnt = write(ofd, cbuf, size);
		if (wcnt != size) {
			fprintf(lfp, "incomplete writing <%d/%d>\n", wcnt, size);
		}

		close(ofd);
		fsync(ofd);
	}

	return wcnt;
}


void hex_dump_fp(char *hexstr, int slen, FILE *ofp)
{
	int i;
	int turn = 20; /* adding newline */

	if ((NULL == ofp) || (NULL == hexstr) || (1 > slen)) {
		fprintf(lfp, "invalid argument(s) - hex_dump_fp(%p, %d, %p)\n", hexstr, slen, ofp);
		return;
	}

	fprintf(ofp, "[HEX<%p:%d>]-----------------S-", hexstr, slen);
	for (i = 0; i < slen; i++) {
		if ((i % turn) == 0)
			fprintf(ofp, "\n");
		fprintf(ofp, "%02hhx ", hexstr[i]);
	}
	fprintf(ofp, "\n[HEX<%p:%d>]-----------------F-\n", hexstr, slen);
}


void hex_dump_dlog(int level, char *hexstr, int slen)
{
	if (lfp && (level >= dlog_level)) {
		hex_dump_fp(hexstr, slen, lfp);
	}
}


void ERR_print_errors_dlog(void)
{
	if (lfp) {
		fprintf(lfp, "[%s] HERE\n", __func__); /* debug */
		ERR_print_errors_fp(lfp);
	}
}


/* return 1 on success, 0 on error */
int X509_print_dlog(int level, X509 *cert)
{
	if (lfp && (level >= dlog_level)) {
		return X509_print_fp(lfp, cert);
	}

	return 0;
}


/* return 1 on success, 0 on error */
int RSA_print_dlog(int level, RSA *x, int offset)
{
	if (lfp && (level >= dlog_level)) {
		return RSA_print_fp(lfp, x, offset);
	}

	return 0;
}

