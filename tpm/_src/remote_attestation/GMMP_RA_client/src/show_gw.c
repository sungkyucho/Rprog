#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>

#include "../GMMP_lib/GMMP.h"

//const char *szServerIP = "61.250.21.211";
//const int nServerPort = 31002;
char *szServerIP = "61.250.21.211";
int nServerPort = 31002;
char *pszDomainCode = "ThingPlug";
char *pszGWAuthID = "002655EDE8F1";
char *pszGWMFID = "mycorporation";
char *pszMessage = "Temperature=28C";

//const int nErrorLevel = GMMP_ERROR_LEVEL_DEBUG_DATA;
int nErrorLevel = GMMP_ERROR_LEVEL_DEBUG_DATA;
int bTimer = true;

unsigned int recv_sleep = 5; /* default for GMMP_ERROR_LEVEL_DEBUG_DATA */

/*
 * setup global value
 * return 0 on OK
 */
int load_config(char *cfname)
{
	FILE *rfp = NULL;
	int ret = -7; /* default: 7 items are needed */
	char *th, *tv, *rest;
	char cbuf[1024];

	if (NULL == cfname) {
		printf("invalid config file\n");
		return ret;
	}

	rfp = fopen(cfname, "r");
	if (NULL == rfp) {
		printf("open(%s) failed: %m\n", cfname);
		return ret;
	}

	while (fgets(cbuf, sizeof(cbuf), rfp) != NULL) {
		if ('#' == cbuf[0]) continue;

		th = strtok_r(cbuf, ":", &rest);
		tv = strtok_r(NULL, "\n", &rest);

		if ((NULL == th) || (NULL == tv)) {
			printf("invalid config\n");
			return ret;
		}

		if (0 == strcmp(th, "mId_addr")) {
			szServerIP = strdup(tv);
			ret++;
		} else if (0 == strcmp(th, "mId_port")) {
			nServerPort = atoi(tv);
			ret++;
		} else if (0 == strcmp(th, "svc_id")) {
			pszDomainCode = strdup(tv);
			ret++;
		} else if (0 == strcmp(th, "auth_id")) {
			pszGWAuthID = strdup(tv);
			ret++;
		} else if (0 == strcmp(th, "mf_id")) {
			pszGWMFID = strdup(tv);
			ret++;
		} else if (0 == strcmp(th, "mesg")) {
			pszMessage = strdup(tv);
			ret++;
		} else if (0 == strcmp(th, "log")) {
			if (0 == strcmp(tv, "error")) {
				nErrorLevel = GMMP_ERROR_LEVEL_ERROR;
				recv_sleep = 1;
			} else if (0 == strcmp(tv, "debug")) {
				nErrorLevel = GMMP_ERROR_LEVEL_DEBUG;
				recv_sleep = 2;
			} else if (0 == strcmp(tv, "verbose")) {
				nErrorLevel = GMMP_ERROR_LEVEL_DEBUG_DATA;
				recv_sleep = 5;
			} else { /* default: error */
				nErrorLevel = GMMP_ERROR_LEVEL_ERROR;
			}
			ret++;
		} else {
			/* ignore */
			printf("invalid config <%s/%s>\n", th, tv);
		}
	}

	fclose(rfp);

	return ret;
}


void* ReadTCPData(void *data)
{
        GMMPHeader stGMMPHeader;
        void* pBody = NULL;
        int nRet = 0;
        while(bTimer == true) {
                nRet = GetReadData(&stGMMPHeader, &pBody);
                if(nRet != GMMP_SUCCESS) {
                        perror("GetReadData Error\n");
                }

				if (pBody) free(pBody); // pBody could be NULL
                pBody = NULL;
        }

        return NULL;
}


/* values given by OMP */
static char *myGwID = NULL;
static char *myAuthKey = NULL;

int Recv(GMMPHeader* pstGMMPHeader, void* pBody)
{
        //U8 cResultCode;
        U8 cMessageType = pstGMMPHeader->ucMessageType;

        if (cMessageType == OPERATION_GW_REG_RSP) {
                stGwRegistrationRspHdr* pstRspHdr =(stGwRegistrationRspHdr*) pBody;

                if (pstRspHdr->ucResultCode != 0x00) {
                        return 1;
                }

                SetAuthKey((char*)pstGMMPHeader->usAuthKey);
                SetGWID((char*)pstRspHdr->usGWID);

				/* check and store GW_ID and Auth_Key */
				if (myAuthKey) free(myAuthKey);
				myAuthKey = strdup((char *)pstGMMPHeader->usAuthKey);
				if (myGwID) free(myGwID);
				myGwID = strdup((char *)pstRspHdr->usGWID);

				printf("GW ID = %s\n", myGwID);
				printf("Auth Key = %s\n", myAuthKey);
				printf("Auth ID = %s\n", pszGWAuthID);

				pthread_exit(NULL);
        } else {
			// ignore...
        }

        return 0;
}


int Init()
{
        /*
           LOG: GMMP_ON_LOG, GMMP_OFF_LOG
           LOG Level : GMMP_ERROR_LEVEL_ERROR, GMMP_ERROR_LEVEL_DEBUG, GMMP_ERROR_LEVEL_DEBUG_DATA
           CONNECTION: GMMP_NETWORK_ALYWAYS_OFF, GMMP_NETWORK_ALYWAYS_ON
        */
        if (Initializaion(szServerIP, nServerPort, pszDomainCode, pszGWAuthID, GMMP_ON_LOG, nErrorLevel,
                GMMP_NETWORK_ALYWAYS_ON, "Log") != 0) {
                return 1;
        }

        SetCallFunction((void*)Recv);

        return 0;
}


int GW_Reg()
{
        int nRet = GMMP_SUCCESS;

        nRet = GO_Reg(NULL, pszGWMFID, false);

        if (nRet < 0) {
                return 1;
        }

        return 0;
}


int main(int argc, char *argv[])
{
        if (IsBigEndianSystem() == 0) {
                printf(">>>> Big Endian <<<<\n");
        } else {
                printf(">>>> Little Endian <<<<\n");
        }

		if (argc > 1) {
			if (load_config(argv[1])) {
				printf("error in loading config<%s>\n", argv[1]);
				return -1;
			}
		}

        if (Init() != 0) {
                GMMP_Printf(GMMP_ERROR_LEVEL_ERROR, GMMP_LOG_MARKET_ERR, "Init Error\n");
                return -1;
        }

        pthread_t thread_3 = 0;
        pthread_create(&thread_3, NULL, ReadTCPData, NULL);

        /* Multiple Registration is allowed */
        if (GW_Reg() != 0) {
                GMMP_Printf(GMMP_ERROR_LEVEL_ERROR, GMMP_LOG_MARKET_ERR, "GW_Reg Error\n");
                return -1;
        }

        pthread_join(thread_3, NULL);

        return 0;
}

