#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "parse_util.h"

const SvrConfigFunc s_config_opts[] = {
	{CONFIG_OPT_DB_HOST, 1, ConfigDbHost},
	{CONFIG_OPT_DB_USER, 1, ConfigDbUser},
	{CONFIG_OPT_DB_PWD, 1, ConfigDbPwd},
	{CONFIG_OPT_DB_NAME, 1, ConfigDbName},
	{CONFIG_OPT_DB_RA_TBL, 1, ConfigDbRaTable},
	{CONFIG_OPT_DB_HIS_TBL, 1, ConfigDbHisTable},
	{CONFIG_OPT_DB_ANS_TBL, 1, ConfigDbAnsTable},
	{CONFIG_OPT_PORT_NUM, 1, ConfigOpenPort},
	{CONFIG_OPT_TIMER_INT, 1, ConfigTimerInterval},
	{CONFIG_OPT_PCR_SELECT, 1, ConfigPcrSelect},
	{CONFIG_OPT_LOG_PATH, 1, ConfigLogPath},
	{CONFIG_OPT_LOG_SIZE, 1, ConfigLogSize},
	{CONFIG_OPT_LOG_TO_FILE, 1, ConfigLogToFile},
	{CONFIG_OPT_LOG_TO_SYS, 1, ConfigLogToSys},
	/* ThingPlug configure */
	{CONFIG_OPT_TP_MIA, 1, ConfigTpMia},
	{CONFIG_OPT_TP_SID, 1, ConfigTpSid},
	{CONFIG_OPT_TP_UDS, 1, ConfigTpUds},
	{CONFIG_OPT_TP_AUTH_ID, 1, ConfigTpAuthId},
	{CONFIG_OPT_TP_AUTH_KEY, 1, ConfigTpAuthKey},
	{CONFIG_OPT_TP_FR_MAX, 1, ConfigTpFrMax},
	{CONFIG_OPT_TP_RT_MAX, 1, ConfigTpRtMx},
	{CONFIG_OPT_TP_ST_MAX, 1, ConfigTpStMax},
	{CONFIG_OPT_TP_PULL_NUM, 1, ConfigTpPullNum},
	{NULL, 0, NULL}
};

const RcvConfigFunc r_config_opts[] = {
	{"x509_path", 1, ConfigX509Path},
	{"privacy_ca", 1, ConfigPCA},
	{"port", 1, ConfigRecvPort},
	{"interface", 1, ConfigInterface},
	{"log_path", 1, ConfigRLogPath},
	{"log_size", 1, ConfigRLogSize},
	{"log_to_file", 1, ConfigRLogToFile},
	{"log_to_syslog", 1, ConfigRLogToSys},
	{NULL, 0, NULL}
};

/**
 * @brief parse server configuration 
 * @return 1 on success, 0 otherwize
 * @param conffile configuration file path and name
 * @param conf_data structure to store configuration value
 * @param seperator seperate the key and value in the configuration
*/
int parse_server_conf(const char *conffile, SVR_CONF * conf_data, const char *seperator){
	FILE *fp;
	char line[256];
	char *tok;
	int i;

	if ((fp = fopen(conffile, "r"))== NULL)
	{
		fprintf(stderr,"[%s] open fail exiting..\n", conffile);
		free(conf_data);
		return 0;
	}

	while(fgets(line, 256, fp) != NULL){
		if(line[0] == '#') continue;
		tok = strtok(line, seperator);	

		for(i=0;  s_config_opts[i].name != NULL; i++){
			if(0 == strcmp(tok, s_config_opts[i].name)){
				tok = strtok(NULL, "\n");
				s_config_opts[i].parse_func(conf_data, tok);
				break;
			}
		}
	}
	fclose(fp);
	return 1;
}

void ConfigDbHost(SVR_CONF  *s_conf, char *args){
	if(args == NULL)
		return;

	s_conf->db_host= strdup(args);
//	printf("database hostname:%s\n", s_conf->db_host);
}
void ConfigDbUser(SVR_CONF   *s_conf, char *args){
	if(args == NULL)
		return;

	s_conf->db_user= strdup(args);
//	printf("database user name:%d\n", s_conf->db_user);
}
void ConfigDbPwd(SVR_CONF   *s_conf, char *args){
	if(args == NULL)
		return;

	s_conf->db_pwd = strdup(args);
//	printf("database password:%d\n", s_conf->db_pwd);
}
void ConfigDbName(SVR_CONF  *s_conf, char *args){
	if(args == NULL)
		return;

	s_conf->db_name= strdup(args);
//	printf("database name:%d\n", s_conf->db_name);
}
void ConfigDbRaTable(SVR_CONF  *s_conf, char *args){
	if(args == NULL)
		return;

	s_conf->db_ra_tbl= strdup(args);
}
void ConfigDbHisTable(SVR_CONF  *s_conf, char *args){
	if(args == NULL)
		return;

	s_conf->db_his_tbl= strdup(args);
}
void ConfigDbAnsTable(SVR_CONF  *s_conf, char *args){
	if(args == NULL)
		return;

	s_conf->db_ans_tbl= strdup(args);
}
void ConfigOpenPort(SVR_CONF  *s_conf, char *args){
	if(args == NULL)
		return;

	s_conf->open_port = strdup(args);
}
void ConfigTimerInterval(SVR_CONF  *s_conf, char *args){
	int value;
	if(args == NULL)
		return;

	value = atoi(args);
	if (value < -1){
		return;
	}
	s_conf->t_interval = (double)value;
}
void ConfigPcrSelect(SVR_CONF  *s_conf, char *args){
	int idx;
	int sum=0;
	char *tok, *argp;
	if(args == NULL)
		return;
	
	argp = args;
	while(argp != NULL){
		tok = strsep(&argp, ",");
		idx = atoi(tok);	
		s_conf->pcr_select[idx >> 3]  |= 1 << (idx & 7); 
		sum += 1 << idx;
	}
	s_conf->pcr_idx_sum=sum;
}

void ConfigLogPath(SVR_CONF *s_conf, char *args){
	if(args == NULL)
		return;

	s_conf->log_path = strdup(args);
}

void ConfigLogSize(SVR_CONF *s_conf, char *args){
    char *end = NULL;
	if(args == NULL)
		return;

    int64_t size = strtoll(args, &end, 10);
	/* set to default log size 10M */
	if (size < 0){
		fprintf(stderr,"log file size is invalid. [%jd] ..\n", size);
		s_conf->log_size = 10 * 1024 * 1024; 
		return;
	}

    switch (*end) {
        case 0:
			break;
        case 'k':
        case 'K':
            size *= 1024;
			break;
        case 'm':
        case 'M':
            size *= 1024 * 1024;
			break;
        case 'g':
        case 'G':
            size *= 1024 * 1024 * 1024;
			break;
        default:
			break;
    }
	if(size < 10 * 1024){
		fprintf(stderr,"log file size is too small. [%jd] ..\n", size);
		size = 10 * 1024 * 1024; 
	}
	s_conf->log_size = (uint32_t)size; 
}

void ConfigLogToFile(SVR_CONF *s_conf, char *args){
	char *tok, *argp;
	if(args == NULL){
		s_conf->log_file = 0x0;
		return;
	}
	
	argp = args;
	while(argp != NULL){
		tok = strsep(&argp, ",");
		if(!strncmp(tok, "INFO", strlen("INFO")))
			s_conf->log_file |= 1 << 1; 
		else if(!strncmp(tok, "ERROR", strlen("ERROR")))
			s_conf->log_file |= 1 << 2; 
		else if(!strncmp(tok, "CRITICAL", strlen("CRITICAL")))
			s_conf->log_file |= 1 << 3; 
	}
}

void ConfigLogToSys(SVR_CONF *s_conf, char *args){
	char *tok, *argp;
	if(args == NULL){
		s_conf->log_sys = 0x0;
		return;
	}
	
	argp = args;
	while(argp != NULL){
		tok = strsep(&argp, ",");
		if(!strncmp(tok, "INFO", strlen("INFO")))
			s_conf->log_sys |= 1 << 1; 
		else if(!strncmp(tok, "ERROR", strlen("ERROR")))
			s_conf->log_sys |= 1 << 2; 
		else if(!strncmp(tok, "CRITICAL", strlen("CRITICAL")))
			s_conf->log_sys |= 1 << 3; 
	}
}


/**
 * @brief parse receiver configuration 
 * @return 1 on success, 0 otherwize
 * @param conffile configuration file path and name
 * @param conf_data structure to store configuration value
*/
int parse_recv_conf(const char *conffile, R_CONF* conf_data){
	FILE *fp;
	char line[STD_BUF];
	char *tok;
	int i;

	if ((fp = fopen(conffile, "r"))== NULL)
	{
		fprintf(stderr,"[%s] open fail exiting..\n", conffile);
		free(conf_data);
		return 0;
	}

	while(fgets(line, STD_BUF, fp) != NULL){
		if(line[0] == '#') continue;
		tok = strtok(line, ":");	

		for(i=0; r_config_opts[i].name != NULL; i++){
			if(0 == strcmp(tok, r_config_opts[i].name)){
				tok = strtok(NULL, "\n");
				r_config_opts[i].parse_func(conf_data, tok);
				break;
			}
		}
	}
	fclose(fp);
	return 1;
}
void ConfigRecvPort(R_CONF *r_conf, char *args){
	int value;
	if(args == NULL)
		return;

	value = atoi(args);
	if (value < -1){
		return;
	}
	r_conf->port = value;
//	printf("server open port:%d\n", s_conf->server_port);
}

void ConfigPCA(R_CONF *r_conf, char *args){
	if(args == NULL)
		return;

	r_conf->caaddr= strdup(args);
//	printf("ca addr:%s\n", r_conf->caaddr);
}
void ConfigInterface(R_CONF *r_conf, char *args){
	if(args == NULL)
		return;

	r_conf->interface= strdup(args);
}
void ConfigX509Path(R_CONF *r_conf, char *args){
	if(args == NULL)
		return;

	r_conf->x509_path= strdup(args);
//	printf("database name:%d\n", s_conf->db_name);
}

void ConfigRLogPath(R_CONF *r_conf, char *args){
	if(args == NULL)
		return;

	r_conf->log_path = strdup(args);
}

void ConfigRLogSize(R_CONF *r_conf, char *args){
    char *end = NULL;
	if(args == NULL)
		return;

    int64_t size = strtoll(args, &end, 10);
	/* set to default log size 10M */
	if (size < 0){
		fprintf(stderr,"log file size is invalid. [%jd] ..\n", size);
		r_conf->log_size = 10 * 1024 * 1024; 
		return;
	}

    switch (*end) {
        case 0:
			break;
        case 'k':
        case 'K':
            size *= 1024;
			break;
        case 'm':
        case 'M':
            size *= 1024 * 1024;
			break;
        case 'g':
        case 'G':
            size *= 1024 * 1024 * 1024;
			break;
        default:
			break;
    }
	if(size < 10 * 1024){
		fprintf(stderr,"log file size is too small. [%jd] ..\n", size);
		size = 10 * 1024 * 1024; 
	}
	r_conf->log_size = (uint32_t)size; 
}

void ConfigRLogToFile(R_CONF *r_conf, char *args){
	char *tok, *argp;
	if(args == NULL){
		r_conf->log_file = 0x0;
		return;
	}
	
	argp = args;
	while(argp != NULL){
		tok = strsep(&argp, ",");
		if(!strncmp(tok, "INFO", strlen("INFO")))
			r_conf->log_file |= 1 << 1; 
		else if(!strncmp(tok, "ERROR", strlen("ERROR")))
			r_conf->log_file |= 1 << 2; 
		else if(!strncmp(tok, "CRITICAL", strlen("CRITICAL")))
			r_conf->log_file |= 1 << 3; 
	}
}

void ConfigRLogToSys(R_CONF *r_conf, char *args){
	char *tok, *argp;
	if(args == NULL){
		r_conf->log_sys = 0x0;
		return;
	}
	
	argp = args;
	while(argp != NULL){
		tok = strsep(&argp, ",");
		if(!strncmp(tok, "INFO", strlen("INFO")))
			r_conf->log_sys |= 1 << 1; 
		else if(!strncmp(tok, "ERROR", strlen("ERROR")))
			r_conf->log_sys |= 1 << 2; 
		else if(!strncmp(tok, "CRITICAL", strlen("CRITICAL")))
			r_conf->log_sys |= 1 << 3; 
	}
}

void ConfigTpMia(SVR_CONF *r_conf, char *args){
	if(args == NULL)
		return;

	r_conf->cfg_mIa= strdup(args);
}
void ConfigTpSid(SVR_CONF *r_conf, char *args){
	if(args == NULL)
		return;

	r_conf->cfg_sId= strdup(args);
}
void ConfigTpUds(SVR_CONF *r_conf, char *args){
	int value;
	if(args == NULL)
		return;

	value = atoi(args);
	if (value < 1 || value > 32){
		r_conf->cfg_uds = 5;
		return;
	}
	r_conf->cfg_uds = value;
}
void ConfigTpAuthId(SVR_CONF *r_conf, char *args){
	if(args == NULL)
		return;

	r_conf->cfg_authId= strdup(args);
}
void ConfigTpAuthKey(SVR_CONF *r_conf, char *args){
	if(args == NULL)
		return;

	r_conf->cfg_authKey= strdup(args);
}
void ConfigTpFrMax(SVR_CONF *r_conf, char *args){
	int value;
	if(args == NULL)
		return;

	value = atoi(args);
	if (value < 0 ){
		r_conf->cfg_fr_max= 1000;
		return;
	}
	r_conf->cfg_fr_max= value;
}
void ConfigTpRtMx(SVR_CONF *r_conf, char *args){
	int value;
	if(args == NULL)
		return;

	value = atoi(args);
	if (value < 0){
		r_conf->cfg_rt_max= 100;
		return;
	}
	r_conf->cfg_rt_max= value;
}
void ConfigTpStMax(SVR_CONF *r_conf, char *args){
	int value;
	if(args == NULL)
		return;

	value = atoi(args);
	if (value < 0 ){
		r_conf->cfg_st_max= 1000;
		return;
	}
	r_conf->cfg_st_max = value;
}
void ConfigTpPullNum(SVR_CONF *r_conf, char *args){
	int value;
	if(args == NULL)
		return;

	value = atoi(args);
	if (value < 0 ){
		r_conf->cfg_pull_num= 100;
		return;
	}
	r_conf->cfg_pull_num = value;
}
