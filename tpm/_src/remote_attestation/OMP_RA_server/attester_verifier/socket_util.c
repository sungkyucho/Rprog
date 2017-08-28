#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h> 
#include <fcntl.h>
#include <sys/ioctl.h>

#include "ra_com.h"
#include "socket_util.h"
#include "dbg_macros.h"

int trigger_attestant(char *ip, char *port, char *s_port){
    struct sockaddr_in client;
    int socket_desc;
	char *message=NULL; 

	socket_desc = create_tcp_connect_socket(ip, atoi(port), &client);
	if(socket_desc == -1){
		dbg_printf(DERROR, "create_tcp_connect_socket failed\n");
		return -1;
	}

//	message = malloc( (sizeof(uint32_t) * 2) + strlen(ip) + strlen(s_port));
	message = malloc(strlen(s_port));
//	*((uint32_t *)message) = strlen(ip);
//	*((uint32_t *)message + 1) = strlen(s_port);

//	memcpy(message + 2*sizeof(uint32_t), ip, strlen(ip));
	memcpy(message, s_port, strlen(s_port));

	if( send(socket_desc, message, strlen(s_port), 0) < 0)
	{
		dbg_printf(DERROR, "send to [%s] trigger failed:%s\n", inet_ntoa(client.sin_addr), strerror(errno));
		free(message);
		close(socket_desc);
		return -1;
	}

	dbg_printf(DINFO, "send to [%s] trigger success\n", inet_ntoa(client.sin_addr));
	free(message);
	close(socket_desc);
	return 0;
}

int create_tcp_listen_socket(int port){
    int socket_desc;
	int opt=1;
	struct timeval timeout;      
	timeout.tv_sec = 20;
	timeout.tv_usec = 0;

    struct sockaddr_in server;

	if(port < 0 || port > 65535){
        dbg_printf(DERROR, "invalid port:%d", port);
		return -1;
	}

    socket_desc = socket(PF_INET, SOCK_STREAM, 0);
    if (socket_desc == -1)
    {
        dbg_printf(DERROR, "Could not create socket:%s", strerror(errno));
		return -1;
    }
    if( setsockopt(socket_desc, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0 )
    {
        dbg_printf(DERROR, "setsockopt :%s", strerror(errno));
		return -1;
	}
	if (setsockopt (socket_desc, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout,
				sizeof(timeout)) < 0){
        dbg_printf(DERROR, "setsockopt receive timeout failed:%s", strerror(errno));
		return -1;
	}
	if (setsockopt (socket_desc, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout,
				sizeof(timeout)) < 0){
        dbg_printf(DERROR, "setsockopt send timeout failed:%s", strerror(errno));
		return -1;
	}

    //Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(port);
     
    if( bind(socket_desc, (struct sockaddr *)&server, sizeof(server)) < 0) {
        dbg_printf(DERROR, "bind failed:%s", strerror(errno));
        return -1;
    }

    if( listen(socket_desc, 100) < 0){
		dbg_printf(DERROR,"Listen failed:%s", strerror(errno));	
		return -1;
	} 
	return socket_desc;
}


int create_tcp_connect_socket(char *ip, int port, struct sockaddr_in *client){
//    struct sockaddr_in client;
	int client_fd;
	char *check_ip = NULL;
	struct timeval timeout;      
	timeout.tv_sec = 10;
	timeout.tv_usec = 0;

	/////////////
	int res; 
	long arg; 
	fd_set myset; 
	struct timeval tv; 
	int valopt; 
	socklen_t lon; 
	////////////////
	
	check_ip = strdup(ip);
	if( 0 == is_valid_ip(check_ip)){
        dbg_printf(DERROR, "invalid ip:%s", check_ip);
		free(check_ip);
		return -1;
	}
	free(check_ip);

	if(port < 0 || port > 65535){
        dbg_printf(DERROR, "invalid port:%d", port);
		return -1;
	}

    client_fd= socket(AF_INET , SOCK_STREAM, 0);
    if (client_fd== -1) {
        dbg_printf(DERROR, "Could not create socket");
		return -1;
    }
	if (setsockopt (client_fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout,
				sizeof(timeout)) < 0){
        dbg_printf(DERROR, "setsockopt receive timeout failed:%s", strerror(errno));
		goto close_return;
	}
	if (setsockopt (client_fd, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout,
				sizeof(timeout)) < 0){
        dbg_printf(DERROR, "setsockopt send timeout failed:%s", strerror(errno));
		goto close_return;
	}

	(*client).sin_addr.s_addr = inet_addr(ip);
    (*client).sin_family = AF_INET;
    (*client).sin_port = htons(port);
	//////////////////////////////////

	// Set non-blocking 
	if( (arg = fcntl(client_fd, F_GETFL, NULL)) < 0) { 
		dbg_printf(DERROR, "Error fcntl(..., F_GETFL) (%s)\n", strerror(errno)); 
		goto close_return;
	} 
	arg |= O_NONBLOCK; 
	if( fcntl(client_fd, F_SETFL, arg) < 0) { 
		dbg_printf(DERROR, "Error fcntl(..., F_SETFL) (%s)\n", strerror(errno)); 
		goto close_return;
	} 
	// Trying to connect with timeout 
	res = connect(client_fd, (struct sockaddr *)client, sizeof(*client)); 
	if (res >= 0)  
		goto block_mode;

	if (errno != EINPROGRESS) { 
		dbg_printf(DERROR, "Error connecting %d - %s\n", errno, strerror(errno)); 
		goto close_return;
	}

	dbg_printf(DINFO, "EINPROGRESS in connect() - selecting\n"); 
	while(1){ 
		tv.tv_sec = 5; 
		tv.tv_usec = 0; 
		FD_ZERO(&myset); 
		FD_SET(client_fd, &myset); 
		res = select(client_fd+1, NULL, &myset, NULL, &tv); 
		if (res < 0 && errno != EINTR) { 
			dbg_printf(DINFO, "Error connecting %d - %s\n", errno, strerror(errno)); 
			goto close_return;
		} 
		else if (res > 0) { 
			// Socket selected for write 
			lon = sizeof(int); 
			if (getsockopt(client_fd, SOL_SOCKET, SO_ERROR, (void*)(&valopt), &lon) < 0) { 
				dbg_printf(DINFO, "Error in getsockopt() %d - %s\n", errno, strerror(errno)); 
				goto close_return;
			} 
			// Check the value returned... 
			if (valopt) { 
				dbg_printf(DINFO, "Error in delayed connection() %d - %s\n", valopt, strerror(valopt) ); 
				goto close_return;
			} 
			break; 
		} 
		else { 
			dbg_printf(DINFO, "Timeout in select() - Cancelling!\n"); 
			goto close_return;
		} 
	} 

block_mode:
	// Set to blocking mode again... 
	if( (arg = fcntl(client_fd, F_GETFL, NULL)) < 0) { 
		dbg_printf(DERROR, "Error fcntl(..., F_GETFL) (%s)\n", strerror(errno)); 
		goto close_return;
	} 
	arg &= (~O_NONBLOCK); 
	if( fcntl(client_fd, F_SETFL, arg) < 0) { 
		dbg_printf(DERROR, "Error fcntl(..., F_SETFL) (%s)\n", strerror(errno)); 
		goto close_return;
	} 


	///////////////////////////
	/*
	dbg_printf(DINFO, "connect to attester...\n");
    if (connect(client_fd, (struct sockaddr *)&client, sizeof(client)) < 0) {
		dbg_printf(DERROR, "Connect failed\n");
		close(client_fd);	
		return -1;
	}
	dbg_printf(DINFO,"Connect Success\n"); 
	*/
//	memcpy(client_in, &client, sizeof(struct sockaddr_in));
	return client_fd;

close_return:
	close(client_fd);	
	return -1;
}

int send_msg(int socketfd, int dwCommTYPE, int dwTransLength, char * rgbBuffer)
{
	int dwSendbytes,dwData_length;
	char str_send_buf[MAXDATASIZE];

	//compute the whole length of the data and the head
	dwData_length = dwTransLength+8;

	//write the head to the data
	memset(str_send_buf,0,MAXDATASIZE);

	memcpy(str_send_buf,&dwCommTYPE,4);
	memcpy(str_send_buf+4,&dwData_length,4);
	if((dwTransLength!=0)&&(rgbBuffer!=NULL))
		memcpy(str_send_buf+8,rgbBuffer,dwTransLength);

	//send the message
	if((dwSendbytes=send(socketfd, str_send_buf, dwData_length, 0)) ==-1) 
	{
		dbg_printf(DERROR, "Send error!\n");
		return -1;
	}

	//send the message succesfully
	if(dwSendbytes == dwData_length)
	{
		return 0;
	}
	else 
	{
		return -1;
	}
} 

int recv_msg(int socketfd, int* dwCommTYPE, int * dwTransLength, char ** prgbBuffer)
{
	char * str_rcv_buf;
	int dw_rec_length;//,dwdata_length,type;
	uint32_t  type, dwdata_length;


	str_rcv_buf=(char *)malloc(MAXDATASIZE);
	if(str_rcv_buf==NULL) {
		dbg_printf(DERROR, "malloc memory error!\n");
		return -1;
	}
	memset(str_rcv_buf,0,MAXDATASIZE);

	if((dw_rec_length=recv(socketfd, str_rcv_buf, MAXDATASIZE, 0)) ==-1) 
	{
		dbg_printf(DERROR, "Receive error!\n");
		free(str_rcv_buf);
		return -2;
	}
	
	//copy the sourceIP & translength & data from the buf
	memcpy(&type,str_rcv_buf, sizeof(uint32_t));
	memcpy(&dwdata_length, str_rcv_buf+sizeof(uint32_t), sizeof(uint32_t));

	if(dwdata_length!=dw_rec_length)
	{
		dbg_printf(DERROR, "Received Data length error!\n");
		free(str_rcv_buf);
		return -3;		
	}

	(*dwCommTYPE)=type;
	(*dwTransLength)=dwdata_length-(sizeof(uint32_t)*2);
	if((*dwTransLength)>0)
		(*prgbBuffer) = str_rcv_buf+(sizeof(uint32_t)*2);
	else{
		(*prgbBuffer) = NULL;
		free(str_rcv_buf);
	}
	return 0;	
}
