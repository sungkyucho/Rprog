#ifndef	_TEST_UTIL_H_
#define	_TEST_UTIL_H_

#include <stdio.h>

/* for testing (naremo) */
void dump_file(FILE *rfp, char *fname);

/* for testing (naremo) */
void dump_bytes(unsigned char *buf, int blen, char *fname);

#endif /* _TEST_UTIL_H_ */
