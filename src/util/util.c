#include "util.h"

int open_listenfd(int port)
{
	int listenfd, optval = 1;	
	struct sockaddr_in serveraddr;
	
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)   //Create a socket descriptor
   		return -1;
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, // listenfd reusable
		(const void *)&optval , sizeof(int)) < 0)
    	return -1;
	
    /* Listenfd will be an endpoint for all requests to port
       on any IP address for this host */
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET; 
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    serveraddr.sin_port = htons((unsigned short)port); 
    if (bind(listenfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0) {
		perror("server bind");
	    return -1;
    }

    /* Make it a listening socket ready to accept connection requests */
    if (listen(listenfd, LISTENQ) < 0)
    {
    	perror("listen overflow");
	    return -1;
    }

    return listenfd;	
}

int socket_accept(int listenfd)
{
	int connfd;

	struct sockaddr_in clientaddr;
	socklen_t inlen = 1;
	memset(&clientaddr, 0, sizeof(struct sockaddr_in));
	connfd = accept(listenfd, (struct sockaddr *)&clientaddr, &inlen);
	
	if (connfd < 0)
	{
		perror("accept() error"); 
		return -1; 
	}
	return connfd;	
}

int socket_connect(int port, char* host)
{
	int sockfd;  					
	struct sockaddr_in dest_addr;

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{ 
        perror("socket_connect");
        return -1;
    }

	memset(&dest_addr, 0, sizeof(dest_addr));
	dest_addr.sin_family = AF_INET;  //IPV4
	dest_addr.sin_port = htons(port);
	dest_addr.sin_addr.s_addr = inet_addr(host);

	if(connect(sockfd, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0 )
	{
        perror("error connecting to server");
		return -1;
    }    
	return sockfd;
}

int recv_data(int sockfd, char* buf, int bufsize)
{
	size_t num_bytes;
	memset(buf, 0, bufsize);

	num_bytes = recv(sockfd, buf, bufsize, 0);
	if (num_bytes < 0) 
		return -1;

	return num_bytes;
}

int send_rspcode(int sockfd, int rspcode)
{
	int conv = htonl(rspcode);
	if (send(sockfd, &conv, sizeof conv, 0) < 0 ) 
	{
		perror("error sending...\n");
		return -1;
	}
	return 0;
}

void read_input(char* buffer, int size)
{
	char *nl = NULL;
	memset(buffer, 0, size);

	if (fgets(buffer, size, stdin) != NULL ) 
	{
		nl = strchr(buffer, '\n');
		if (nl) 
			*nl = '\0'; 
	}
}

void trimstr(char *str, int n)
{
	int i;
	for (i = 0; i < n; i++) 
	{
		if (isspace(str[i])) str[i] = 0;
		if (str[i] == '\n') str[i] = 0;
	}
}

