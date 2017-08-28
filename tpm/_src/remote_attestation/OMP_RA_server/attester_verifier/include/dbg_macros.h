#ifndef _DBG_MACROS_H_
#define _DBG_MACROS_H_

#include <stdio.h>
#include <assert.h>
#include <syslog.h>
#include <stdarg.h>
#include <stdint.h>
#include <tss/tspi.h>
#include <trousers/trousers.h>
#include <tss/tddl_error.h>
#include <tss/tcs_error.h>

#define DDBG   0x01
#define DINFO  0x02
#define DERROR 0x04
#define DCRIT  0x08

extern int dbg_lv;

/*
#define print_error(function, result) \
	do { \
		printf("\t0 FAIL  :  %s  returned (%d) %s\n", function, result, err_string(result)); \
		fprintf(stderr, "%s\t0 FAIL  :  %s  returned (%d) %s\n", __FILE__, function, result, err_string(result));  \
	} while (0)
*/
#define dbg_printf(lv,...) \
	DebugPrintFunc(lv, __FILE__, __LINE__, ##__VA_ARGS__)
int init_log(uint32_t, const char *, const char *, uint8_t, uint8_t);
int clean_log();
void DebugPrintFunc(int, const char *file, int line, const char *, ...);
//#define dbg_printf(lv,fmt,...) if (lv== DCRIT) syslog(LOG_CRIT, "%s:%d " fmt, __FILE__, __LINE__, ##__VA_ARGS__); else if(lv<= dbg_lv) printf("%s:%d " fmt, __FILE__, __LINE__, ##__VA_ARGS__)

//#define print_error(message,tResult) printf("(Line %d, %s) %s returned 0x%08x. %s.\n", __LINE__, __func__, message, tResult, Trspi_Error_String(tResult));

//#define err_string(tResult) Trspi_Error_String(tResult)

#endif 

