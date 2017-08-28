#ifndef	_TP_RA_TYPE_H_
#define	_TP_RA_TYPE_H_

/* refer "GMMP_lib/Operation/Struct_Common.h" */
#define	LEN_GW_ID	16
#define	LEN_DEVICE_ID	16
#define	LEN_AUTH_KEY	16
#define	LEN_AUTH_ID	16

/* refer "GMMP_lib/Define/Define.h" */
#define	MAX_MSG_BODY	2048
#define	MAX_SMS_MSG_BODY	38

/* refer "attester_verifier/include/ra_com.h" */
#define	NONCE_LEN	20
#define	PCR_LIST_LEN	3

/* refer /usr/include/tss/platform.h
 * BYTE is defined here
 * -- this code could be used with trousers
 */
#ifndef	TSS_PLATFORM_H
#include <stdint.h>
typedef uint8_t BYTE;
#endif // TSS_PLATFORM_H

/* CHECK - source of app's information
 * domain / serviceName / sclId - should be read from config file?
 */

/* bucket for ID/Nonce/MeasurementList */
typedef struct {
	char gw_id[LEN_GW_ID + 2];
	char dev_id[LEN_DEVICE_ID + 2];
	BYTE nonce[NONCE_LEN];
	BYTE pcr_select[PCR_LIST_LEN];
	BYTE padding; /* for alignment */
} ra_request;

/* TIME format in HTML: YYYY-MM-DDThh:mm:ssTZD
 * T or space - a separator (required if time is also specified)
 * TZD - assume max length is 4-byte
 * --> 19 + TZD + EOS < 24-byte
 */
#define	TIME_STR_LEN	24

#include <time.h>

/* Status of OMP: PROGRESS/ACK/NACK/TIMEOUT/SUCCESS/FAIL
 * [BEGIN] -- PROGRESS|NACK -- ACK|TIMEOUT -- SUCCESS|FAIL
 */
typedef enum {
	/* waiting on PROGRESS/ACK */
	TP_RA_NADA = -1, /* uninitialized */
	TP_RA_SUCCESS = 0, /* SUCCESS */
	TP_RA_PROGRESS = 1, /* OMP accept */
	TP_RA_NACK = 2, /* fail to send command to client */
	TP_RA_ACK = 3, /* ACK from client */
	TP_RA_TIMEOUT = 4, /* expire without ACK from client */
	TP_RA_FAIL = 5
} tp_resp_code;

/* bucket for ID/Time/OK/Composite */
typedef struct {
	char gw_id[LEN_GW_ID + 2];
	char dev_id[LEN_DEVICE_ID + 2];
	time_t mtime; /* lastModifiedTime */
	tp_resp_code decision;
	int bodylen;
	BYTE msgbody[MAX_MSG_BODY];
} ra_response;

#endif // _TP_RA_TYPE_H_
