#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

#include "tp_ra_type.h"
#include "apr_base64.h"
#include "tp_ralib_conf.h"
#include "test_util.h"
#include "dbg_macros.h"

//#define	DBG_VERBOSE	// show verbose message (on stdout)
#ifdef	DBG_VERBOSE
#define	debug(fmt, ...)	do { printf(fmt, ## __VA_ARGS__); } while (0)
#else
#define	debug(fmt, ...)
#endif // DBG_VERBOSE


/**
 * @brief convert "YYYY-MM-DDThh:mm:ssTZD" into time_t
 *  - NOTE - timezone is not processed
 * @param html_time string "YYYY-MM-DDThh:mm:ssTZD"
 * @return 0 on error, otherwise converted time_t
 */
time_t get_local_time(char *html_time)
{
	int pcnt, year, mon, day, hour, min, sec;
	char zone[8];
	struct tm when;

	memset(zone, 0, sizeof(zone));
	memset(&when, 0, sizeof(when));

	pcnt = sscanf(html_time, "%d-%d-%dT%d:%d:%d%s", \
			&year, &mon, &day, &hour, &min, &sec, zone);

	if (6 > pcnt) {
		dbg_printf(DERROR, "<%s> invalid time <%s>\n", __func__, html_time);
		return 0; /* error */
	}

	when.tm_year = year - 1900;
	when.tm_mon = mon - 1;
	when.tm_mday = day;
	when.tm_hour = hour;
	when.tm_min = min;
	when.tm_sec = sec;

	return mktime(&when);
}


/* convert string into [tp_resp_code] */
tp_resp_code get_decision(char *resp_code)
{
	tp_resp_code ret = TP_RA_NADA; /* default */

	if (!strcmp("SUCCESS", resp_code)) {
		ret = TP_RA_SUCCESS;
	} else if (!strcmp("PROGRESS", resp_code)) {
		ret = TP_RA_PROGRESS;
	} else if (!strcmp("NACK", resp_code)) {
		ret = TP_RA_NACK;
	} else if (!strcmp("ACK", resp_code)) {
		ret = TP_RA_ACK;
	} else if (!strcmp("TIMEOUT", resp_code)) {
		ret = TP_RA_TIMEOUT;
	} else if (!strcmp("FAIL", resp_code)) {
		ret = TP_RA_FAIL;
	} else {
		dbg_printf(DERROR, "<%s> invalid response code <%s>\n", __func__, resp_code);
	}

	return ret;
}


/* update [ra_response] structure with given response message
 * - following fields are checked
 * - <lastModifiedTime> / <definedActionStatus>
 * - <response> - by GET (from notify message from client)
 * -- length of <response> could be reach about MAX_MSG_BODY * (4/3)
 * return 0 on success
 */
const char t_time_b[] = "<lastModifiedTime>";
const char t_time_e[] = "</lastModifiedTime>";
const char t_status_b[] = "<definedActionStatus>";
const char t_status_e[] = "</definedActionStatus>";
const char t_resp_b[] = "<response>";
const char t_resp_e[] = "</response>";


int update_response(ra_response *resp, FILE *rfp)
{
	int ret = -1, len;
	char *lbuf = NULL, *token = NULL, *pch = NULL;
	int f_time = 0, f_status = 0, f_resp = 0;

	if ((NULL == resp) || (NULL == rfp)) {
		dbg_printf(DERROR, "<%s> invalid response<%p> or file<%p>\n", __func__, resp, rfp);
		goto err_out;
	}

	// for base64encode(MAX_MSG_BODY string)
	lbuf = malloc(MAX_MSG_BODY * 2);
	if (NULL == lbuf) {
		dbg_printf(DCRIT, "<%s> malloc failed: %m\n", __func__);
		goto err_out;
	}

	fseek(rfp, 0, SEEK_SET);
	while (fgets(lbuf, MAX_MSG_BODY * 2, rfp)) {
		if (!f_time && (token = strstr(lbuf, t_time_b))) {
			token += strlen(t_time_b);
			pch = strstr(token, t_time_e);
			if (NULL == pch) {
				dbg_printf(DERROR, "<%s> no %s\n", __func__, t_time_e);
				continue; /* skip this line */
			}
			*pch = '\0';
			resp->mtime = get_local_time(token);
			debug("<%s> time <%s>\n", __func__, token);
		} else if (!f_status && (token = strstr(lbuf, t_status_b))) {
			token += strlen(t_status_b);
			pch = strstr(token, t_status_e);
			if (NULL == pch) {
				dbg_printf(DERROR, "<%s> no %s\n", __func__, t_status_e);
				continue; /* skip this line */
			}
			*pch = '\0';
			resp->decision = get_decision(token);
			debug("<%s> decision <%s>\n", __func__, token);
		} else if (!f_resp && (token = strstr(lbuf, t_resp_b))) {
			token += strlen(t_resp_b);
			pch = strstr(token, t_resp_e);
			if (NULL == pch) {
				dbg_printf(DERROR, "<%s> no %s\n", __func__, t_resp_e);
				continue; /* skip this line */
			}
			*pch = '\0';
			/* now [token] points base64encoded string */
			debug("<%s> msgbody <%s>\n", __func__, token);
			resp->bodylen = apr_base64_decode_len(token);
			len = apr_base64_decode((unsigned char *)(resp->msgbody), token);
			if (len != resp->bodylen) {
				dbg_printf(DERROR, "<%s> decoded length mismatched <%d/%d>\n", __func__, len, resp->bodylen);
				//continue;
			}
			debug("<%s> decoded msgbody length <%d/%d>\n", __func__, len, resp->bodylen);
#ifdef	DBG_VERBOSE
		} else {
			int blen = strlen(lbuf);
			if ('\n' == lbuf[blen - 1]) {
				lbuf[blen - 1] = '\0';
			}
			printf("<%s> bypass <%s>\n", __func__, lbuf);
#endif // DBG_VERBOSE
		}
	}

	ret = 0; /* OK */

err_out:
	if (lbuf) free(lbuf);
	return ret;
}


/* Auth info - AppId/AuthId/AuthKey
 * Default App: AP10009515/AP10009515/AP10009515
 * RA App: AP10009560/AP10009560/AK10000248
 * PCA App: AP10009559/AP10009559/AK10000247
 */
/**
 * @brief return Authorization value of ThingPlug
 * @param authId AuthId value
 * @param authKey AuthKey value
 * @return buffer allocated for Authorization, otherwise NULL
 */
char *get_auth_omp(char *authId, char *authKey)
{
	int a_len = 0;
	char authInfo[128];
	char *encoded = NULL;

	/* Authorization: + base64_encode(AuthId + : + AuthKey) */
	/* "QVAxMDAwOTUxNTpBUDEwMDA5NTE1" = base64_encode("AP10009515:AP10009515") */

	if ((NULL == authId) || (NULL == authKey)) {
		dbg_printf(DERROR, "invalid authId<%p> or authKey<%p>\n", authId, authKey);
		return NULL;
	}

	a_len = snprintf(authInfo, sizeof(authInfo) - 1, "%s:%s", authId, authKey);
	authInfo[sizeof(authInfo) - 1] = 0x0;
	dbg_printf(DDBG, "authInfo = <%s>\n", authInfo);

	encoded = base64encode(authInfo, a_len);

#ifdef	DBG_VERBOSE
	if (NULL == encoded) {
		printf("Error in base64encode(len:%d): %m\n", a_len);
	} else {
		printf("Result <%s>\n", encoded);
	}

	char *decoded = base64decode(encoded);
	printf("decoded = <%s>\n", decoded);
	free(decoded);
#endif	// DBG_VERBOSE

	return encoded;
}


/* return the pointer of allocated buffer
 * [*pf_sz] should be updated with length of buffer
 */
char *get_postfield(ra_request *req, int *pf_sz)
{
	char *pbuf = NULL, *ebuf = NULL;
	FILE *f_tmp = NULL;
	int plen = 0;
	char plain[NONCE_LEN + PCR_LIST_LEN];

	f_tmp = tmpfile();
	if (NULL == f_tmp) {
		dbg_printf(DCRIT, "<%s> tmpfile failed: %m\n", __func__);
		goto err_out;
	}

	memset(plain, 0, sizeof(plain));
	memcpy(plain, req->nonce, NONCE_LEN);
	memcpy(plain + NONCE_LEN, req->pcr_select, PCR_LIST_LEN);
	/* NOTE - base64 encoded string should be ASCII string */
	ebuf = base64encode(plain, sizeof(plain));
	if (NULL == ebuf) {
		dbg_printf(DERROR, "<%s> base64encoding failed\n", __func__);
		goto err_out;
	}

	fprintf(f_tmp, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	fprintf(f_tmp, "<mgmtObj>\n<request>");
	fprintf(f_tmp, "%s", ebuf);
	fprintf(f_tmp, "</request>\n</mgmtObj>\n");

	free(ebuf); /* no more usage */

	fflush(f_tmp);
	plen = ftell(f_tmp);
	dbg_printf(DDBG, "<%s:%d> plen<%d>\n", __func__, __LINE__, plen);
	fseek(f_tmp, 0, SEEK_SET);

	pbuf = malloc(plen + 1); /* for EOS */
	if (pbuf) {
		int rcnt = 0;
		pbuf[plen] = '\0'; /* adding EOS */
		rcnt = (int)fread(pbuf, 1, plen, f_tmp);
		if (plen != rcnt) {
			dbg_printf(DERROR, "<%s> fail to read postfield: %m\n", __func__);
			free(pbuf); pbuf = NULL;
			goto err_out;
		}
	}

	*pf_sz = plen;

err_out:
	if (f_tmp) fclose(f_tmp);

	return pbuf;
}


/* config (should be initialized/freed) */
static struct tp_config *cfg_ra = NULL;


/* Send Control: send one PUT request to OMP
 * return 0 on success
 */
int send_put_to_omp(ra_request *req, ra_response *resp)
{
	int ret = -1; /* default: error */
	int bidx = 0, pf_len = 0;
	FILE *f_tmp = NULL;
	CURL *hCurl = NULL;
	struct curl_slist *slist = NULL;
	char *auth_code = NULL, *postfield = NULL;
	CURLcode rcode = 0;
	char tbuf[256]; /* CHECK */

	if ((NULL == req) || (NULL == resp)) {
		dbg_printf(DERROR, "<%s> invalid request<%p> or response<%p>\n", __func__, req, resp);
		goto err_out;
	}

	if (NULL == cfg_ra) {
		dbg_printf(DERROR, "<%s> no configuration\n", __func__);
		goto err_out;
	}

	/* CHECK - AuthId/AuthKey from OMP or config file */
	auth_code = get_auth_omp(cfg_ra->auth_id, cfg_ra->auth_key);
	if (NULL == auth_code) {
		dbg_printf(DERROR, "get_auth_omp failed\n");
		goto err_out;
	}

	f_tmp = tmpfile();
	if (NULL == f_tmp) {
		dbg_printf(DCRIT, "tmpfile failed: %m\n");
		goto err_out;
	}

	/* CHECK - easy vs. multi --> what's a difference? */
	hCurl = curl_easy_init();
	if (NULL == hCurl) {
		dbg_printf(DERROR, "curl_easy_init failed\n");
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

	// http://61.250.21.211:10005/rapoc/scls/SC10024618/mgmtObjs/mgmtUser05
	// URL's length = 72 + alpha (by the length of service ID)
	// HTTP + OMP_src_mIa + SvcCode + scls + GwId + mgmtObjs/mgmtUser + Number
	bidx = snprintf(tbuf, sizeof(tbuf) - 1, \
			"http://%s/%s/scls/%s/mgmtObjs/mgmtUser%02d", \
			cfg_ra->mIa_addr, cfg_ra->svc_id, req->gw_id, cfg_ra->udc_no);
	tbuf[bidx] = '\0';

	postfield = get_postfield(req, &pf_len);
	if ((NULL == postfield) || (0 == pf_len)) {
		dbg_printf(DERROR, "<%s> fail to get postfield <%p/%d>\n", __func__, postfield, pf_len);
		goto err_out;
	}

	/* setup options */
	curl_easy_setopt(hCurl, CURLOPT_URL, tbuf);
	curl_easy_setopt(hCurl, CURLOPT_POSTFIELDS, postfield);
	curl_easy_setopt(hCurl, CURLOPT_POSTFIELDSIZE, pf_len);
	curl_easy_setopt(hCurl, CURLOPT_WRITEDATA, (BYTE **)f_tmp);
	curl_easy_setopt(hCurl, CURLOPT_CUSTOMREQUEST, "PUT");
	curl_easy_setopt(hCurl, CURLOPT_HTTPHEADER, slist);

	rcode = curl_easy_perform(hCurl);
	if (0 != rcode) {
		dbg_printf(DERROR, "error in curl_easy_perform<0x%02x>: %s\n", rcode, curl_easy_strerror(rcode));
		goto err_out;
	}

	curl_slist_free_all(slist);

	fflush(f_tmp);
	/* debug - dump response */
	//dump_file(f_tmp, "put.rsp");

	/* processing response */
	if (update_response(resp, f_tmp)) {
		dbg_printf(DERROR, "<%s> error in update_response\n", __func__);
		goto err_out;
	}

	ret = 0; /* OK */

err_out:
	if (postfield) free(postfield);
	if (auth_code) free(auth_code);
	if (f_tmp) fclose(f_tmp);
	if (hCurl) curl_easy_cleanup(hCurl);

	return ret;
}


/* Get Notify: send one GET request to OMP
 * return 0 on success
 */
int send_get_to_omp(ra_response *resp)
{
	int ret = -1; /* default: error */
	int bidx = 0;
	FILE *f_tmp = NULL;
	CURL *hCurl = NULL;
	struct curl_slist *slist = NULL;
	char *auth_code = NULL;
	CURLcode rcode = 0;
	char tbuf[256]; /* CHECK */

	if (NULL == resp) {
		dbg_printf(DERROR, "<%s> invalid response<%p>\n", __func__, resp);
		goto err_out;
	}

	if (NULL == cfg_ra) {
		dbg_printf(DERROR, "<%s> no configuration\n", __func__);
		goto err_out;
	}

	/* CHECK - AuthId/AuthKey from OMP or config file */
	auth_code = get_auth_omp(cfg_ra->auth_id, cfg_ra->auth_key);
	if (NULL == auth_code) {
		dbg_printf(DERROR, "get_auth_omp failed\n");
		goto err_out;
	}

	f_tmp = tmpfile();
	if (NULL == f_tmp) {
		dbg_printf(DCRIT, "tmpfile failed: %m\n");
		goto err_out;
	}

	/* CHECK - easy vs. multi --> what's a difference? */
	hCurl = curl_easy_init();
	if (NULL == hCurl) {
		dbg_printf(DERROR, "curl_easy_init failed\n");
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

	// http://61.250.21.211:10005/rapoc/scls/SC10024618/mgmtObjs/mgmtUser05
	// URL's length = 72 + alpha (by the length of service ID)
	// HTTP + OMP_src_mIa + SvcCode + scls + GwId + mgmtObjs/mgmtUser + Number
	bidx = snprintf(tbuf, sizeof(tbuf) - 1, \
			"http://%s/%s/scls/%s/mgmtObjs/mgmtUser%02d", \
			cfg_ra->mIa_addr, cfg_ra->svc_id, resp->gw_id, cfg_ra->udc_no);
	tbuf[bidx] = '\0';

	/* setup options */
	curl_easy_setopt(hCurl, CURLOPT_URL, tbuf);
	curl_easy_setopt(hCurl, CURLOPT_WRITEDATA, (BYTE **)f_tmp);
	curl_easy_setopt(hCurl, CURLOPT_HTTPHEADER, slist);

	rcode = curl_easy_perform(hCurl);
	if (0 != rcode) {
		dbg_printf(DERROR, "error in curl_easy_perform<0x%02x>: %s\n", rcode, curl_easy_strerror(rcode));
		goto err_out;
	}

	curl_slist_free_all(slist);

	fflush(f_tmp);
	/* debug - dump response */
	//dump_file(f_tmp, "get.rsp");

	/* processing response */
	if (update_response(resp, f_tmp)) {
		dbg_printf(DERROR, "<%s> error in update_response\n", __func__);
		goto err_out;
	}

	ret = 0; /* OK */

err_out:
	if (auth_code) free(auth_code);
	if (f_tmp) fclose(f_tmp);
	if (hCurl) curl_easy_cleanup(hCurl);

	return ret;
}


// processing queue - circular queue (linux LIST)
//#include <x86_64-linux-gnu/sys/queue.h>
#include <sys/queue.h>
// maximum number must be configured

// structure for entry
struct ra_entry {
	CIRCLEQ_ENTRY(ra_entry) qlink; /* NOTE */
	ra_response resp;
};

// Headers for List of Free, Result, and RA status
typedef CIRCLEQ_HEAD(statusq, ra_entry) ra_header;
// prevent dumping core on calling functions without init.
static ra_header rt_head = CIRCLEQ_HEAD_INITIALIZER(rt_head);
static ra_header st_head = CIRCLEQ_HEAD_INITIALIZER(st_head);
static int rt_cnt = 0, st_cnt = 0;
static struct ra_entry *st_mark = NULL;

// FIXME - should be mamaged by config
// over fr_max, free
// over rt_max, leave in st_head
// over st_max, ignore request
//static cfg_rt_max = 100;
static int cfg_st_max = 1000;

/* NOTE - RA status queue should be circular queue
 *        bcz updating status would be done with budget counter
 *        keeping position of final update is critial
 *        for preventing starvation in the long list of ra_status
 */

/* Q-related debug/test routines
 * NOTE - "2016-09-12T17:55:11" -- "%Y-%m-%dT%H:%M:%S"
 */
void dump_entries(ra_header *headp)
{
	int i = 0;
	struct ra_entry *entry = NULL;
	struct tm *ptm = NULL;
	char tbuf[64]; /* > 26 */

	if (NULL == headp) {
		printf("<%s> Invalid header<%p>\n", __func__, headp);
		return;
	}

	CIRCLEQ_FOREACH(entry, headp, qlink) {
		printf("[%04d] gw_id <%s>\n", i, entry->resp.gw_id);
		printf("[%04d] dev_id <%s>\n", i, entry->resp.dev_id);

		ptm = localtime(&(entry->resp.mtime));
		if (ptm && strftime(tbuf, sizeof(tbuf), "%Y-%m-%dT%H:%M:%S", ptm)) {
			printf("[%04d] mtime <%s>\n", i, tbuf);
		} else { /* conversion failed */
			printf("[%04d] mtime <%lx>\n", i, entry->resp.mtime);
		}

		printf("[%04d] decision <%d>\n", i, entry->resp.decision);
		printf("[%04d] bodylen <%d>\n", i, entry->resp.bodylen);

		i++; // count (for checking)
	}

	printf("<%d> entries linked to given header\n", i);
}


/**
 * @brief dump all entries in List of Free, Result, and RA status for debug
 */
void dump_all_entries(void)
{
	printf("++ Dump RA status [count: %d]\n", st_cnt);
	dump_entries(&st_head);
	printf("++ Dump Result [count: %d]\n", rt_cnt);
	dump_entries(&rt_head);
}


// NOTE - after CIRCLEQ_FOREACH, entry might point [ra_header]
//        not [struct ra_entry]
//        Don't refer any member of [struct ra_entry]
//        without checking whether entry point [ra_header] or not
// NOTE - in CIRCLEQ_FOREACH, CIRCLEQ_REMOVE or freeing might
//        cause bugs or segfault

/* search [ra_entry] whose ID's are same with given [ra_request] */
struct ra_entry *search_ra_entry(ra_request *req, struct statusq *headp)
{
	struct ra_entry *entry = NULL, *target = NULL;

	if ((NULL == headp) || (NULL == req)) {
		dbg_printf(DERROR, "<%s> Invalid header<%p> or request<%p>\n", __func__, headp, req);
		return NULL;
	}

	CIRCLEQ_FOREACH(entry, headp, qlink) {
		if (strncmp(entry->resp.gw_id, req->gw_id, LEN_GW_ID))
			continue; /* skip */

		if (strncmp(entry->resp.dev_id, req->dev_id, LEN_DEVICE_ID))
			continue; /* skip */

#ifdef	DBG_VERBOSE
		printf("<%s> gateway/device ID matched\n", __func__);
		printf("<%s> gw_id <%s/%s>\n", __func__, entry->resp.gw_id, req->gw_id);
		printf("<%s> dev_id <%s/%s>\n", __func__, entry->resp.dev_id, req->dev_id);
#endif // DBG_VERBOSE

		target = entry;
		break;
	}

	return target;
}


/* return entry of RA status for given ra_request
 * NOTE - gw_id/dev_id are copied
 */
struct ra_entry *get_ra_entry(ra_request *req)
{
	struct ra_entry *entry = NULL;

	if (NULL == req) {
		dbg_printf(DERROR, "<%s> Invalid request<%p>\n", __func__, req);
		goto err_out;
	}

	// search duplicated entry of [req] in given list (headp)
	entry = search_ra_entry(req, &st_head);

	if (entry) {
		CIRCLEQ_REMOVE(&st_head, entry, qlink);
		st_cnt--; // NOTE
	} else {
		if (st_cnt >= cfg_st_max) {
			dbg_printf(DERROR, "<%s> RA status entries are full <%d/%d>\n", __func__, st_cnt, cfg_st_max);
			goto err_out;
		}

		entry = (struct ra_entry *)malloc(sizeof(struct ra_entry));
		if (NULL == entry) {
			dbg_printf(DCRIT, "<%s> malloc(%lu) failed: %m\n", __func__, sizeof(struct ra_entry));
			goto err_out;
		}
	}

	// reset entry
	memset(entry, 0, sizeof(struct ra_entry));
	memcpy(entry->resp.gw_id, req->gw_id, LEN_GW_ID);
	memcpy(entry->resp.dev_id, req->dev_id, LEN_DEVICE_ID);
	entry->resp.decision = TP_RA_NADA; /* initial status */

err_out:
	return entry;
}


/* add given entry into List of RA status
 * return 0 on success
 */
int add_ra_entry(struct ra_entry *entry)
{
	int ret = -1; /* default: error */

	if (NULL == entry) {
		dbg_printf(DERROR, "<%s> invalid entry <%p>\n", __func__, entry);
		goto err_out;
	}

	CIRCLEQ_INSERT_TAIL(&st_head, entry, qlink);
	st_cnt++;

	ret = 0; /* OK */

err_out:
	return ret;
}


/**
 * @brief register given RA request(s) into waiting queue
 * @param cnt number of given RA request(s)
 * @param req array of RA request(s)
 * @return counter of successfully loaded into waiting queue, or -1 on error
 */
int issue_ra(int cnt, ra_request *req)
{
	int i = 0, icnt = 0;
	struct ra_entry *entry = NULL;

	dbg_printf(DINFO, "start to issue RA(s) with counter<%d>\n", cnt);

	if (1 > cnt) {
		dbg_printf(DERROR, "<%s:%d> invalid counter: %d\n", __func__, __LINE__, cnt);
		return -1;
	}

	/* send control and save into response with initial response */
	for (i = 0, icnt = 0; i < cnt; i++) {
		// if # of response reach limit, stop and return # of processed
		entry = get_ra_entry(req + i);
		if (NULL == entry) {
			dbg_printf(DERROR, "<%s> error in get_ra_entry\n", __func__);
			goto err_out;
		}

		// send request(PUT) to omp
		// save initial result into response
		if (send_put_to_omp(req + i, &(entry->resp))) {
			dbg_printf(DERROR, "<%s> error in send_put_to_omp [%d]\n", __func__, i);
			goto err_out;
		}

		if (0 == add_ra_entry(entry)) {
			icnt++; // increment # of processed
		} else {
			dbg_printf(DERROR, "<%s> failed to add request to list\n", __func__);
			goto err_out;
		}
	}

err_out:
	if (cnt != i) { // something is happened in for-loop
		if (entry) {
			free(entry);
		}
	}

	dbg_printf(DINFO, "%d RA%s issued\n", cnt, (cnt > 1) ? "'s are" : " is");

	return icnt;
}


/* return 1 on finished, 0 on unfinished, -1 on invalid */
int is_finished(tp_resp_code decision)
{
	int ret = -1; /* default: invalid */

	switch (decision) {
	case TP_RA_SUCCESS:
	case TP_RA_NACK:
	case TP_RA_TIMEOUT:
	case TP_RA_FAIL:
		dbg_printf(DDBG, "<%s> finished <%d>\n", __func__, decision);
		ret = 1; /* finished */
		break;
	case TP_RA_NADA:
	case TP_RA_PROGRESS:
	case TP_RA_ACK:
		dbg_printf(DDBG, "<%s> unfinished <%d>\n", __func__, decision);
		ret = 0; /* unfinished */
		break;
	default:
		dbg_printf(DINFO, "<%s> invalid <%d>\n", __func__, decision);
	}

	return ret;
}


/**
 * @brief request completed response(s)
 *  - NOTE - [budget] doesn't mean the number of finished response(s)
 *  - NOTE - [resp] would be allocated in this function
 * @param budget max. number of response(s) for this time
 * @param resp pointer containing result
 * @return counter of completed response(s) on success, -1 on error
 */
int get_response(int budget, ra_response **resp)
{
	int rp_cnt, rt_cnt, finished;
	struct ra_entry *entry = NULL, *pick = NULL;
	ra_response *result = NULL;

	dbg_printf(DINFO, "start to get response(s) with budget<%d>\n", budget);

	if ((1 > budget) || (NULL == resp)) {
		dbg_printf(DERROR, "<%s> invalid budget<%d>, resp<%p>\n", __func__, budget, resp);
		return 0; /* no result */
	}

	// loop over saved pre-matured responses
	if (CIRCLEQ_EMPTY(&st_head)) {
		dbg_printf(DINFO, "<%s> No RA status\n", __func__);
		return 0; /* empty */
	}

	// RA status is not empty
	if ((NULL == st_mark) || (st_mark == (void *)&st_head)) {
		dbg_printf(DDBG, "<%s> start from first\n", __func__);
		entry = CIRCLEQ_FIRST(&st_head);
	} else {
		dbg_printf(DDBG, "<%s> start from next of last\n", __func__);
		// prevent the case entry would point [&st_head]
		entry = CIRCLEQ_LOOP_NEXT(&st_head, st_mark, qlink);
	}

	for (rp_cnt = 0, rt_cnt = 0; rp_cnt < budget; rp_cnt++) {
		// NOTE - for CIRCLEQ based circular, [last->next != first]
		//        but [last->next == first->prev == &head]
		// NOTE - don't use CIRCLEQ_LOOP_NEXT in this block,
		//        bcz problem comes when budget >> length of RA status
		if (entry == (void *)&st_head) {
			dbg_printf(DDBG, "<%s> no more entry <%d/%d>\n", __func__, rp_cnt, budget);
			st_mark = NULL; /* reset st_mark */
			break; /* done */
		}

		// send GET to OMP for unfinished response
		if (0 == is_finished(entry->resp.decision)) {
			dbg_printf(DDBG, "<%s> send GET to OMP <%d>\n", __func__, entry->resp.decision);
			//rp_cnt++;
			if (send_get_to_omp(&(entry->resp))) {
				dbg_printf(DERROR, "<%s> error in send_get_to_omp\n", __func__);
				st_mark = entry; /* mark this one as processed */
				entry = CIRCLEQ_NEXT(st_mark, qlink);
				continue; /* skip other steps */
				/* give more chance after loop,
				   but might be over-written by next request */
			}
			/* updated with reponse of GET */
		}

		// remember current entry, and move to next
		pick = entry;
		entry = CIRCLEQ_NEXT(entry, qlink);

		// check decision, move finished response into Result
		finished = is_finished(pick->resp.decision);
		if (0 > finished) { /* invalid */
			dbg_printf(DDBG, "<%s> move to Free <%d>\n", __func__, pick->resp.decision);

			CIRCLEQ_REMOVE(&st_head, pick, qlink);
			st_cnt--; // decrement length of RA status

			free(pick);

			continue; /* skip other steps */
		} else if (0 < finished) { /* finished */
			dbg_printf(DDBG, "<%s> move to Result <%d>\n", __func__, pick->resp.decision);

			CIRCLEQ_REMOVE(&st_head, pick, qlink);
			st_cnt--; // decrement length of RA status

			// do [rt_cnt++] on moving finished response into Result
			CIRCLEQ_INSERT_TAIL(&rt_head, pick, qlink);
			rt_cnt++;

			dbg_printf(DDBG, "<%s> counter of Result <%d>\n", __func__, rt_cnt);
		} else {
			dbg_printf(DDBG, "<%s> still unfinished <%d>\n", __func__, pick->resp.decision);
			// for unfinished response, update [st_mark]
			st_mark = pick;
		}
	}

	// malloc for finished responses (rt_cnt)
	dbg_printf(DDBG, "<%s> final counter of Result <%d> <%d/%d>\n", __func__, rt_cnt, rp_cnt, budget);

	if (1 > rt_cnt) {
		return 0; /* no result */
	}

	result = (ra_response *)malloc(sizeof(ra_response) * rt_cnt);
	if (NULL == result) {
		dbg_printf(DCRIT, "<%s> malloc for result is failed: %m\n", __func__);
		return 0; /* no result */
	}

	// copy data into malloc'ed buffer (use memcpy for [ra_response])
	// and remove those responses from Result (free or move to Free)
	// NOTE - don't use CIRCLEQ_REMOVE for cleaning up List
	// NOTE - rp_cnt is reused
	for (rp_cnt = 0; !CIRCLEQ_EMPTY(&rt_head); rp_cnt++) {
		entry = CIRCLEQ_FIRST(&rt_head);

		// check~
		memcpy(result + rp_cnt, &(entry->resp), sizeof(ra_response));

		// remove copied entry of result
		CIRCLEQ_REMOVE(&rt_head, entry, qlink);
		free(entry);
	}

	*resp = result; /* copy result to outside */

	dbg_printf(DINFO, "<%s> <%d> entries are copied and freed\n", __func__, rp_cnt);

	return rt_cnt;
}


/* initialize curl library (should be called by worker thread)
 * NOTE - call only on exiting, bcz curl_global_init is thread unsafe.
 * [See https://curl.haxx.se/libcurl/c/curl_global_init.html]
 * return 0 on success
 */
static int init_ralib_i(char *cfname, char *mIa_addr, char *svc_id, int udc_no, char *auth_id, char *auth_key)
{
	int ret = -1;
	CURLcode rcode = 0;

	/* load config from given file */
	if (cfg_ra) {
		dbg_printf(DDBG, "<%s> remove old config<%p>\n", __func__, cfg_ra);
		free(cfg_ra);
	}

	cfg_ra = (struct tp_config *)calloc(1, sizeof(struct tp_config));
	if (NULL == cfg_ra) {
		dbg_printf(DCRIT, "<%s> error in init_ralib\n", __func__);
		return (-1); /* error */
	}

	if (NULL == cfname) {
		cfg_ra->mIa_addr = mIa_addr;
		cfg_ra->svc_id = svc_id;
		cfg_ra->auth_id = auth_id;
		cfg_ra->auth_key = auth_key;
		cfg_ra->udc_no = udc_no;
	} else {
		if (load_ra_config(cfname, cfg_ra)) {
			dbg_printf(DERROR, "<%s> error in load_ra_config\n", __func__);
			goto err_out;
		}
	}
#ifdef	DBG_VERBOSE
	printf("mIa_addr <%s>\n", cfg_ra->mIa_addr);
	printf("svc_id <%s>\n", cfg_ra->svc_id);
	printf("auth_id <%s>\n", cfg_ra->auth_id);
	printf("auth_key <%s>\n", cfg_ra->auth_key);
	printf("udc_no <%d>\n", cfg_ra->udc_no);
#endif // DBG_VERBOSE

	/* curl_global_init MUST be called by main thread */
	rcode = curl_global_init(CURL_GLOBAL_ALL);
	if (0 != rcode) {
		dbg_printf(DERROR, "error in curl_global_init<0x%02x>: %s\n", rcode, curl_easy_strerror(rcode));
		goto err_out;
	}
	/* curl_global_cleanup would be called on exit */

	CIRCLEQ_INIT(&rt_head);
	CIRCLEQ_INIT(&st_head);

	rt_cnt = st_cnt = 0;

	ret = 0; /* OK */

err_out:
	if (ret && cfg_ra) {
		free(cfg_ra);
		cfg_ra = NULL;
	}

	return ret;
}


/**
 * @brief initialize ralib with data from given file
 * @param cfname name of config file
 * @return 0 on success, -1 on error
 */
int init_ralib2(char *cfname)
{
	dbg_printf(DINFO, "initialize resources for ralib with config file\n");

	return init_ralib_i(cfname, NULL, NULL, 0, NULL, NULL);
}


/**
 * @brief initialize ralib with given data
 * @param mIa_addr IP address of ThingPlug, connection point for App
 * @param svc_id ServiceID of App
 * @param udc_no Number of Userver Defined Control Message
 * @param auth_id AuthID of App
 * @param auth_key AuthKey of App
 * @return 0 on success, -1 on error
 */
int init_ralib(char *mIa_addr, char *svc_id, int udc_no, char *auth_id, char *auth_key)
{
	dbg_printf(DINFO, "initialize resources for ralib with parsed data\n");

	return init_ralib_i(NULL, mIa_addr, svc_id, udc_no, auth_id, auth_key);
}


/* free up all entries linked to given [ra_header]
 * NOTE - free up without moving to free list
 */
void free_entries(ra_header *headp)
{
	int i = 0;
	struct ra_entry *entry = NULL;

	if (cfg_ra) {
		clear_ra_config(cfg_ra);
		free(cfg_ra);
		cfg_ra = NULL;
	}

	if (NULL == headp) {
		dbg_printf(DERROR, "<%s> Invalid header<%p>\n", __func__, headp);
		return;
	}

	for (i = 0; !CIRCLEQ_EMPTY(headp); i++) {
		entry = CIRCLEQ_FIRST(headp);
		CIRCLEQ_REMOVE(headp, entry, qlink);
		free(entry);
	}

	dbg_printf(DDBG, "<%d> entries removed\n", i);
}


/* NOTE - call only on exiting, bcz curl_global_cleanup is thread unsafe.
 * [See https://curl.haxx.se/libcurl/c/curl_global_cleanup.html]
 */
/**
 * @brief clean up resource related with ralib
 */
void cleanup_ralib(void)
{
	dbg_printf(DINFO, "clean up resources for ralib\n");

	/* curl_global_cleanup MUST be called by main thread */
	curl_global_cleanup();

	// cleanup resources
	rt_cnt = 0;
	if (CIRCLEQ_EMPTY(&rt_head)) {
		dbg_printf(DINFO, "clean up result list\n");

		free_entries(&rt_head);
		rt_cnt = 0;
	}

	st_cnt = 0;
	if (CIRCLEQ_EMPTY(&st_head)) {
		dbg_printf(DINFO, "clean up RA status list\n");

		free_entries(&st_head);
		st_cnt = 0;
	}
}
