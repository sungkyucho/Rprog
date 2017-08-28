#include <getopt.h>
#include <sys/types.h>
#include <time.h>
#include <syslog.h>
#include <pthread.h>
#include <mysql/mysql.h>
#include <signal.h>
#include <openssl/rand.h>

#include "maria_db.h"
#include "tp_maria_db.h"
#include "timer.h"
#include "dbg_macros.h"
#include "parse_util.h"
#include "remote_attestation.h"
#include "socket_util.h"
#include "verify_data.h"

#include "tp_ralib.h"
#include "tp_ra_type.h"
#include "tp_server.h"

pthread_mutex_t timer_mutex;
pthread_mutex_t tp_mutex;
SVR_CONF  *s_conf=NULL; 
int call_counter=0;

struct option l_server_options[] = {
	{"config-file", 1, 0, 'c'},
	{"debug", 0, 0, 'd'},
	{"help", 0, 0, 'h'},
	{0, 0, 0, 0}
};

static void
usage(char *argv0)
{
	fprintf(stderr, "\t%s: SecurityPlatform RA Server(Attester)\n"
		"\tusage: %s [options] \n\n"
		"\tOptions:\n"
		"\t\t-c|--config-file   specify configure file.\n"
		"\t\t-d|--debug         debug mode.\n"
		"\t\t-h|--help          print this help message\n"
		,argv0, argv0);
	exit(-1);
}

/**
 * @brief create RA request and timer thread
 *        get response from ThingPlug 
 */
int main(int argc, char* argv[])
{
	int c;
	ra_response *resp = NULL;
	int i, ret, rc;
	MYSQL *mysql = NULL;

	char * conffile=NULL;

	pthread_t t_thread;	
	pthread_t sms_thread;	

	pthread_mutex_init(&timer_mutex,NULL);
	pthread_mutex_init(&tp_mutex,NULL);

	openlog("TPServer", LOG_PID|LOG_CONS, LOG_USER);

	s_conf = (SVR_CONF  *)malloc(sizeof(SVR_CONF ));
	if (s_conf== NULL){
		fprintf(stderr,"fail to memory alloc..\n");
		exit(-1);
	}
	memset(s_conf, 0x00, sizeof(SVR_CONF));
	while (1) {
		c = getopt_long(argc, argv, "hdc:",
				l_server_options, NULL);
		if (c == -1)
			break;

		switch (c) {
			case 'h':
				usage(argv[0]);
				break;
			case 'd':
				dbg_lv = DDBG;
				break;
			case 'c':
				conffile = strdup(optarg);
				rc=parse_server_conf(conffile, s_conf, "=");
				if(!rc)
					usage(argv[0]);
				break;
			default:
				usage(argv[0]);
				break;
		}
	}
	if(conffile == NULL) {
		usage(argv[0]);
	}
	free(conffile);

//	if(s_conf->log_size && s_conf->log_path && s_conf->log_file && s_conf->log_sys)
	if(s_conf->log_size && s_conf->log_path)
		init_log(s_conf->log_size, s_conf->log_path, "tp_server.log", s_conf->log_file, s_conf->log_sys);
	else{
		dbg_printf(DCRIT, "Failed to read log configure.\n");
		usage(argv[0]);
	}

	if( pthread_create(&t_thread, NULL, timer_handler, (void *)&(s_conf->t_interval)) < 0){
		dbg_printf(DERROR, "Failed to create timer thread.\n");
		goto free_conf;
	}

	if( pthread_create(&sms_thread, NULL, sms_handler, (void *)s_conf) < 0){
		dbg_printf(DERROR, "Failed to create sms thread\n");
		goto free_conf;
	}

	usleep(1000 * 1000 * 2); // 2 sec

	while(1)
	{
		dbg_printf(DINFO, "Get RA Response.\n");
		pthread_mutex_lock(&tp_mutex);
		ret = get_response(s_conf->cfg_pull_num > 0 ? s_conf->cfg_pull_num:100, &resp);
		pthread_mutex_unlock(&tp_mutex);
		dbg_printf(DINFO, "RA Response cnt<%d> resp<%p>\n", ret, resp);

		if ((1 > ret) || (NULL == resp)) {
			dbg_printf(DERROR, "<%s> invalid responses\n", __func__);
			usleep(1000 * 1000 * 10); // 10 sec
			if(resp) { free(resp); resp=NULL;}
			continue;
		}

		rc = DB_connect(&mysql, s_conf);
		if(rc == EXIT_FAILURE){
			usleep(1000 * 1000 * 10);
			if(resp) { free(resp); resp=NULL;}
			continue;
		}

		for(i=0; i<ret; i++){
			if(resp[i].decision == 0)
				verify_response(mysql, s_conf, resp[i].msgbody, resp[i].gw_id, resp[i].mtime);
			else
				DB_tp_update_device_info(mysql, s_conf, resp[i].gw_id, resp[i].decision, 0, resp[i].mtime); 
		}
		if (ret < s_conf->cfg_pull_num){
			usleep(1000 * 1000); // 1 sec
		}
		else{
			usleep(1000);
		}
		if(resp) { free(resp); resp=NULL;}

		mysql_close(mysql);
	}
free_conf:
	pthread_mutex_destroy(&timer_mutex);
	pthread_mutex_destroy(&tp_mutex);
	if(s_conf->db_host) free(s_conf->db_host);
	if(s_conf->db_user) free(s_conf->db_user);
	if(s_conf->db_pwd) free(s_conf->db_pwd);
	if(s_conf->db_name) free(s_conf->db_name);
	if(s_conf->db_ra_tbl) free(s_conf->db_ra_tbl);
	if(s_conf->db_his_tbl) free(s_conf->db_his_tbl);
	if(s_conf->db_ans_tbl) free(s_conf->db_ans_tbl);
	if(s_conf->open_port) free(s_conf->open_port);
	if(s_conf->cfg_mIa) free(s_conf->cfg_mIa);
	if(s_conf->cfg_sId) free(s_conf->cfg_sId);
	if(s_conf->cfg_authId) free(s_conf->cfg_authId);
	if(s_conf->cfg_authKey) free(s_conf->cfg_authKey);
	if(s_conf->log_path) free(s_conf->log_path);
	if(s_conf) free(s_conf);
	clean_log();
	closelog();

    return 0;
}

/**
 * @brief increate call_counter  
*/
void trigger_handler(void) {
	pthread_mutex_lock(&timer_mutex);
	if(call_counter > 5){
		pthread_mutex_unlock(&timer_mutex);
		return;
	}
	call_counter++;
	pthread_mutex_unlock(&timer_mutex);

	return;
}

/**
 * @brief verify attestant's response 
 *        PCR composite, nonce, digest
 * @return 0 on success, -1 otherwize
 * @param mysql db connection to query 
 * @param s_conf server configuration  
 * @param resp attestant's response
 * @param gw_id attestant's gateway id 
 * @param mtime attestant's response time 
*/
int verify_response(MYSQL *mysql, SVR_CONF *s_conf, BYTE* resp, char *gw_id, time_t mtime){
	uint32_t pcrCompositeSize=0;
	uint32_t signedPCRSize=0;
	uint32_t keyPubSize=0;

	unsigned char * nonce=NULL;
	unsigned char * pcrComposite=NULL;
	unsigned char * signedPCR=NULL;
	unsigned char * keyPub=NULL;
	unsigned char * pcrAnswer=NULL;
	unsigned char * pcrResp=NULL;
	
	uint8_t ra_result; 
	int rc;
	int result=0;

	memcpy(&pcrCompositeSize, resp, 4);
	memcpy(&signedPCRSize, resp + sizeof(uint32_t),4);
	memcpy(&keyPubSize, resp + (sizeof(uint32_t) * 2), 4);

	pcrComposite=malloc(pcrCompositeSize);
	if(pcrComposite== NULL){
		dbg_printf(DCRIT, "fail to pcrComposite memory alloc..\n");
		result = -1;
		goto clean_exit;
	}
	signedPCR=malloc(signedPCRSize);
	if(signedPCR== NULL){
		dbg_printf(DCRIT, "fail to signedPCR memory alloc..\n");
		if(pcrComposite) free(pcrComposite);
		result = -1;
		goto clean_exit;
	}
	keyPub=malloc(keyPubSize);
	if(keyPub== NULL){
		dbg_printf(DCRIT, "fail to keyPub memory alloc..\n");
		if(pcrComposite) free(pcrComposite);
		if(signedPCR) free(signedPCR); 
		result = -1;
		goto clean_exit;
	}
	pcrAnswer=malloc((PCR_DIGEST_LEN *2) +1);
	if(pcrAnswer== NULL){
		dbg_printf(DCRIT, "fail to pcrAnswer memory alloc..\n");
		if(pcrComposite) free(pcrComposite);
		if(signedPCR) free(signedPCR); 
		if(keyPub) free(keyPub);
		result = -1;
		goto clean_exit;
	}

	memcpy(pcrComposite, resp+3*sizeof(uint32_t), pcrCompositeSize);
	memcpy(signedPCR, resp+3*sizeof(uint32_t)+pcrCompositeSize, signedPCRSize);
	memcpy(keyPub, resp+3*sizeof(uint32_t)+pcrCompositeSize+signedPCRSize, keyPubSize);
	//		memcpy(pcrSelect, resp+4*sizeof(uint32_t)+pcrCompositeSize+signedPCRSize+keyPubSize, pcrSelectSize);

	if(dbg_lv == DDBG){
		print_hex_block((char *)signedPCR, signedPCRSize, "RECV>> Signed PCR");
		print_hex_block((char *)keyPub, keyPubSize, "RECV>> AIK's Public Key");
		print_hex_block((char *)pcrComposite, pcrCompositeSize, "RECV>> PCR Composite");
	}

	print_state_block(DINFO, "Verify Attester's Response");

	if(SUCCESS==verify_pcr_composite(pcrComposite, pcrCompositeSize, keyPub, keyPubSize, signedPCR, signedPCRSize))
		ra_result = 1<<BIT_RA_SIG_VERIFY;
	else
		ra_result = 0;

	dbg_printf(DINFO, "Verify attestant nonce...\n");
	nonce=malloc(NONCE_LEN);
	if(nonce== NULL){
		dbg_printf(DCRIT, "fail to pcrComposite memory alloc..\n");
		result = -1;
		goto clean_exit;
	}
	DB_tp_get_nonce(mysql, s_conf, gw_id, nonce);

	if(SUCCESS == verify_pcr_nonce(pcrComposite, nonce))
		ra_result |= 1 << BIT_RA_NONCE_MATCH ;
	free(nonce);

	dbg_printf(DINFO, "Verify pcr digest...\n");
	rc = DB_tp_get_pcr_answer(mysql, s_conf, pcrAnswer, gw_id);
	if (rc == EXIT_FAILURE ){
		mysql_close (mysql);
		result = -1;
		goto clean_exit;
	}
	if(SUCCESS == verify_pcr_digest(pcrComposite, pcrAnswer))
		ra_result |= 1 << BIT_RA_PCR_MATCH;

	pcrResp=malloc((PCR_DIGEST_LEN *2) +1);
	if(pcrResp== NULL){
		dbg_printf(DCRIT, "fail to pcrResp memory alloc..\n");
		result = -1;
		goto clean_exit;
	}
#ifdef TPM1_1 // TPM 1.1
	hex_to_hexstr((char *)pcrResp, (unsigned char *)pcrComposite + 28, PCR_DIGEST_LEN);
#else
	hex_to_hexstr((char *)pcrResp, (unsigned char *)pcrComposite + QUOTE2_COMPOSITE_IDX_PCR_DIGEST, PCR_DIGEST_LEN);
#endif

	DB_tp_update_device_info(mysql, s_conf, gw_id, 0, ra_result, mtime); 

	free(pcrResp);

clean_exit:
	if(pcrComposite) free(pcrComposite);
	if(signedPCR) free(signedPCR); 
	if(keyPub) free(keyPub);
	if(pcrAnswer) free(pcrAnswer);
	return result;
}

/**
 * @brief RA request to ThingPlug  
 * @param arg server configuration
*/
void *sms_handler(void *arg){
	SVR_CONF *s_conf; 
	MYSQL *mysql = NULL;
	int i, dev_num;
	BYTE nonce[NONCE_LEN+1];
	int r, rc, ret;
	char *nonce_str;

	ra_request *dev_list=NULL;
	s_conf = (SVR_CONF *)arg;

	ret = init_ralib(s_conf->cfg_mIa, s_conf->cfg_sId, 
			s_conf->cfg_uds, s_conf->cfg_authId, s_conf->cfg_authKey);

	nonce_str=malloc((NONCE_LEN*2) +1);
	if(nonce_str== NULL){
		dbg_printf(DCRIT, "fail to nonce_str memory alloc..\n");
	}

	while(1)
	{
		if(call_counter > 0)
		{
			rc = DB_connect(&mysql, s_conf);
			if(rc == EXIT_FAILURE){
				usleep(1000 * 1000 * 10);
				continue;
			}

			rc = DB_tp_get_ra_list(mysql, s_conf, &dev_list, &dev_num);
			if(rc == EXIT_FAILURE ){
				dbg_printf(DERROR, "Failed to get RA device list\n");
				mysql_close(mysql);
				usleep(1000);
				continue;
			}
			dbg_printf(DINFO, "Get RA device list and number(%d) from DB\n", dev_num);

			for(i=0; i<dev_num; i++){
				r = RAND_bytes(nonce, (sizeof(BYTE) * NONCE_LEN));
				if(r != 1){
					dbg_printf(DERROR, "RAND_bytes failed");
					memset(dev_list[i].nonce, 0x00, NONCE_LEN);
					continue;
				}
				memcpy((char *)dev_list[i].nonce, nonce, NONCE_LEN);

				hex_to_hexstr(nonce_str, (unsigned char *)nonce, NONCE_LEN);
				rc = DB_tp_update_nonce(mysql, s_conf, dev_list[i].gw_id, (char *)nonce_str);
				if (rc == EXIT_FAILURE ){
					dbg_printf(DERROR, "Failed to update RA request :%s\n", dev_list[i].gw_id);
				}
			}
			dbg_printf(DINFO, "RA request to ThingPlug.\n");
			pthread_mutex_lock(&tp_mutex);
			ret = issue_ra(dev_num, dev_list);
			pthread_mutex_unlock(&tp_mutex);
			if (0 > ret) {
				dbg_printf(DERROR, "error in RA request\n");
			} else if (dev_num != ret) {
				dbg_printf(DERROR, "Queue is full, retry RA request\n");
			} else {
				dbg_printf(DINFO, "RA request success\n");
			}

			free(dev_list);
			mysql_close(mysql);

			pthread_mutex_lock(&timer_mutex);
			call_counter--;
			if(call_counter < 0)
				call_counter = 0;
			pthread_mutex_unlock(&timer_mutex);
		}
		else{ //  sleep 1ms 
			usleep(1000);
		}
	}
	free(nonce_str);
}

/**
 * @brief call trigger_handler every given time. 
 * @param arg time interval from configuration 
*/
void *timer_handler(void *arg){
	double interval = *((double  *)arg);
	sp_timer_create();
	sp_timer_register_handler(trigger_handler);

	dbg_printf(DINFO, "Starting timer...\nevery [%f] sec\n", interval);

	sp_timer_start(1, (double)interval);

	while (1) 
	{
		/*
		sigset_t signal_set;
		sigfillset(&signal_set);
		sigdelset(&signal_set, SIGALRM);
		sigsuspend(&signal_set); 
		sp_timer_start(1, (double)interval);
		*/
		usleep(800);
	}
	sp_timer_delete();
	return 0;
}


