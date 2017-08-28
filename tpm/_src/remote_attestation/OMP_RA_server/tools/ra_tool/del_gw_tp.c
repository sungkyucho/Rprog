#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <curl/curl.h>
#include <stdint.h>

#include "apr_base64.h"

typedef uint8_t BYTE;

extern void dump_file(FILE *rfp, char *fname); // in test_util.c

static char *etsi_addr = "61.250.21.211:8090"; // only for removing GW
static char *svc_id = "rapoc"; // from config(?)

int del_gw(char *gw_id, char *auth_id, char *auth_key)
{
	int ret = -1; /* default: error */
	int bidx = 0;
	CURL *hCurl = NULL;
	struct curl_slist *slist = NULL;
	CURLcode rcode = 0;
	char tbuf[256]; /* CHECK */
	char *auth_id_enc = NULL, *auth_key_enc = NULL;
	char *auth_id_hd = NULL, *auth_key_hd = NULL;

	if ((NULL == gw_id) || (NULL == auth_id) || (NULL == auth_key)) {
		return -1;
	}

	auth_id_enc = base64encode(auth_id, strlen(auth_id));
	if (NULL == auth_id_enc) {
		fprintf(stderr, "<%s> fail to base64encode(auth_id)\n", __func__);
		goto err_out;
	}

	auth_key_enc = base64encode(auth_key, strlen(auth_key));
	if (NULL == auth_key_enc) {
		fprintf(stderr, "<%s> fail to base64encode(auth_key)\n", __func__);
		goto err_out;
	}

	/* CHECK - easy vs. multi --> what's a difference? */
	hCurl = curl_easy_init();
	if (NULL == hCurl) {
		fprintf(stderr, "<%s> curl_easy_init failed\n", __func__);
		/* CHECK - how to get error info. */
		goto err_out;
	}

	/*
	 * AuthID header
	 * "AuthID: MDEwMjI3NTk4OTA="
	 * --> 8 (fixed) + 16 (almost fixed)
	 */
	bidx = snprintf(tbuf, sizeof(tbuf) - 1, "AuthID: %s", auth_id_enc);
	tbuf[bidx] = '\0';

	auth_id_hd = strdup(tbuf);

	/*
	 * AuthKey header
	 * "AuthKey: U1AxMDA1MzA2OGtleQ=="
	 * --> 9 (fixed) + 20 (almost fixed)
	 */
	bidx = snprintf(tbuf, sizeof(tbuf) - 1, "AuthKey: %s", auth_key_enc);
	tbuf[bidx] = '\0';

	auth_key_hd = strdup(tbuf);

	/* setup curl_slist */
	/* Content-Type: application/xml */
	slist = curl_slist_append(slist, "Pragma: no-cache");
	slist = curl_slist_append(slist, "Content-Type: application/xml");
	slist = curl_slist_append(slist, auth_id_hd);
	slist = curl_slist_append(slist, auth_key_hd);

	// http://61.250.21.211:8090/rapoc/scls/SC10028994
	bidx = snprintf(tbuf, sizeof(tbuf) - 1, "http://%s/%s/scls/%s", \
			etsi_addr, svc_id, gw_id);
	tbuf[bidx] = '\0';

	/* setup options */
	curl_easy_setopt(hCurl, CURLOPT_URL, tbuf);
	curl_easy_setopt(hCurl, CURLOPT_CUSTOMREQUEST, "DELETE");
	curl_easy_setopt(hCurl, CURLOPT_HTTPHEADER, slist);

	rcode = curl_easy_perform(hCurl);
	if (0 != rcode) {
		fprintf(stderr, "<%s> error in curl_easy_perform<0x%02x>: %s\n", __func__, rcode, curl_easy_strerror(rcode));
		goto err_out;
	}

	curl_slist_free_all(slist);

	long http_code = 0;
	// check return code -- 204(OK) 208(duplicated) others(error)
	rcode = curl_easy_getinfo(hCurl, CURLINFO_RESPONSE_CODE, &http_code);
	if (rcode != CURLE_ABORTED_BY_CALLBACK) {
		if (http_code == 204) {
			fprintf(stderr, "OK DELETE\n");
			ret = 0; /* OK */
		} else {
			fprintf(stderr, "NOK DELETE\n");
		}
		fprintf(stdout, "%ld\n", http_code);
	} else {
		fprintf(stderr, "<%s> CURLE_ABORTED_BY_CALLBACK\n", __func__);
	}

err_out:
	if (auth_id_enc) free(auth_id_enc);
	if (auth_key_enc) free(auth_key_enc);
	if (auth_id_hd) free(auth_id_hd);
	if (auth_key_hd) free(auth_key_hd);
	if (hCurl) curl_easy_cleanup(hCurl);

	return ret;
}


/* return pointer of allocated buffer */
char *load_svc_id(char *cfname)
{
	char *svc_id = NULL;
	char *th = NULL, *tv = NULL, *rest = NULL;
	FILE *rfp = NULL;
	char cbuf[1024];

	if (NULL == cfname) {
		fprintf(stderr, "invalid config file\n");
		return NULL;
	}

	rfp = fopen(cfname, "r");
	if (NULL == rfp) {
		fprintf(stderr, "open(%s) failed: %m\n", cfname);
		return NULL;
	}

	while (fgets(cbuf, sizeof(cbuf), rfp) != NULL) {
		if ('#' == cbuf[0]) continue;

		th = strtok_r(cbuf, ":", &rest);
		tv = strtok_r(NULL, "\n", &rest);

		if ((NULL == th) || (NULL == tv)) {
			fprintf(stderr, "invalid config\n");
			goto err_out;
		}

		if (0 == strcmp(th, "svc_id")) {
			svc_id = strdup(tv);
		} else if (0 == strcmp(th, "etsi_addr")) {
			etsi_addr = strdup(tv);
		} else {
			/* ignore */
		}
	}

err_out:
	if (rfp) fclose(rfp);

	return svc_id;
}


void usage_exit(char *progname)
{
	printf("%s [-c config] gw_id auth_id auth_key\n", progname);

	exit(-1);
}


#define	DEF_CONF	"tpconf.conf"


int main(int argc, char *argv[])
{
	char *progname = argv[0];
	int ret = -1;
	int c;
	char *cfname = NULL;

	if (argc < 4) {
		usage_exit(progname);
	}

	while (1) {
		c = getopt(argc, argv, "c:");
		if (c == -1) break;

		switch (c) {
		case 'c': // config file
			if (cfname) {
				fprintf(stderr, "duplicated config: %s/%s\n", cfname, optarg ? optarg : "null");
				usage_exit(progname);
			}
			cfname = strdup(optarg);
			break;
		default:
			usage_exit(progname);
		}
	}

	if (NULL == cfname) cfname = DEF_CONF;

	svc_id = load_svc_id(cfname);
	if (NULL == svc_id) {
		fprintf(stderr, "fail to get svc_id: %m\n");
		usage_exit(progname);
	}

	ret = del_gw(argv[optind], argv[optind+1], argv[optind+2]);
	if (ret < 0) {
		fprintf(stderr, "error in del_gw\n");
	}

	return ret;
}
