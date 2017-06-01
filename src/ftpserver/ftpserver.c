#include "ftpserver.h"

int main(int argc, char *argv[])
{
	int listenfd, connfd, port, pid;
	if(argc != 2) {
		printf("Usage: %s <port>\n",argv[0]);
		exit(0);
	}
	port = atoi(argv[1]);
	listenfd = open_listenfd(port);
	if(listenfd < 0) {
		perror("open_listenfd");
		exit(1);
	}
		
	while(1) {  //block IO
		if( (connfd = socket_accept(listenfd)) < 0) {
			close(listenfd);
			exit(1);
		}
		if((pid = fork()) < 0) {
			perror("fork");
			exit(1);
		}
		else if(pid == 0) {  //child process
			close(listenfd);
			ftpser_process(connfd);  //process client request
			close(connfd);
			exit(0);
		}
		else {  //father process
			close(connfd);  
			//wait(0);
		}
	} 
	return 0;
}

void ftpser_process(int sockfd)
{
	int sdfd;  //data socket
	char cmd[5];
	char arg[MAXSIZE];

	send_rspcode(sockfd, 220); //welcome code

	if (ftpser_login(sockfd) == 1)  
		send_rspcode(sockfd, 230);  //authen success
	else 
	{
		send_rspcode(sockfd, 430);	//authen failed
		exit(1);
	}	
	
	while (1) 
	{
		int rspcode = ftpser_recv_cmd(sockfd, cmd, arg);  //recv request from client		
		if ((rspcode < 0) || (rspcode == 221))  //Client: QUIT
			break;
		if (rspcode == 200 ) 
		{
			if ((sdfd = ftpser_start_data_conn(sockfd)) < 0)  //data socket connect
			{
				close(sockfd);
				perror("ftpser_start_data_conn");
				exit(1); 
			}

			if (strcmp(cmd, "LIST") == 0) 
				ftpser_list(sdfd, sockfd);
			
			else if (strcmp(cmd, "RETR") == 0) 
				ftpser_retr(sockfd, sdfd, arg);
			close(sdfd);
		} 
	}
}

int ftpser_login(int sockfd)
{	
	char buf[MAXSIZE];
	char user[MAXSIZE];
	char pass[MAXSIZE];	
	memset(user, 0, MAXSIZE);
	memset(pass, 0, MAXSIZE);
	memset(buf, 0, MAXSIZE);
	
	if ( (recv_data(sockfd, buf, sizeof(buf)) ) == -1)  //blocking Recv: User Name
	{
		perror("recv error\n"); 
		exit(1);
	}	

	int i = 5;  
	int n = 0;
	while (buf[i] != 0) //buf[0-4]="USER"
		user[n++] = buf[i++];
	
	send_rspcode(sockfd, 331);  //Notify code				
	
	memset(buf, 0, MAXSIZE);
	if ( (recv_data(sockfd, buf, sizeof(buf)) ) == -1) 
	{
		perror("recv error\n"); 
		exit(1);
	}
	
	i = 5;
	n = 0;
	while (buf[i] != 0) // buf[0 - 4] = "PASS"
		pass[n++] = buf[i++];
	
	return (ftpser_authen(user, pass)); 
}

int ftpser_authen(char*user, char*pass)  //authen user login & passwd
{
	char username[MAXSIZE];
	char password[MAXSIZE];
	char *pch;
	char buf[MAXSIZE];
	char *line = NULL;
	size_t num_read;									
	size_t len = 0;
	FILE* fd;
	int auth = 0;
	
	fd = fopen(".auth", "r");  
	if (fd == NULL) 
	{
		perror("file not found");
		return -1;
	}	

	/***
	 * getline(): http://man.he.net/?topic=getline&section=all
	 **/
	while ((num_read = getline(&line, &len, fd)) != -1)  
	{
		memset(buf, 0, MAXSIZE);
		strcpy(buf, line);
		
		pch = strtok (buf," ");
		strcpy(username, pch);

		if (pch != NULL)
		{
			pch = strtok (NULL, " ");
			strcpy(password, pch);
		}

		trimstr(password, (int)strlen(password));

		if ((strcmp(user,username)==0) && (strcmp(pass,password)==0)) 
		{
			auth = 1; 
			break;
		}		
	}
	free(line);	
	fclose(fd);	
	return auth;
}

void ftpser_retr(int sockfd, int sdfd, char* filename)
{
	FILE *fd = NULL;
	char data[MAXSIZE];
	size_t siz;
	fd = fopen(filename, "r");
	if(fd == NULL) {
		perror("fopen");
		send_rspcode(sockfd, 550);  //550 Requested action not taken
	}
	else {	
		send_rspcode(sockfd, 150); //okay (150 File status okay)
		do {
			siz = fread(data, 1, MAXSIZE, fd); 
			if (siz < 0) 
				printf("error in fread()\n");

			if (send(sdfd, data, siz, 0) < 0)
				perror("error sending file\n");
		} while (siz > 0);										
		send_rspcode(sockfd, 226); //closing conn, file transfer successful
		fclose(fd);
	}
}

int ftpser_list(int sdfd, int sockfd)
{
	char data[MAXSIZE];
	size_t siz;									
	FILE* fd;

	int rs = system("ls -l | tail -n+2 > tmp.txt"); //system call command
	if ( rs < 0) {
		perror("system");
		return -1;
	}
	
	fd = fopen("tmp.txt", "r");	
	if (NULL) {
		perror("fopen");
		return -1; 
	}
	
	fseek(fd, SEEK_SET, 0);
	send_rspcode(sockfd, 1); 
	memset(data, 0, MAXSIZE);

	while ((siz = fread(data, 1, MAXSIZE, fd)) > 0) 
	{
		if (send(sdfd, data, siz, 0) < 0) 
			perror("send");	
		memset(data, 0, MAXSIZE);
	}

	fclose(fd);
	send_rspcode(sockfd, 226);	 //file request success

	return 0;	
}

int ftpser_start_data_conn(int sockfd)
{
	char buf[1024];	
	int wait, sdfd;

	if (recv(sockfd, &wait, sizeof wait, 0) < 0 ) //接收wait?
	{
		perror("blocking recv");
		return -1;
	}
	
	struct sockaddr_in client_addr;
	socklen_t len = sizeof client_addr;
	getpeername(sockfd, (struct sockaddr*)&client_addr, &len);  //get connected client IP address and port
	inet_ntop(AF_INET, &client_addr.sin_addr, buf, sizeof(buf));  //IP address: numeric to presentation

	if ((sdfd = socket_connect(CLIENT_PORT_ID, buf)) < 0)
		return -1;

	return sdfd;		
}


int ftpser_recv_cmd(int sockfd, char*cmd, char*arg)
{	
	int rspcode = 200;
	char buffer[MAXSIZE];
	
	memset(buffer, 0, MAXSIZE);
	memset(cmd, 0, 5);
	memset(arg, 0, MAXSIZE);
		
	if ((recv_data(sockfd, buffer, sizeof(buffer)) ) == -1) 
	{
		perror("recv error\n"); 
		return -1;
	}
	
	strncpy(cmd, buffer, 4);
	strcpy(arg, buffer+5);
	
	if (strcmp(cmd, "QUIT")==0) 
		rspcode = 221;
	else if ((strcmp(cmd, "USER") == 0) || (strcmp(cmd, "PASS") == 0) || (strcmp(cmd, "LIST") == 0) || (strcmp(cmd, "RETR") == 0))
		rspcode = 200;
	else 
		rspcode = 502; //Invalid cmd

	send_rspcode(sockfd, rspcode);	
	return rspcode;
}

