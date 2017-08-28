#ifndef	_TP_RALIB_CONF_H_
#define	_TP_RALIB_CONF_H_

struct tp_config {
	char *mIa_addr;		/* ex. 61.250.21.211:10005 */
	char *svc_id;		/* ex. rapoc (max. 10-byte) */
	char *auth_id;		/* ex. AP10009744 (max. 16-byte) */
	char *auth_key;		/* ex. AP10009744 (max. 16-byte) */
	int udc_no;			/* ex. 5 (1 ~ 32) */
};

/* parse and load the content of [fname] into [cfg_ra]
 * NOTE - [cfg_ra] must point the block allocated by caller
 * return 0 on success
 */
int load_ra_config(char *fname, struct tp_config *cfg_ra);

/* free all members in [struct tp_config] and itself
 * NOTE - [cfg_ra] is no more valid on return
 */
void clear_ra_config(struct tp_config *cfg_ra);

#endif /* _TP_RALIB_CONF_H_ */
