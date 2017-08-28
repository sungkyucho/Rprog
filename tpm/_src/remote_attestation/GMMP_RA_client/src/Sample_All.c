#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>

#include "../GMMP_lib/GMMP.h"
#include "ralib.h"

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
const int nTID = 10;
int bTimer = true;
int bProfile = false;
int nTimer;
int nHBTimer;

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


// only for removing warnings
int GW_Delivery();
int GW_Profile();


void* ReadTCPData(void *data) {
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

void* SendData() {
        while(bProfile == false) {
                sleep(1);
        }

        while(bProfile == true) {
                if(GW_Delivery() != 0) {
                        break;
                }

                sleep(nTimer * 60);
        }

        return NULL;
}

void* SendHB() {
        while(bProfile == false) {
                sleep(1);
        }

        while(bProfile == true) {
                SetTID(GetTID()+1);

                int nRet = GMMP_SetHB(pszGWAuthID, g_szAuthKey, pszDomainCode, GetGWID());
                if(nRet != GMMP_SUCCESS) {
                }

                sleep(nHBTimer * 60);
        }

        return NULL;
}


int Recv(GMMPHeader* pstGMMPHeader, void* pBody) {
        //U8 cResultCode;
        U8 cMessageType = pstGMMPHeader->ucMessageType;

        if (cMessageType == OPERATION_GW_REG_RSP) {
                stGwRegistrationRspHdr* pstRspHdr =(stGwRegistrationRspHdr*) pBody;

                if (pstRspHdr->ucResultCode != 0x00) {
                        return 1;
                }

                SetAuthKey((char*)pstGMMPHeader->usAuthKey);
                SetGWID((char*)pstRspHdr->usGWID);

                if (GW_Profile() != 0) {
                        GMMP_Printf(GMMP_ERROR_LEVEL_ERROR, GMMP_LOG_MARKET_ERR, "GW_Profile Error\n");
                        return -1;
                }
        }
        else if(cMessageType == OPERATION_PROFILE_RSP) {
                stProfileRspHdr* pstRspHdr =(stProfileRspHdr*) pBody;

                if(pstRspHdr->ucResultCode != 0x00) {
                        return 1;
                }

                /* can get additional profile information */
                nTimer = Char2int((char *)pstRspHdr->unReportPeriod, sizeof(pstRspHdr->unReportPeriod));
                nHBTimer = Char2int((char *)pstRspHdr->unHeartbeatPeriod, sizeof(pstRspHdr->unHeartbeatPeriod));

                bProfile = true;
        }
        else if(cMessageType == OPERATION_DELIVERY_RSP) {
                stPacketDeliveryRspHdr* pstRspHdr =(stPacketDeliveryRspHdr*) pBody;

                if(pstRspHdr->ucResultCode != 0x00)
                {
                        return 1;
                }
        }
        else if(cMessageType == OPERATION_HEARTBEAT_RSP) {
        }
        else if(cMessageType == OPERATION_CONTROL_REQ) {
                stControlReqHdr* pstReqHdr = (stControlReqHdr*) pBody;

                U8 cControlType = pstReqHdr->ucControlType;

                char cResult = 0x00;

                int nTID_RECV = Char2int((char*)pstGMMPHeader->usTID, sizeof(pstGMMPHeader->usTID));

				// NOTE - Response(= ACK) would be sent in <GO_Control>
                GO_Control((char*)pstReqHdr->usGWID, (char*)pstReqHdr->usDeviceID, nTID_RECV, (char)pstReqHdr->ucControlType, cResult);

				/* Processing RA message */
				if ((cControlType >= User_defined_Control_Min) && (cControlType <= User_defined_Control_Max)) {
					int ra_ret = 0; /* default: OK */
					printf("CONTROL User-Defined Control <0x%hhx>...\n", cControlType);
					// pstGMMPHeader points GMMP_Header, pstReqHdr points Control_Header
					ra_ret = GMMP_Do_RA(pstGMMPHeader, pstReqHdr);
					if (ra_ret == RA_OK) {
						printf("GMMP_Do_RA - OK\n");
					} else if (ra_ret == RA_BYPASS) {
						printf("GMMP_Do_RA - Not RA Request\n");
					} else {
						printf("GMMP_Do_RA - error <0x%x>\n", ra_ret);
					}

					if (ra_ret != RA_BYPASS) {
						sleep(1);

						// NOTE - GO_Notifi should be called by builer of composite
						return ra_ret;
					}
					// RA_BYPASS fall through...
				}

	        switch(cControlType) {
                	case CONTROL_Reset:
                                printf("CONTROL_Reset process...\n");
                                break;
                	case CONTROL_Turn_Off:
                       		printf("CONTROL_Turn_Off process...\n");
                                break;
                	case CONTROL_Report_On:
                                printf("CONTROL_Report_On process...\n");
                                break;
                	case CONTROL_Report_Off:
                                printf("CONTROL_Report_Off process...\n");
                                break;
                	case CONTROL_Time_Sync:
                                printf("CONTROL_Time_Sync process...\n");
                                break;
                	case CONTROL_Pause:
                                printf("CONTROL_Pause process...\n");
                                break;
                	case CONTROL_Restart:
                                printf("CONTROL_Restart process...\n");
                                break;
                	case CONTROL_Signal_Power_Check:
                                printf("CONTROL_Signal_Power_Check process...\n");
                                break;
                	case CONTROL_Diagnostic:
                                printf("CONTROL_Diagnostic process...\n");
                                break;
                	case CONTROL_Reserved:
                                printf("CONTROL_Reserved process...\n");
                                break;
                	case CONTROL_Profile_Reset:
                                printf("CONTROL_Profile_Reset process...\n");
                                break;
                	case CONTROL_Status_Check:
                                printf("CONTROL_Status_Check process...\n");
                                break;
                	case CONTROL_FW_Download:
                                printf("CONTROL_FW_Download process...\n");
                                break;
                	case CONTROL_FW_Update:
                                printf("CONTROL_FW_Update process...\n");
                                break;
                	case CONTROL_App_Download:
                                printf("CONTROL_App_Download process...\n");
                                break;
                	case CONTROL_App_Update:
                                printf("CONTROL_App_Update process...\n");
                                break;
                	case CONTROL_Remote_Access:
                                printf("CONTROL_Remote_Access process...\n");
                                break;
                	case CONTROL_Multimedia_Control_Start:
                                printf("CONTROL_Multimedia_Control_Start process...\n");
                                break;
                	case CONTROL_Multimedia_Control_Pause:
                                printf("CONTROL_Multimedia_Control_Pause process...\n");
                                break;
                	case CONTROL_Multimedia_Control_Stop:
                                printf("CONTROL_Multimedia_Control_Stop process...\n");
                                break;
                	case CONTROL_Multimedia_Control_Restart:
                                printf("CONTROL_Multimedia_Control_Restart process...\n");
                                break;
                	case CONTROL_Frequency_Band_Change:
                                printf("CONTROL_Frequency_Band_Change process...\n");
                                break;
                	case CONTROL_Mode_Change:
                                printf("CONTROL_Mode_Change process...\n");
                                break;
                	case CONTROL_Communication_Status_Check:
                                printf("CONTROL_Communication_Status_Check process...\n");
                                break;
                	case CONTROL_Firmware_Version_Check:
                                printf("CONTROL_Firmware_Version_Check process...\n");
                                break;
                	default:
                        	printf("0x%x\n", pstReqHdr->ucControlType);
                        	break;
        	}

				sleep(recv_sleep);

                GO_Notifi((char*)pstReqHdr->usGWID, (char*)pstReqHdr->usDeviceID, (char)pstReqHdr->ucControlType, cResult, NULL, 0);
        }
        else if(cMessageType == OPERATION_NOTIFICATION_RSP) {
        }


        return 0;
}


#define	PCA_ADDR	"223.39.121.20:8080"
//#define	PCA_ADDR	"192.168.122.13:8888"
#define	CERT_SAVE	"/home/root/SandBox/GMMP_RAc/Debug/x509.pem"

int Init() {
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

		/* init RA with default udc_no(= 5) */
		if (init_RA(PCA_ADDR, CERT_SAVE, 5)) {
			printf("<%s> error in init_RA\n", __func__);
			return (-1);
		}
		init_log(100*1024, "./", "Sample_tp_attestant.log", 1<<1 | 1<<2 , 1<<3);

        return 0;
}

int GW_Reg() {
        int nRet = GMMP_SUCCESS;

        nRet = GO_Reg(NULL, pszGWMFID, false);

        if (nRet < 0) {
                return 1;
        }

        return 0;
}

int GW_Profile() {
        int nRet = GMMP_SUCCESS;

        /* has to use Gateway ID generated by Mobius */
        nRet = GO_Profile(GetGWID(), NULL, nTID);

        if(nRet < 0) {
                return 1;
        }

        return 0;
}

int GW_Delivery() {
        int nRet = 0;
        int nTotalCount = 0;
        int nLoop = 0;
        int nMessageBodyLen = strlen(pszMessage);

        if(nMessageBodyLen < MAX_MSG_BODY) {
                nTotalCount = 1;
        }
        else {
                nTotalCount = nMessageBodyLen/MAX_MSG_BODY;

                if(nMessageBodyLen%MAX_MSG_BODY > 0) {
                        nTotalCount++;
                }
        }

        int nMessagePos = 0;
        int nSendLen = 0;
        int nSendedLen = nMessageBodyLen;

        char szMessage[MAX_MSG_BODY];

        for(nLoop = 1 ; nLoop <= nTotalCount ; nLoop++) {
                memset(szMessage, 0, sizeof(szMessage) );

                if(nSendedLen >= MAX_MSG_BODY) {
                        nSendLen = MAX_MSG_BODY;
                }
                else {
                        nSendLen = nSendedLen;
                }

                memcpy(szMessage, pszMessage+nMessagePos, nSendLen);

                nRet = GO_Delivery(GetGWID(), NULL, DELIVERY_COLLECT_DATA,  0x01, szMessage, nTotalCount, nLoop, GMMP_ENCRYPTION_NOT);

                if(nRet < 0) {
                        return 1;
                }
                nSendedLen -= nSendLen;
                nMessagePos+= nSendedLen;
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

        pthread_t thread_1 = 0;
        pthread_t thread_2 = 0;
        pthread_t thread_3 = 0;
        pthread_create(&thread_1, NULL, SendData, NULL);
        pthread_create(&thread_2, NULL, SendHB, NULL);
        pthread_create(&thread_3, NULL, ReadTCPData, NULL);

        /* Multiple Registration is allowed */
        if (GW_Reg() != 0) {
                GMMP_Printf(GMMP_ERROR_LEVEL_ERROR, GMMP_LOG_MARKET_ERR, "GW_Reg Error\n");
                return -1;
        }

        pthread_join(thread_1, NULL);
        pthread_join(thread_2, NULL);
        pthread_join(thread_3, NULL);
/*
        while(bTimer == true) {
        }
*/
        return 0;
}

