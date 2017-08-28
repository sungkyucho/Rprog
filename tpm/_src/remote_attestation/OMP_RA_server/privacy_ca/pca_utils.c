#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include "debug_log.h"


/* utility: load given file into buffer
 * return pointer of buffer allocated by malloc
 */
char *file2buf_bin(char *fname, int *pSize)
{
	int rcnt = 0, rfd = -1, rsize;
	char *fbuf = NULL;
	struct stat stbuf;

	/* get the size of given file with stat */
	if (stat(fname, &stbuf)) {
		dlog(DLOG_ERROR, "fail to stat(%s): %m\n", fname);
		goto err_out;
	}

	rsize = stbuf.st_size;
	dlog(DLOG_DEBUG, "Content Length: <%d>\n", rsize);

	if (rsize > 0) {
		fbuf = (char *)malloc(rsize);
	}

	if (fbuf == NULL) {
		dlog(DLOG_ERROR, "Invalid size OR fail to malloc(%d): %m\n", rsize);
		goto err_out;
	}

	/* load content of given file into buffer */
	rfd = open(fname, O_RDONLY);
	if (rfd < 0) {
		dlog(DLOG_ERROR, "open(%s) failed: %m\n", fname);
		free(fbuf);
		fbuf = NULL;
	} else {
		rcnt = read(rfd, fbuf, rsize);
		if (rcnt == rsize) {
			dlog(DLOG_DEBUG, "loading ok: <%d/%d>\n", rcnt, rsize);
			if (pSize) *pSize = rsize;
		} else {
			dlog(DLOG_ERROR, "insufficient read: <%d/%d>\n", rcnt, rsize);
			free(fbuf);
			fbuf = NULL;
		}

		close(rfd);
	}

err_out:
	return fbuf;
}


/* read [csize] bytes from [rfp] */
char *get_content(int csize, FILE *rfp)
{
	char *cbuf = NULL;
	int ridx = 0;

	if (csize > 0) {
		cbuf = malloc(csize);
		memset(cbuf, 0, csize);

		do {
			ridx += fread(cbuf + ridx, 1, csize - ridx, rfp);
			dlog(DLOG_DEBUG, "[%d/%d] bytes in total\n", ridx, csize);
		} while (ridx < csize);
	}

	return cbuf;
}


