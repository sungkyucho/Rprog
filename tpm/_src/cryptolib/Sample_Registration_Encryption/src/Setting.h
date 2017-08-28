#ifndef __SETTING_H__
#define __SETTING_H__

#include <json-c/json.h>

struct config_data_t {
	/** \brief malloc()'ed string */
	char*	szServerIP;
	/** \brief server's listening port  */
	int	nServerPort;
	/** \brief malloc()'ed string  */
	char*	pszDomainCode;
	/** \brief malloc()'ed string  */
	char*	pszGWAuthID;
	/** \brief malloc()'ed string  */
	char*	pszGWMFID;
	/** \brief malloc()'ed string  */
	char*	pszDeviceMFID;
	/** \brief error level */
	int	nErrorLevel;
};

int read_config(struct config_data_t *pCfg, const char *filename);
int write_config(struct config_data_t *pCfg, const char *filename);
int free_config(struct config_data_t *pCfg);
size_t config_to_string(struct config_data_t *pCfg, char *cfgstr, int msize);

int sign_config(struct config_data_t *pCfg, char *signedCfgBuf, size_t buflen, char *privfile);
int verify_config(char * signedCfgStr, size_t signedCfgLen, struct config_data_t *pCfg, char *pubfile);

int encrypt_config(struct config_data_t *pCfg, char *secret, char *encCfgBuf, size_t bufsize);
int decrypt_config(struct config_data_t *pCfg, char *secret, char *decCfgBuf, size_t bufsize);

#endif /* __SETTING_H__ */
/* vi: set ai sw=4 ts=8 sts=4: */
