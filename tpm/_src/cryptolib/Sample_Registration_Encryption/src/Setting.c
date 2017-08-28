#include <stdlib.h>
#include <stdio.h>
#include "../GMMP_lib/GMMP.h"
#include "CryptoSuite.h"
#include "Setting.h"
#include "stdlib.h"

static void getSettingsFromJobj(struct config_data_t *pCfg, json_object *jobj)
{
	json_object *j1;
	if (json_object_object_get_ex(jobj, "ServerIP", &j1)) {
	    pCfg->szServerIP = strdup(json_object_get_string(j1));
	}
	if (json_object_object_get_ex(jobj, "ServerPort", &j1)) {
	    pCfg->nServerPort = json_object_get_int(j1);
	}
	if (json_object_object_get_ex(jobj, "DomainCode", &j1)) {
	    pCfg->pszDomainCode = strdup(json_object_get_string(j1));
	}
	if (json_object_object_get_ex(jobj, "GWAuthID", &j1)) {
	    pCfg->pszGWAuthID = strdup(json_object_get_string(j1));
	}
	if (json_object_object_get_ex(jobj, "GWMFID", &j1)) {
	    pCfg->pszGWMFID = strdup(json_object_get_string(j1));
	}
	if (json_object_object_get_ex(jobj, "DeviceMFID", &j1)) {
	    pCfg->pszDeviceMFID = strdup(json_object_get_string(j1));
	}
	if (json_object_object_get_ex(jobj, "ErrorLevel", &j1)) {
	    pCfg->nErrorLevel = json_object_get_int(j1);
	}
}


int read_config(struct config_data_t *pCfg, const char *filename)
{
	char * p;
	json_object *jobj ;
	if (!pCfg || !filename) {
	    return 0;
	}
	jobj = json_object_from_file(filename);
			// -> json_object_from_encrypted_file(filename)
	if (!jobj) {
	    return -1;
	}
	/* load secret key from file (later from TPM) */
	/* at provisioning, secret key is generated from random()(later from TPM) */
	/* decrypt file with secret key */
	/* verify with my public-key(in TPM) */

	getSettingsFromJobj(pCfg, jobj);
	return 0;
}

static int attach_tab(char *ptr, int level)
{
    int i, o=0;
    for (i=0; i<level; ++i) {
	o += sprintf(&ptr[o], "\t");
    }
    return o;
}

static int attach_json_line(char *ptr, int ofst, int limit, int level, char *title)
{
    int i = ofst;
    if (limit < ofst + level + strlen(title) + 1) {
	return 0;
    }
    i += attach_tab(&ptr[i], level);
    i += sprintf(&ptr[i], "%s\n", title);
    return i;
}

static int attach_json_str(char *ptr, int ofst, int limit, int level, char *title, char *value)
{
    int i = ofst;
    if (limit < ofst + level + strlen(title) + strlen(value) + 7) {
	return 0;
    }
    i += attach_tab(&ptr[i], level);
    i += sprintf(&ptr[i], "\"%s\":\"%s\",\n", title, value);
    return i;
}

static int expectedlen(int value)
{
    char buf[16];   /* max digit of int type is 10 */
    return sprintf(buf, "%d", value);
}

static int attach_json_int(char *ptr, int ofst, int limit, int level, char *title, int value)
{
    int i = ofst;
    if (limit < ofst + level + strlen(title) + expectedlen(value) + 5) {
	return 0;
    }
    i += attach_tab(&ptr[i], level);
    i += sprintf(&ptr[i], "\"%s\":%d,\n", title, value);
    return i;
}

/** @brief make string to be written to settings file
 *  @param pCfg configuration
 *  @param cfgs buffer to store configuration string.
 *  @param msize size of buffer
 *  @return length of string
 */
size_t config_to_string(struct config_data_t *pCfg, char *cfgs, int msize)
{
    size_t i = 0;
    /* json_object_to_file */
    i = attach_json_line(cfgs, i, msize, 0, "{");
    i = attach_json_str(cfgs, i, msize, 1, "ServerIP", pCfg->szServerIP);
    i = attach_json_int(cfgs, i, msize, 1, "ServerPort", pCfg->nServerPort);
    i = attach_json_str(cfgs, i, msize, 1, "DomainCode", pCfg->pszDomainCode);
    i = attach_json_str(cfgs, i, msize, 1, "GWAuthID", pCfg->pszGWAuthID);
    i = attach_json_str(cfgs, i, msize, 1, "GWMFID", pCfg->pszGWMFID);
    i = attach_json_str(cfgs, i, msize, 1, "DeviceMFID", pCfg->pszDeviceMFID);
    i = attach_json_int(cfgs, i, msize, 1, "ErrorLevel", pCfg->nErrorLevel);
    i = attach_json_line(cfgs, i, msize, 0, "}");

    return i;
}

int write_config(struct config_data_t *pCfg, const char *filename)
{
	FILE *fp;
	char *cfgs = NULL;
	size_t l;
	size_t allocsize = 2048;
	if (!pCfg || !filename) {
	    return -1;
	};

	cfgs = malloc(allocsize);
	if (!cfgs) {
	    perror("malloc");
	    return -1;
	}

	l = config_to_string(pCfg, cfgs, allocsize);

	fp = fopen(filename, "w");
	if (!fp) {
	    return -1;
	}
	fwrite(cfgs, sizeof(char), l, fp);
	fclose(fp);

	free(cfgs);
	return 0;
}

int free_config(struct config_data_t *pCfg)
{
	if (!pCfg) {
	    return 0;
	}
	if (pCfg->szServerIP) {
	    free(pCfg->szServerIP);
	}
	if (pCfg->pszDomainCode) {
	    free(pCfg->pszDomainCode);
	}
	if (pCfg->pszGWAuthID) {
	    free(pCfg->pszGWAuthID);
	}
	if (pCfg->pszGWMFID) {
	    free(pCfg->pszGWMFID);
	}
	if (pCfg->pszDeviceMFID) {
	    free(pCfg->pszDeviceMFID);
	}
	return 0;
}

/**
 * @brief attach signature to serialized value of config_data_t structure
 *        signature is separated from cfgstr with '===== BEGIN RSA SIGNATURE ====='
 * @param pCfg pointer to config_data_t structure
 * @param signedCfgBuf
 * @param buflen
 * @param privfile PEM formatted file with private key
 * @return
 */
int sign_config(struct config_data_t *pCfg, char *signedCfgBuf, size_t buflen, char *privfile)
{
    int r = 0;
    size_t siglen = 256;
    char sigbuf[256];
    size_t blen = buflen;
    size_t cfglen;

    if (!pCfg || !signedCfgBuf || !buflen || !privfile) {
	return -1;
    }

    cfglen = config_to_string(pCfg, signedCfgBuf, buflen);
    r = gen_signature(privfile, signedCfgBuf, cfglen, sigbuf, &siglen);
    if (r < 0) {
	goto out;
    }

    cfglen += sprintf(&signedCfgBuf[cfglen], "===== BEGIN RSA SIGNATURE =====\n");
    blen = buflen - cfglen;
    r = (size_t)encode_base64(&signedCfgBuf[cfglen], &blen, sigbuf, siglen);
    if (r < 0) {
	goto out;
    }
    cfglen += blen;
    cfglen += sprintf(&signedCfgBuf[cfglen], "===== END RSA SIGNATURE =====\n");

out:
    return r;
}

static int readSettingFromString(struct config_data_t *pCfg, char *cfgstr)
{
    json_object * jobj;
    enum json_tokener_error e;
    jobj = json_tokener_parse_verbose(cfgstr, &e);
    if (e != 0) {
	fprintf(stderr, "Error: %s\n", json_tokener_error_desc(e));
	return -1;
    }
    getSettingsFromJobj(pCfg, jobj);
    return 0;
}

int verify_config(char * signedCfgStr, size_t signedCfgLen, struct config_data_t *pCfg, char *pubfile)
{
    int r = 0;
    int cfglen;
    char sigbuf[512];
    size_t siglen = 512;
    char *p, *p2;
    char *beginmark = "===== BEGIN RSA SIGNATURE =====\n";
    char *endmark = "===== END RSA SIGNATURE =====\n";

    if (!signedCfgStr || !signedCfgLen || !pCfg || !pubfile) {
	return -1;
    }
    p = strstr(signedCfgStr, beginmark);

    p2 = strstr(signedCfgStr, endmark);
    cfglen = (size_t)(p - signedCfgStr);
    p += strlen(beginmark);
    r = decode_base64(sigbuf, &siglen, p, (size_t)(p2-p));
    if (r != 0) {
	fprintf(stderr, "Error: decode_base64()\n");
	goto out;
    }
    r = verify_signature(pubfile, signedCfgStr, cfglen, sigbuf, siglen);
    if (r != 0) {
	fprintf(stderr, "Error: verify_signature()\n");
	goto out;
    }
    signedCfgStr[cfglen] = 0;

    r = readSettingFromString(pCfg, signedCfgStr);

out:
    return r;
}

int encrypt_config(struct config_data_t *pCfg, char *secret, char *encCfgBuf, size_t bufsize)
{
    int r;
    size_t cfglen = 4096;
    char *cfgstr;
    
    if (!pCfg || !secret || !encCfgBuf || !bufsize) {
	return -1;
    }
    cfgstr = malloc(cfglen);
    cfglen = config_to_string(pCfg, cfgstr, cfglen);

    r = sym_encrypt(cfgstr, cfglen, encCfgBuf, &bufsize, secret);
    if (r < 0) {
	goto out;
    }
out:
    if (cfgstr)
	free(cfgstr);
    return r;
}

int decrypt_config(struct config_data_t *pCfg, char *secret, char *encCfgBuf, size_t bufsize)
{
    int r;
    size_t cfglen;
    char *cfgstr;
    
    if (!pCfg || !secret || !encCfgBuf || !bufsize) {
	return -1;
    }
    cfglen = bufsize;
    cfgstr = malloc(cfglen);
    r = sym_decrypt(encCfgBuf, bufsize, cfgstr, &cfglen, secret);
    if (r < 0) {
	goto out;
    }
    r = readSettingFromString(pCfg, cfgstr);
    if (r < 0) {
	goto out;
    }
out:
    if (cfgstr)
	free(cfgstr);
    return r;
}
/* vi: set ai sw=4 ts=8 sts=4: */
