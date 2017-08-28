#include <unistd.h>
#include <sys/types.h>
#include <stdint.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>   //ifreq

#include "ra_com.h" 
#include "dbg_macros.h"

#define DELIM "."
int dbg_lv = DINFO;

void
print_hex( char *buf, UINT32 len)
{
	UINT32 i = 0, j;

	while (i < len) {
		for (j=0; (j < 16) && (i < len); j++, i++)
			printf("%02x ", buf[i] & 0xff);
		printf("\n");
	}
}
void
print_hex_block( char *buf, UINT32 len, const char *msg)
{
	UINT32 i = 0, j;

	printf("****** %s is ******\n", msg);
	while (i < len) {
		for (j=0; (j < 16) && (i < len); j++, i++)
			printf("%02x ", buf[i] & 0xff);
		printf("\n");
	}
	printf("****** %s end ******\n\n", msg);
}

void print_result_block(int dbg_lv, const char *msg){
	dbg_printf(dbg_lv, "+++++++++++++++++++++++++++++++++++++++++++++++\n");
	dbg_printf(dbg_lv, "+++++  %s  +++++\n", msg);
	dbg_printf(dbg_lv, "+++++++++++++++++++++++++++++++++++++++++++++++\n\n");
}

void print_state_block(int dbg_lv, const char *msg){
	dbg_printf(dbg_lv, "=========================================================\n");
	dbg_printf(dbg_lv, "============  [ %s ]  ===========\n", msg);
	dbg_printf(dbg_lv, "=========================================================\n\n");
}

static int valid_digit(char *ip_str)
{
    while (*ip_str) {
        if (*ip_str >= '0' && *ip_str <= '9')
            ++ip_str;
        else
            return 0;
    }
    return 1;
}
 
int is_valid_ip(char *ip_str)
{
    int num, dots = 0;
    char *ptr;
 
    if (ip_str == NULL)
        return 0;
 
    ptr = strtok(ip_str, DELIM);
 
    if (ptr == NULL)
        return 0;
 
    while (ptr) {
        if (!valid_digit(ptr))
            return 0;
 
        num = atoi(ptr);
 
        if (num >= 0 && num <= 255) {
            ptr = strtok(NULL, DELIM);
            if (ptr != NULL)
                ++dots;
        } else
            return 0;
    }
 
    if (dots != 3)
        return 0;
    return 1;
}

char *get_mac_addr(char *iface, char *macaddr)
{
	int fd;
	struct ifreq ifr;
	unsigned char *mac = NULL;

	memset(&ifr, 0, sizeof(ifr));

	fd = socket(AF_INET, SOCK_DGRAM, 0);

	ifr.ifr_addr.sa_family = AF_INET;
	strncpy(ifr.ifr_name , iface , IFNAMSIZ-1);

	if (0 == ioctl(fd, SIOCGIFHWADDR, &ifr)) {
		mac = (unsigned char *)ifr.ifr_hwaddr.sa_data;

		//display mac address
		sprintf(macaddr, "%02X:%02X:%02X:%02X:%02X:%02X", 
			mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	}
//	printf("macaddr:%s\n", macaddr);
	close(fd);
	return macaddr;
}

int hexstr_to_hex(char *hexstring, unsigned char *hexbytes, size_t len){
	char *pos = hexstring;
	size_t count;

	if(len <= 0) return 0;

	for(count = 0; count < len; count++) {
		sscanf(pos, "%2hhx", &hexbytes[count]);
		pos += 2*sizeof(char);
	}
	return 1;
}

int hex_to_hexstr(char *stringbuf, unsigned char *hexbytes, int len){
	int i;
	char* buf2 = stringbuf;

	if(len <= 0) return 0;

	for (i = 0; i < len; i++) {
		buf2 += sprintf(buf2, "%02x", hexbytes[i]);
	}
	return 1;
}
