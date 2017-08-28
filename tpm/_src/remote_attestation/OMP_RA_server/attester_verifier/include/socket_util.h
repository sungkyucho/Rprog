#ifndef _SOCKET_CON_H_
#define _SOCKET_CON_H_

int trigger_attestant(char *ip, char *port, char *);
int create_tcp_listen_socket(int port);
int create_tcp_connect_socket(char *ip, int port, struct sockaddr_in *);
int send_msg(int socketfd, int dwCommTYPE, int dwTransLength, char * rgbBuffer);
int recv_msg(int socketfd, int* dwCommTYPE, int * dwTransLength,char * * prgbBuffer);

#endif
