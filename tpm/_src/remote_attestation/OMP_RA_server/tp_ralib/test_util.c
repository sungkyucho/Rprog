#include <stdio.h>
#include <stdlib.h>

/* for testing (naremo) */
void dump_file(FILE *rfp, char *fname)
{
	FILE *ofp = NULL;
	char *buf = NULL;
	int blen = 0;
	long cur_pos = 0;

	if (rfp == NULL) {
		fprintf(stderr, "<%s> NULL file pointer\n", __func__);
		return;
	}

	if (fname != NULL) {
		ofp = fopen(fname, "w");
		if (ofp == NULL) {
			fprintf(stderr, "<%s> fopen(%s) failed: %m\n", __func__, fname);
		}
	}

	if (ofp == NULL) {
		fprintf(stderr, "<%s> dumping on stdout\n", __func__);
		fprintf(stdout, "----------------------------\n");
		ofp = stdout;
	}

	cur_pos = ftell(rfp); /* mark */
	fseek(rfp, 0, SEEK_END);
	blen = 	ftell(rfp);
	fseek(rfp, 0, SEEK_SET);
	buf = malloc(blen);
	if (buf) {
		if ((int)fread(buf, 1, blen, rfp) != blen) {
			fprintf(stderr, "Unable to read in file: %m\n");
		} else {
			fwrite(buf, 1, blen, ofp);
		}
		free(buf);
	}

	fseek(rfp, cur_pos, SEEK_SET); /* restore */

	if (ofp == stdout) {
		fprintf(stdout, "\n----------------------------\n");
	} else {
		fclose(ofp);
	}
}


/* for testing (naremo) */
void dump_bytes(unsigned char *buf, int blen, char *fname)
{
	FILE *ofp = NULL;
	int wcnt = 0;

	if (buf == NULL) {
		fprintf(stderr, "<%s> NULL buffer\n", __func__);
		return;
	}

	if (fname != NULL) {
		ofp = fopen(fname, "w");
		if (ofp == NULL) {
			fprintf(stderr, "<%s> fopen(%s) failed: %m\n", __func__, fname);
		}
	}

	if (ofp == NULL) {
		fprintf(stderr, "<%s> dumping on stdout\n", __func__);
		fprintf(stdout, "----------------------------\n");
		ofp = stdout;
	}

	wcnt = fwrite(buf, 1, blen, ofp);
	if (wcnt != blen) {
		fprintf(stderr, "<%s> fwrite failed<%d/%d>: %m\n", __func__, wcnt, blen);
	}

	if (ofp == stdout) {
		fprintf(stdout, "\n----------------------------\n");
	} else {
		fclose(ofp);
	}
}


