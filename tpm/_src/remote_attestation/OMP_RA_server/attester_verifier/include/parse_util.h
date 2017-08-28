#ifndef _PARSE_UTIL_H_
#define _PARSE_UTIL_H_

#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>

#define CONFIG_OPT_DB_HOST "db_hostname"
#define CONFIG_OPT_DB_USER "db_user"
#define CONFIG_OPT_DB_PWD  "db_password"
#define CONFIG_OPT_DB_NAME "db_name"
#define CONFIG_OPT_DB_RA_TBL "db_ra_table"
#define CONFIG_OPT_DB_HIS_TBL "db_history"
#define CONFIG_OPT_DB_ANS_TBL "db_answer"
#define CONFIG_OPT_PORT_NUM "open_port"
#define CONFIG_OPT_TIMER_INT "timer_interval"
#define CONFIG_OPT_PCR_SELECT "pcr_select"
#define CONFIG_OPT_LOG_PATH "log_path"
#define CONFIG_OPT_LOG_SIZE "log_size"
#define CONFIG_OPT_LOG_TO_FILE "log_to_file"
#define CONFIG_OPT_LOG_TO_SYS "log_to_syslog"

/* ThingPlug configure */
#define CONFIG_OPT_TP_MIA "cfg_mIa"
#define CONFIG_OPT_TP_SID "cfg_sId"
#define CONFIG_OPT_TP_UDS "cfg_uds"
#define CONFIG_OPT_TP_AUTH_ID "cfg_authId"
#define CONFIG_OPT_TP_AUTH_KEY "cfg_authKey"
#define CONFIG_OPT_TP_FR_MAX "cfg_fr_max"
#define CONFIG_OPT_TP_RT_MAX "cfg_rt_max"
#define CONFIG_OPT_TP_ST_MAX "cfg_st_max"
#define CONFIG_OPT_TP_PULL_NUM "cfg_pull_num"

#define STD_BUF 256
// RA Server 
typedef struct ra_server_config{
	char * db_host;
	char * db_user;
	char * db_pwd;
	char * db_name;
	char * db_ra_tbl;
	char * db_his_tbl;
	char * db_ans_tbl;
	char * open_port;
	double t_interval;
	char pcr_select[3];
	int pcr_idx_sum;
	uint32_t log_size;
	char *log_path;
	uint8_t log_file;
	uint8_t log_sys;
	/* ThingPlug config */
	char *cfg_mIa; 
	char *cfg_sId;
	int cfg_uds; /* NOTE: 1 ~ 32 */
	char *cfg_authId;
	char *cfg_authKey;
	int cfg_fr_max; 
	int cfg_rt_max; 
	int cfg_st_max;
	int cfg_pull_num;

}SVR_CONF;
typedef void (*ParseSConfigFunc)(SVR_CONF *, char *);
typedef struct _SvrConfigFunc
{
	const char *name;
	int args_requried;
	ParseSConfigFunc parse_func;
} SvrConfigFunc;


// RA Receiver
typedef struct r_conf{
	char * x509_path;
	unsigned int port;
	char *caaddr;
	char *interface;
	uint32_t log_size;
	char *log_path;
	uint8_t log_file;
	uint8_t log_sys;
}R_CONF;

typedef void (*ParseConfigFunc)(R_CONF*, char *);
typedef struct _RcvConfigFunc
{
	const char *name;
	int args_requried;
	ParseConfigFunc parse_func;
} RcvConfigFunc;

// RA Server 
int parse_server_conf(const char *conffile, SVR_CONF * conf_data, const char *);
void ConfigDbHost(SVR_CONF  *, char *);
void ConfigDbUser(SVR_CONF  *, char *);
void ConfigDbPwd(SVR_CONF  *, char *);
void ConfigDbName(SVR_CONF  *, char *);
void ConfigDbRaTable(SVR_CONF  *s_conf, char *args);
void ConfigDbHisTable(SVR_CONF  *s_conf, char *args);
void ConfigDbAnsTable(SVR_CONF  *s_conf, char *args);
void ConfigOpenPort(SVR_CONF  *, char *);
void ConfigTimerInterval(SVR_CONF  *, char *);
void ConfigPcrSelect(SVR_CONF *, char *args);
void ConfigLogPath(SVR_CONF *s_conf, char *args);
void ConfigLogSize(SVR_CONF *s_conf, char *args);
void ConfigLogToFile(SVR_CONF *s_conf, char *args);
void ConfigLogToSys(SVR_CONF *s_conf, char *args);

void ConfigTpMia(SVR_CONF *r_conf, char *args);
void ConfigTpSid(SVR_CONF *r_conf, char *args);
void ConfigTpUds(SVR_CONF *r_conf, char *args);
void ConfigTpAuthId(SVR_CONF *r_conf, char *args);
void ConfigTpAuthKey(SVR_CONF *r_conf, char *args);
void ConfigTpFrMax(SVR_CONF *r_conf, char *args);
void ConfigTpRtMx(SVR_CONF *r_conf, char *args);
void ConfigTpStMax(SVR_CONF *r_conf, char *args);
void ConfigTpPullNum(SVR_CONF *r_conf, char *args);

// RA Receiver
int parse_recv_conf(const char *conffile, R_CONF* conf_data);
void ConfigRecvPort(R_CONF *r_conf, char *args);
void ConfigPCA(R_CONF *r_conf, char *args);
void ConfigInterface(R_CONF *r_conf, char *args);
void ConfigX509Path(R_CONF *r_conf, char *args);
void ConfigRLogPath(R_CONF *r_conf, char *args);
void ConfigRLogSize(R_CONF *r_conf, char *args);
void ConfigRLogToFile(R_CONF *r_conf, char *args);
void ConfigRLogToSys(R_CONF *r_conf, char *args);

#endif
