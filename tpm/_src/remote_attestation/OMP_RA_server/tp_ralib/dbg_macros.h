#ifndef _DBG_MACROS_H_
#define _DBG_MACROS_H_

/* sync with [attester_verifier/include/dbg_macros.h]
 * NOTE - init_log is already called by main
 */

#define DDBG   0x01
#define DINFO  0x02
#define DERROR 0x04
#define DCRIT  0x08

/* should be in other object
 * - currently in [attester_verifier/ra_com.o]
 */
extern int dbg_lv;

#define dbg_printf(lv,...) \
	DebugPrintFunc(lv, __FILE__, __LINE__, ##__VA_ARGS__)

/* should be in other object
 * - currently in [attester_verifier/dbg_log.c]
 */
void DebugPrintFunc(int, const char *file, int line, const char *, ...);

#endif 
