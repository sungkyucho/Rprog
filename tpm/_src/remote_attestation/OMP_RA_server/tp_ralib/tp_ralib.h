#ifndef	_TP_RALIB_H_
#define	_TP_RALIB_H_

#include "tp_ra_type.h"

/* return 0 on success */
int issue_ra(int cnt, ra_request *req);

/* return #cnt of responses
 * NOTE - [resp] would be allocated in this function
 */
int get_response(int budget, ra_response **resp);

/* initialize with config file */
int init_ralib(char *mIa_addr, char *svc_id, int udc_no, char *auth_id, char *auth_key);
int init_ralib2(char *cfname);

/* NOTE - call only on exiting, bcz curl_global_cleanup is thread unsafe.
 * [See https://curl.haxx.se/libcurl/c/curl_global_cleanup.html]
 */
void cleanup_ralib(void);

/* followings are utlities */

/* convert "YYYY-MM-DDThh:mm:ssTZD" into time_t */
time_t get_local_time(char *html_time);

/* Q-related debug/test routines */
void dump_all_entries(void);

/* get [Authorization] field of ThingPlug */
char *get_auth_omp(char *authId, char *authKey);

#endif /* _TP_RALIB_H_ */
