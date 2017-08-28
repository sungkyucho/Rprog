#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
//#include <stdlib.h> // for exit
#include <stdarg.h>
#include <curl/curl.h>

#include "tp_ralib.h"
#include "tp_ralib_conf.h"

struct tp_config *cfg_ra = NULL; // global


// for usage in [tp_ralib]
#define	DDBG	0x01
#define	DINFO	0x02
#define	DERROR	0x04
#define	DCRIT	0x08

static int dbg_lv_here = DERROR;

// print out to standard error
// NOTE - only for linking with tp_ralib
void DebugPrintFunc(int log_lv, const char *file, int line, const char *errmsg, ...)
{
	va_list ap;

	va_start(ap, errmsg);

	if (dbg_lv_here <= log_lv) {
		fprintf(stderr, "[%s:%d] ", file, line);
		vfprintf(stderr, errmsg, ap);
	}

	va_end(ap);
}


extern void dump_file(FILE *rfp, char *fname); // in test_util.c


int add_gw(char *mac_addr)
{
	int ret = -1; /* default: error */
	int bidx = 0;
	FILE *f_tmp = NULL;
	CURL *hCurl = NULL;
	struct curl_slist *slist = NULL;
	char *auth_code = NULL;
	CURLcode rcode = 0;
	char *pbuf = NULL;
	char tbuf[256]; /* CHECK */

	fprintf(stderr, "<%s> start <mac_addr:%p>\n", __func__, mac_addr);

	if (NULL == mac_addr) {
		return -1;
	}

	/* CHECK - AuthId/AuthKey from OMP or config file */
	auth_code = get_auth_omp(cfg_ra->auth_id, cfg_ra->auth_key);
	if (NULL == auth_code) {
		fprintf(stderr, "<%s> get_auth_omp failed\n", __func__);
		goto err_out;
	}

	f_tmp = tmpfile();
	if (NULL == f_tmp) {
		fprintf(stderr, "<%s> tmpfile failed: %m\n", __func__);
		goto err_out;
	}

	/* CHECK - easy vs. multi --> what's a difference? */
	hCurl = curl_easy_init();
	if (NULL == hCurl) {
		fprintf(stderr, "<%s> curl_easy_init failed\n", __func__);
		/* CHECK - how to get error info. */
		goto err_out;
	}

	/* auth header
	 * "Authorization: QVAxMDAwOTUxNTpBUDEwMDA5NTE1"
	 * --> 15 (fixed) + 28 (almost fixed)
	 */
	bidx = snprintf(tbuf, sizeof(tbuf) - 1, "Authorization: %s", auth_code);
	tbuf[bidx] = '\0';

	/* setup curl_slist */
	/* Content-Type: application/xml */
	/* Authorization: QVAxMDAwOTUxNTpBUDEwMDA5NTE1 */
	slist = curl_slist_append(slist, "Pragma: no-cache");
	slist = curl_slist_append(slist, "Content-Type: application/xml");
	slist = curl_slist_append(slist, tbuf);

	// http://61.250.21.211:10005/rapoc/scls/min
	bidx = snprintf(tbuf, sizeof(tbuf) - 1, "http://%s/%s/scls/min", \
			cfg_ra->mIa_addr, cfg_ra->svc_id);
	tbuf[bidx] = '\0';

	// make body
	pbuf = malloc(2048); /* sufficient */
	if (NULL == pbuf) {
		fprintf(stderr, "<%s> malloc failed: %m\n", __func__);
		goto err_out;
	}

	bidx = sprintf(pbuf, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	bidx += sprintf(pbuf + bidx, "<scl>\n");
	bidx += sprintf(pbuf + bidx, "<gwID>%s</gwID>\n<MfID>MF%s</MfID>\n", mac_addr, mac_addr);
	bidx += sprintf(pbuf + bidx, "<Priority>1</Priority>\n<reportPeriod>10</reportPeriod>\n");
	bidx += sprintf(pbuf + bidx, "<locTargetDevice>%s</locTargetDevice>\n", mac_addr);
	bidx += sprintf(pbuf + bidx, "<sclType>A</sclType>\n"); // GMMP + AlwaysOn
	bidx += sprintf(pbuf + bidx, "</scl>\n");

	fprintf(stderr, "<%s> postfield length<%d>\n", __func__, bidx);

	/* setup options */
	curl_easy_setopt(hCurl, CURLOPT_URL, tbuf);
	curl_easy_setopt(hCurl, CURLOPT_POSTFIELDS, pbuf);
	curl_easy_setopt(hCurl, CURLOPT_POSTFIELDSIZE, bidx);
	//curl_easy_setopt(hCurl, CURLOPT_WRITEDATA, (BYTE **)f_tmp);
	curl_easy_setopt(hCurl, CURLOPT_CUSTOMREQUEST, "POST");
	curl_easy_setopt(hCurl, CURLOPT_HTTPHEADER, slist);

	rcode = curl_easy_perform(hCurl);
	if (0 != rcode) {
		fprintf(stderr, "<%s> error in curl_easy_perform<0x%02x>: %s\n", __func__, rcode, curl_easy_strerror(rcode));
		goto err_out;
	}

	curl_slist_free_all(slist);

	//fflush(f_tmp);
	/* debug - dump response */
	//dump_file(f_tmp, "put.rsp");

	long http_code = 0;
	// check return code -- 201(OK) 207(duplicated) others(error)
	rcode = curl_easy_getinfo(hCurl, CURLINFO_RESPONSE_CODE, &http_code);
	if (rcode != CURLE_ABORTED_BY_CALLBACK) {
		if (http_code == 201) {
			fprintf(stderr, "OK ADD\n");
			ret = 0; /* OK */
		} else {
			fprintf(stderr, "NOK ADD\n");
		}
		fprintf(stdout, "%ld\n", http_code);
	} else {
		fprintf(stderr, "<%s> CURLE_ABORTED_BY_CALLBACK\n", __func__);
	}

err_out:
	if (auth_code) free(auth_code);
	//if (f_tmp) fclose(f_tmp);
	if (hCurl) curl_easy_cleanup(hCurl);

	return ret;
}


int search_gw(char *gw_id)
{
	int ret = -1; /* default: error */
	int bidx = 0;
	FILE *f_tmp = NULL;
	CURL *hCurl = NULL;
	struct curl_slist *slist = NULL;
	char *auth_code = NULL;
	CURLcode rcode = 0;
	char tbuf[256]; /* CHECK */

	fprintf(stderr, "<%s> start <gw_id:%p>\n", __func__, gw_id);

	if (NULL == gw_id) {
		return -1;
	}

	/* CHECK - AuthId/AuthKey from OMP or config file */
	auth_code = get_auth_omp(cfg_ra->auth_id, cfg_ra->auth_key);
	if (NULL == auth_code) {
		fprintf(stderr, "<%s> get_auth_omp failed\n", __func__);
		goto err_out;
	}

	f_tmp = tmpfile();
	if (NULL == f_tmp) {
		fprintf(stderr, "<%s> tmpfile failed: %m\n", __func__);
		goto err_out;
	}

	/* CHECK - easy vs. multi --> what's a difference? */
	hCurl = curl_easy_init();
	if (NULL == hCurl) {
		fprintf(stderr, "<%s> curl_easy_init failed\n", __func__);
		/* CHECK - how to get error info. */
		goto err_out;
	}

	/* auth header
	 * "Authorization: QVAxMDAwOTUxNTpBUDEwMDA5NTE1"
	 * --> 15 (fixed) + 28 (almost fixed)
	 */
	bidx = snprintf(tbuf, sizeof(tbuf) - 1, "Authorization: %s", auth_code);
	tbuf[bidx] = '\0';

	/* setup curl_slist */
	/* Content-Type: application/xml */
	/* Authorization: QVAxMDAwOTUxNTpBUDEwMDA5NTE1 */
	slist = curl_slist_append(slist, "Pragma: no-cache");
	slist = curl_slist_append(slist, "Content-Type: application/xml");
	slist = curl_slist_append(slist, tbuf);

	// http://61.250.21.211:10005/rapoc/scls/SC10024618
	bidx = snprintf(tbuf, sizeof(tbuf) - 1, "http://%s/%s/scls/%s", \
			cfg_ra->mIa_addr, cfg_ra->svc_id, gw_id);
	tbuf[bidx] = '\0';

	/* setup options */
	curl_easy_setopt(hCurl, CURLOPT_URL, tbuf);
	curl_easy_setopt(hCurl, CURLOPT_WRITEDATA, (BYTE **)f_tmp);
	//curl_easy_setopt(hCurl, CURLOPT_CUSTOMREQUEST, "GET");
	curl_easy_setopt(hCurl, CURLOPT_HTTPHEADER, slist);

	rcode = curl_easy_perform(hCurl);
	if (0 != rcode) {
		fprintf(stderr, "<%s> error in curl_easy_perform<0x%02x>: %s\n", __func__, rcode, curl_easy_strerror(rcode));
		goto err_out;
	}

	curl_slist_free_all(slist);

	fflush(f_tmp);
	/* debug - dump response */
	//dump_file(f_tmp, "search.rsp");
	dump_file(f_tmp, NULL);

	long http_code = 0;
	// check return code -- 200(OK) others(error)
	rcode = curl_easy_getinfo(hCurl, CURLINFO_RESPONSE_CODE, &http_code);
	if (rcode != CURLE_ABORTED_BY_CALLBACK) {
		if (http_code == 200) {
			fprintf(stderr, "OK SEARCH\n");
			ret = 0; /* OK */
		} else {
			fprintf(stderr, "NOK SEARCH\n");
		}
		fprintf(stdout, "%ld\n", http_code);
	} else {
		fprintf(stderr, "<%s> CURLE_ABORTED_BY_CALLBACK\n", __func__);
	}

	fflush(stdout);

err_out:
	if (auth_code) free(auth_code);
	if (f_tmp) fclose(f_tmp);
	if (hCurl) curl_easy_cleanup(hCurl);

	return ret;
}


void usage_exit(char *progname)
{
	printf("%s -[a|c|s] args\n", progname);
	printf("   a mac_addr: add new device into OMP\n");
	printf("   s gw_id: search GW with gw_id\n");
	printf("   c cfg_file: config file for tp_ralib\n");

	exit(-1);
}


#define	ADD_U_CMD	0x0001
#define	SEARCH_CMD	0x0002

#define	DEF_CONF	"tpconf.conf"


int main(int argc, char *argv[])
{
	int cmd_flag = 0x00;
	char *progname = argv[0];
	int c, ret = -1;
	char *keydata = NULL, *cfname = NULL;
	int (*handler)(char *key) = NULL;

	while (1) {
		c = getopt(argc, argv, "a:c:s:r:");
		if (c == -1) break;

		if (cmd_flag && ('c' != (char)c)) {
			fprintf(stderr, "duplicated command: %c/%04x\n", (char)c, cmd_flag);
			usage_exit(progname);
		}

		switch (c) {
		case 'a': // add unregistered GW
			cmd_flag |= ADD_U_CMD;
			keydata = strdup(optarg); // mac_addr
			handler = add_gw;
			break;
		case 'c': // config file
			if (cfname) {
				fprintf(stderr, "duplicated config: %s/%s\n", cfname, optarg ? optarg : "null");
				usage_exit(progname);
			}
			cfname = strdup(optarg);
			break;
		case 's': // search registered GW
			cmd_flag |= SEARCH_CMD;
			keydata = strdup(optarg); // gw_id
			handler = search_gw;
			break;
		default:
			usage_exit(progname);
		}
	}

	fprintf(stderr, "CMD<%04x> keydata<%s>\n", \
			cmd_flag, keydata ? keydata : "none");

	if (cmd_flag == 0x00) {
		usage_exit(progname);
	}

	cfg_ra = (struct tp_config *)calloc(1, sizeof(struct tp_config));
	if (NULL == cfg_ra) {
		fprintf(stderr, "calloc failed: %m\n");
		return -1;
	}

	if (NULL == cfname) {
		cfname = DEF_CONF;
		fprintf(stderr, "use default config file\n");
	}
	fprintf(stderr, "config file<%s>\n", cfname);

	ret = load_ra_config(cfname, cfg_ra);
	fprintf(stderr, "load_ra_config -> <%d>\n", ret);

	ret = handler(keydata);
	fprintf(stderr, "ret<%d>\n", ret);

	if (keydata) free(keydata);
	keydata = NULL;

	free(cfg_ra);

	return 0;
}
