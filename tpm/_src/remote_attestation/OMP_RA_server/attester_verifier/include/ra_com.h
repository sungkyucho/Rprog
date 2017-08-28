#ifndef _RA_COM_H_
#define _RA_COM_H_

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h> 
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <time.h>

#include <tss/tddl_error.h>
#include <tss/tcs_error.h>
#include <tss/tspi.h>

#define NONCE_LEN 20
#define PCR_LIST_LEN 3
#define MAC_LEN 17

#define BACKLOG 10				// the max num of connection //
#define MAXDATASIZE 65536		// the max num of transferred bytes//
#define MaxIPNum 100  // the max num of IPs that need to compare the PCR

//Client Exit code

#define ERROR_BAD_PARAMFORMAT			1
#define ERROR_NO_IPSETFILE				2

#define ERROR_READ_SERVERPUB			3
#define ERROR_TOO_MANY_SERVER			100

#define SUCCESS EXIT_SUCCESS 
#define FAILED EXIT_FAILURE 


#define STATE_REQ_CONNECT       0x0001
#define STATE_RA_READY		    0x0002
#define STATE_REQ_PCR			0x0004
#define STATE_RA_DONE           0x0008
//Communication Type
#define COMM_REQ_CONNECT		0x00000001
#define COMM_RSP_ALLOW			0x00000002
#define COMM_RSP_DENY			0x00000003
#define RA_REQ_PCR				0x00000004
#define RA_RSP_PCR				0x00000005
#define RA_RSP_ERROR			0x00000006
#define RA_REQ_EVENTLOG			0x00000007
#define RA_RSP_EVENTLOG			0x00000008
#define RA_RSP_RESULT			0x00000009
#define COMM_RSP_FINAL			0x0000000A
#define IMR_REQ_EVENTLOG		0x0000000B
#define IMR_RSP_EVENTLOG		0x0000000C
#define IMR_RSP_NORECORD		0x0000000D
#define IMR_RSP_ERROR			0x0000000E
#define COMM_RSP_EVENTLOG     0x0000000F
#define RA_RSP_SESSIONKEY1		0x00000011  //SESSIONKEY BETWEEN C AND S
#define RA_RSP_SESSIONKEY2		0x00000012  //SESSIONKEY BETWEEN C AND IMRS
#define RA_REQ_PLATFORMSTATE	0x00000013
#define RA_RSP_PCRandEVENTLOG	0x00000014
#define RA_REQ_INTEGRITYREF		0x00000015  //C ASK TO IMRS FOR THE INTEGRITY REFERENCE VALUE 
#define RA_RSP_INTEGRITYREF		0x00000016
#define RA_RSP_SESSIONKEY1RESULT		0x00000017
#define RA_RSP_SESSIONKEY2RESULT		0x00000018


#define RA_RSP_ERROR_TCSD			0x00000019
#define RA_RSP_ERROR_SSLSIGN		0x0000001A
#define RA_RSP_ERROR_DECRYTION 	0x0000001B

#define RA_RSP_ERROR_RECEIVEDATA	0x0000001C
#define RA_RSP_ERROR_READDATA		0x0000001D
#define RA_RSP_ERROR_QUOTE 		0x0000001E

#define MAX_THREADS 1024

typedef	struct 
{
	char index[30];
	char filename[30];
}	eventlogIndex;

int attestant_req(char *ip_addr, unsigned char *port, char *ca_addr, char * interface, const char *);

void print_hex( char *buf, UINT32 len );
void print_hex_block( char *buf, UINT32 len, const char * );
void print_result_block(int dbg_lv, const char *msg);
void print_state_block(int dbg_lv, const char *msg);
int is_valid_ip(char *ip_str);
char *get_mac_addr(char *iface, char *macaddr);
int hexstr_to_hex(char *hexstring, unsigned char *hexbytes, size_t len);
int hex_to_hexstr(char *stringbuf, unsigned char *hexbytes, int len);
#endif
