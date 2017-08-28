/* Wrapper for calling RA functions */

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "remote_attestation.h"
#include "dbg_macros.h"
#include "tpm_util.h"

#include "ralib.h"

int dbg_lv = DINFO;
/* Utilities - Begin */
void print_hex(char *hexstr, int hlen)
{
	int i;
	int bucket = 20;

	if (hexstr == NULL) {
		printf("<%s> Null String\n", __func__);
		return;
	}

	printf("<%s> HexStr<%p> Len<%d>\n", __func__, hexstr, hlen);

	for (i = 0; i < hlen; i++) {
		printf("%02hhx", hexstr[i]);
		if (bucket - (i % bucket) == 1) {
			printf("\n");
		} else {
			printf(" ");
		}
	}
	printf("\n");
}


void print_hexstr(unsigned char *hexstr, int hlen)
{
	int i;
	int bucket = 60;

	if (hexstr == NULL) {
		printf("<%s> Null String\n", __func__);
		return;
	}

	printf("<%s> HexStr<%p> Len<%d>\n", __func__, hexstr, hlen);

	for (i = 0; i < hlen; i++) {
		if (!isascii(hexstr[i])) {
			break;
		}
		printf("%c", (char)hexstr[i]);
		if (bucket - (i % bucket) == 1) {
			printf("\n");
		}
	}
	printf("\n");
}
/* Utilities - End */


/* config part */
// ex) char *cfg_pca_addr = "192.168.122.13:8888";
static char *cfg_pca_addr = NULL;
// NOTE - directories should be exist (configurable by App)
// ex) char *cfg_x509_path = "/home/root/SandBox/GMMP_RAc/Debug/x509.pem";
static char *cfg_x509_path = NULL;
// NOTE - User-defined Control: User_defined_Control_Min (mgmtUser01) ~ +31
// ex) int cfg_udc = User_defined_Control_Min + 4; // mgmtUser05
static int cfg_udc = 0; // default: all allow


/**
 * @brief initialize essential data for RA
 * @param pca_addr IP address with port of PCA
 * @param x509_path path for storing cert of signed AIK
 * @param udc_no number of mgmtUser (not control-type)
 *        set 0 for allowing all control-type for RA
 * @return 0 on success, otherwise -1
 */
int init_RA(char *pca_addr, char *x509_path, int udc_no)
{
	if ((NULL == pca_addr) || (NULL == x509_path)) {
		printf("<%s> invalid pca_addr<%p> or x509_path<%p>\n", \
				__func__, pca_addr, x509_path);
		return (-1);
	}

	if (cfg_pca_addr) free(cfg_pca_addr);
	if (cfg_x509_path) free(cfg_x509_path);

	cfg_pca_addr = strdup(pca_addr);
	cfg_x509_path = strdup(x509_path);
	if (udc_no > 0) {
		cfg_udc = User_defined_Control_Min + udc_no - 1;
	} else {
		cfg_udc = 0; // all allow
	}

#if 1
	if(pcr_reset_extend(23, "/usr/local/lib/libra.la")){
		printf("PCR[23] reset and extend Success\n");
	}
	else{
		printf("PCR[23] reset and extend Failed\n");
	}
#endif
	return 0;
}


/**
 * @brief free essential data for RA
 */
void stop_RA(void)
{
	if (cfg_pca_addr) free(cfg_pca_addr);
	if (cfg_x509_path) free(cfg_x509_path);

	cfg_pca_addr = NULL;
	cfg_x509_path = NULL;
	cfg_udc = 0; // all allow
}


/**
 * @brief process GMMP message
 * @param pstGMMPHeader pointer for GMMP Header
 * @param pstReqHdr pointer for Request Header
 * @return RA_OK for processed message, RA_BYPASS for non-RA message,
 *         RA_BAD_CONFIG for uninitialized state,
 *         RA_BAD_PACKET for invalid argument(s) as pointer
 */
int GMMP_Do_RA(GMMPHeader *pstGMMPHeader, stControlReqHdr *pstReqHdr)
{
	U8 cControlType;

	if ((NULL == cfg_pca_addr) || (NULL == cfg_x509_path)) {
		printf("<%s> config is not initialized <%p/%p>\n", \
				__func__, cfg_pca_addr, cfg_x509_path);
		return RA_BAD_CONFIG;
	}

	if ((NULL == pstGMMPHeader) || (NULL == pstReqHdr)) {
		printf("<%s> invalid GMMP packet <G:%p/R:%p>\n", \
				__func__, pstGMMPHeader, pstReqHdr);
		return RA_BAD_PACKET;
	}

	cControlType = pstReqHdr->ucControlType;

	printf("<%s> Control-Type <0x%hhx>\n", __func__, cControlType); // debug

	if (cfg_udc && (cfg_udc != cControlType)) {
		printf("<%s> Not RA message <0x%hhx>\n", __func__, cControlType); // debug
		return RA_BYPASS;
	}

	/* Control_Req = GMMPHeader(header) + stControlReqHdr(body)
	 * stControlReqHdr = usDomainCode/usGWID/usDeviceID/ucControlType
	 *                   + usMessageBody[MAX_MSG_BODY]
	 * --> length of message body = total_length - (10+16+16+1)
	 */

	/* these routine is already done in [GMMP_Read]
	 * - No need to check size for handling error
	 * - Q. why don't they save this info into headers
	 */
	ConvertShort cvtShort;

	cvtShort.sU8 = 0;

	memcpy(cvtShort.usShort, pstGMMPHeader->usMessageLength, sizeof(pstGMMPHeader->usMessageLength));
	cvtShort.sU8 = btols(cvtShort.sU8);

	int blen = (int)(cvtShort.sU8 - sizeof(GMMPHeader));

	printf("Total<%hd> - Common Header<%ld> = Message Header+Body<%d>\n", \
			cvtShort.sU8, sizeof(GMMPHeader), blen); // debug
	
	int mhlen = (int)(sizeof(stControlReqHdr) - MAX_MSG_BODY);
	int mblen = blen - mhlen;

	printf("Message Header <%d> + Body <%d>\n", mhlen, mblen);

	/* NOTE - Message Body is encoded as base64 only for SMS
	 *        so, it is an array of char's including non-ASCII
	 */
	print_hex((char *)pstReqHdr->usMessageBody, mblen); // debug
	print_hexstr(pstReqHdr->usMessageBody, mblen); // debug

	char cResult = 0x00; // result code
	// GMMP Spec V2.1.5h 20150921.pdf - 6.3 Result Code
	// 0x00: ok, 0x08: status_not_acceptable (reuse legacy code)
	// 0x20 ~ : defined...
	// Therefore, let's use 0x10 ~ 0x1F (-_-)/~

	// processing data in [pstReqHdr->usMessageBody]
	// store result into [pstReqHdr->usMessageBody] or other buffer
	BYTE nonce[NONCE_LEN];
	BYTE pcr_list[PCR_LIST_LEN];
	BYTE *rstbuf = NULL;
	int rstlen = 0;
	int rcode;

	memcpy(nonce, pstReqHdr->usMessageBody, NONCE_LEN);
	memcpy(pcr_list, pstReqHdr->usMessageBody + NONCE_LEN, PCR_LIST_LEN);

	rcode = remote_attestation(nonce, NONCE_LEN, &rstbuf, &rstlen, cfg_pca_addr, pcr_list, cfg_x509_path);
	if (TSS_SUCCESS != rcode) {
		printf("error in remote_attestation <0x%x>\n", rcode);
		rstbuf = pstReqHdr->usMessageBody;
		rstlen = mblen;
		cResult = 0x11; // random selection (-_-)
	}

	printf("<< result >> rstbuf<%p> rstlen<%d>\n", rstbuf, rstlen); // debug

	// NOTE: send result with [GO_Notifi]
	GO_Notifi((char *)pstReqHdr->usGWID, (char *)pstReqHdr->usDeviceID, (char)pstReqHdr->ucControlType, cResult, (char *)rstbuf, rstlen);

	return RA_OK;
}
