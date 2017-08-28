#include <stdio.h>
#include <stdlib.h>

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>

#include "CryptoSuite.h"
void setTPMOff(void);
void setTPMOn(void);

#define LINESIZE 8

static int plaintext_line(uint8_t *line, int linelen, uint8_t *buf)
{
    int i;
    int len = 0;
    for (i=0; i<linelen; ++i) {
	len += sprintf(&buf[len], "%02x ", line[i]);
    }

    len += sprintf(&buf[len], " ");

    for (i=0; i<linelen; ++i) {
	if (isprint(line[i])) {
	    len += sprintf(&buf[len], "%c", line[i]);
	} else {
	    len += sprintf(&buf[len], ".");
	}
    }
    return len;
}
static int ciphertext_line(uint8_t *line, int linelen, uint8_t *buf)
{
    return plaintext_line(line, linelen, buf);
}

static void print_line(uint8_t *buf, int buflen, int row_size, void * ctx)
{
    uint8_t *cipher;
    uint8_t *linebuf;
    int len;
    size_t cipherlen;
    int mid = (int)((row_size + 1)/2);
    linebuf = (char *)malloc(row_size+1);
    memset(linebuf, ' ', row_size);
    linebuf[row_size] = '\0';

    len = plaintext_line(buf, buflen, linebuf);
    linebuf[len] = ' ';

    linebuf[mid] = '|';

    cipher = malloc(buflen);
    cipherlen = buflen;
    encrypt_update(ctx, buf, buflen, cipher, &cipherlen);
    len = ciphertext_line(cipher, buflen, &linebuf[mid+1]);
    linebuf[mid+1+len] = ' ';

    fprintf(stdout, "%s\n", linebuf);
    free(linebuf);
    free(cipher);
}

static int scroll_2_panes(char *filename, char *secret)
{
    char buf[LINESIZE];
    size_t len;
    FILE *fp;
    struct winsize w;
    void *ctx;
    uint8_t iv[16] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
    char tgtbuf[16];
    size_t blen = 16;

    fp = fopen(filename, "r");
    if (!fp) {
	perror(filename);
	return -1;
    }
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    ctx = encrypt_init(secret, iv);
    while(1) {
	memset(buf, 0, LINESIZE);
	len = fread(buf, 1, LINESIZE, fp);
	print_line(buf, LINESIZE, w.ws_col, ctx);
	if (len < LINESIZE) {
	    encrypt_final(ctx, tgtbuf, &blen);
	    ctx = encrypt_init(secret, iv);
	    rewind(fp);
	}
	usleep(400000);
    }
    fclose(fp);
    encrypt_final(ctx, tgtbuf, &blen);
}

static int check_file(char *filename)
{
    struct stat statbuf;
    int r = stat(filename, &statbuf);
    if (r < 0) {
	perror(filename);
	return -1;
    }
    return 0;
}

static int generate_key_and_save(char *prefix)
{
    int len = strlen(prefix);
    char keyfile[128]; 
    char secret[16];
    int r;

    if (len > 64) {
	perror("filename too long");
	return -1;
    }
    gen_secret_key(secret, 16);
    sprintf(keyfile, "%s.raw", prefix);
    setTPMOff();
    r = save_secret(keyfile, secret, 16);
    if (r < 0) {
	fprintf(stderr, "Error: save_secret\n");
	return r;
    }
    setTPMOn();
    sprintf(keyfile, "%s.tpm", prefix);
    r = save_secret(keyfile, secret, 16);
    if (r < 0) {
	fprintf(stderr, "Error: save_secret\n");
	return r;
    }
    return 0;
}

static const char *fifo = "/tmp/fifo.scroller";
static int waitfor2(void)
{
    int fd;
    char buf;
    int r;
    struct stat statbuf;
    if (mkfifo(fifo, 0666) < 0) {
	r = stat(fifo, &statbuf);
	if (!(r == 0 && S_ISFIFO(statbuf.st_mode))) {
	    perror("mkfifo");
	    return -1;
	}
    }
    if ((fd = open(fifo, O_RDWR)) < 0) {
	perror("open");
	return -1;
    }
    r = read(fd, &buf, 1);
    if (r < 0) {
	perror("read");
	return -1;
    }
    close(fd);
    return 0;
}

static int notify1(void)
{
    int fd;
    char buf = 0;
    if ((fd = open(fifo, O_WRONLY)) < 0) {
	perror("open");
	return -1;
    }
    if (write(fd, &buf, 1) < 0) {
	perror("write");
    }
    close(fd);

    return 0;
}

static int scrollfile(char *filename, char *keyfile, int order)
{
    char secret[16];
    int r;

    if (check_file(filename) < 0) {
	return -1;
    }

    if (!keyfile) {
	gen_secret_key(secret, 16);
    } else {
	r = load_secret(keyfile, secret, 16);
	if (r < 0) {
	    fprintf(stderr, "Error: load_secret\n");
	    return r;
	}
    }
    if (order == 1) {
	if (waitfor2() < 0) {
	    return -1;
	}
    } else if (order == 2) {
	if (notify1() < 0) {
	    return -1;
	}
    }
    scroll_2_panes(filename, secret);

    return 0;
}

static void usage(char *cmdline)
{
    printf(
"Usage:\n"
"%s filename\n"
"\tjust generate random key and run; don't save it\n"
"\tfilename: plaintext file to scroll\n"
"%s -k keyfile filename\n"
"\t-keyfile: file with secret key. without it, random number is used.\n"
"\tfilename: plaintext file to scroll\n"
"%s -[12] -k keyfile filename\n"
"\t-1: run first and wait other scroller running\n"
"\t-2: run after other scroller and notify to first scroller\n"
"\t-keyfile: file with secret key. without it, random number is used.\n"
"\tfilename: plaintext file to scroll\n"
"%s -genkey keyfile\n"
"\tgenerate key only. key files are keyfile.raw and keyfile.tpm\n"
, cmdline, cmdline, cmdline, cmdline);
}

int main(int argc, char *argv[])
{
    char *filename;
    char *keyfile;
    int order = 0;
    if (argc < 2) {
	usage(argv[0]);
	return 0;
    }

    if (argc >= 2) {
	if (argc == 3 && !strncmp(argv[1], "-genkey", strlen("-genkey"))) {
	    keyfile = argv[2];
	    generate_key_and_save(keyfile);
	} else if (argc == 2) {
	    filename = argv[1];
	    scrollfile(filename, NULL, 0);
	} else if (argc == 4 && !strncmp(argv[1], "-k", 2)) {
	    keyfile = argv[2];
	    filename = argv[3];
	    scrollfile(filename, keyfile, 0);
	} else if (argc == 5 && !strncmp(argv[2], "-k", 2)) {
	    if (!strncmp(argv[1], "-1", 2)) {
		order = 1;
	    } else if (!strncmp(argv[1], "-2", 2)) {
		order = 2;
	    }
	    keyfile = argv[3];
	    filename = argv[4];
	    scrollfile(filename, keyfile, order);
	} else {
	    fprintf(stderr, "Wrong Command\n");
	}
    }
    return 0;
}
/* vi: set ai sw=4 ts=8 sts=4: */
