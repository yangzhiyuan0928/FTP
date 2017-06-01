#ifndef __UTIL_H_
#define __UTIL_H_

#include <arpa/inet.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>		
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define DEBUG				1
#define MAXSIZE 			512 
#define LISTENQ             64
#define CLIENT_PORT_ID		30020

struct command 
{
	char arg[255];
	char code[5];
};

int open_listenfd(int port);  //TCP Server
int socket_accept(int listenfd);  //TCP Server
int socket_connect(int port, char *host);  //TCP Client
int recv_data(int sockfd, char* buf, int bufsize);
int send_rspcode(int sockfd, int rspcode);
void read_input(char* buffer, int size);
void trimstr(char *str, int n);

#endif
