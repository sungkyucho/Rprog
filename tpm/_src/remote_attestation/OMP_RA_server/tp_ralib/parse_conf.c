#include <stdio.h>
#include <stdlib.h>
#include <string.h>   

#include "tp_ralib_conf.h"	// [struct tp_config]
#include "dbg_macros.h"	// edited for local usage


#define CONFIG_OPT_MIA_ADDR		"mIa_addr"
#define CONFIG_OPT_SERVICE_ID	"svc_id"
#define CONFIG_OPT_UDC_NO		"udc_no"
#define CONFIG_OPT_AUTH_ID		"auth_id"
#define CONFIG_OPT_AUTH_KEY		"auth_key"

#define STD_BUF 1024
const char comment_tag = '#';
const char *cfg_eol = "\n";
const char *cfg_delm = ":";

/* utility functions for parsing */
void config_mIa_addr(struct tp_config *, char *);
void config_svc_id(struct tp_config *, char *);
void config_udc_no(struct tp_config *, char *);
void config_auth_id(struct tp_config *, char *);
void config_auth_key(struct tp_config *, char *);

const struct _cfg_entry {
	const char *name;
	void (*parse_func)(struct tp_config *, char *);
} cfg_handle[] = {
	{CONFIG_OPT_MIA_ADDR, config_mIa_addr},
	{CONFIG_OPT_SERVICE_ID, config_svc_id},
	{CONFIG_OPT_UDC_NO, config_udc_no},
	{CONFIG_OPT_AUTH_ID, config_auth_id},
	{CONFIG_OPT_AUTH_KEY, config_auth_key},
	{NULL, NULL}
};


/* parse and load the content of [fname] into [cfg_ra]
 * NOTE - [cfg_ra] must point the block allocated by caller
 * return 0 on success
 */
int load_ra_config(char *fname, struct tp_config *cfg_ra)
{
	int i;
	FILE *rfp = NULL;
	char *tok, *rest;
	char lbuf[STD_BUF];

	if ((NULL == fname) || (NULL == cfg_ra)) {
		dbg_printf(DERROR, "<%s> invalid fname<%p> or cfg_ra<%p>\n", __func__, fname, cfg_ra);
		return (-1); /* error */
	}

	dbg_printf(DINFO, "<%s> Loading config of RA from <%s>\n", __func__, fname);

	if ((rfp = fopen(fname, "r"))== NULL) {
		dbg_printf(DERROR, "<%s> fopen(%s) failed: %m\n", __func__, fname);
		return (-1); /* error */
	}

	while (fgets(lbuf, STD_BUF, rfp) != NULL) {
		if (comment_tag == lbuf[0]) /* skip comment */
			continue;
		tok = strtok_r(lbuf, cfg_delm, &rest);

		for (i = 0;  cfg_handle[i].name != NULL; i++) {
			if (0 == strcmp(tok, cfg_handle[i].name)) {
				tok = strtok_r(NULL, cfg_eol, &rest);
				cfg_handle[i].parse_func(cfg_ra, tok);
				break;
			}
		}
	}

	fclose(rfp);

	return 0; /* OK */
}


/* free all members in [struct tp_config] and itself
 * NOTE - [cfg_ra] is no more valid on return
 */
void clear_ra_config(struct tp_config *cfg_ra)
{
	if (cfg_ra) {
		if (cfg_ra->mIa_addr) free(cfg_ra->mIa_addr);
		if (cfg_ra->svc_id) free(cfg_ra->svc_id);
		if (cfg_ra->auth_id) free(cfg_ra->auth_id);
		if (cfg_ra->auth_key) free(cfg_ra->auth_key);
	}
}


/* utility functions for parsing */

/* parsing [mIa_addr]: "a.b.c.d:port" --> "a.b.c.d" + port */
void config_mIa_addr(struct tp_config *cfg_ra, char *args)
{
	if ((NULL == cfg_ra) || (NULL == args)) {
		dbg_printf(DERROR, "<%s> invalid cfg_ra<%p> or args<%p>\n", __func__, cfg_ra, args);
		return;
	}

	cfg_ra->mIa_addr = strdup(args);
}


/* parsing [svc_id]: "abc" */
void config_svc_id(struct tp_config *cfg_ra, char *args)
{
	if ((NULL == cfg_ra) || (NULL == args)) {
		dbg_printf(DERROR, "<%s> invalid cfg_ra<%p> or args<%p>\n", __func__, cfg_ra, args);
		return;
	}

	cfg_ra->svc_id = strdup(args);
}


/* parsing [udc_no]: "no" --> port */
void config_udc_no(struct tp_config *cfg_ra, char *args)
{
	if ((NULL == cfg_ra) || (NULL == args)) {
		dbg_printf(DERROR, "<%s> invalid cfg_ra<%p> or args<%p>\n", __func__, cfg_ra, args);
		return;
	}

	cfg_ra->udc_no = atoi(args);
}



/* parsing [auth_id]: "abc" */
void config_auth_id(struct tp_config *cfg_ra, char *args)
{
	if ((NULL == cfg_ra) || (NULL == args)) {
		dbg_printf(DERROR, "<%s> invalid cfg_ra<%p> or args<%p>\n", __func__, cfg_ra, args);
		return;
	}

	cfg_ra->auth_id = strdup(args);
}



/* parsing [auth_key]: "abc" */
void config_auth_key(struct tp_config *cfg_ra, char *args)
{
	if ((NULL == cfg_ra) || (NULL == args)) {
		dbg_printf(DERROR, "<%s> invalid cfg_ra<%p> or args<%p>\n", __func__, cfg_ra, args);
		return;
	}

	cfg_ra->auth_key = strdup(args);
}
