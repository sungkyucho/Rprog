#ifndef	_PCA_UTILS_H_
#define	_PCA_UTILS_H_

#include <stdio.h>

/* utility: load given file into buffer
 * return pointer of buffer allocated by malloc
 */
char *file2buf_bin(char *fname, int *pSize);
/* read [csize] bytes from [rfp] */
char *get_content(int csize, FILE *rfp);

#endif /* _PCA_UTILS_H_ */
