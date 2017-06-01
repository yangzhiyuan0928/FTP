#include "ftpclient.h"
	
int main(int argc, char* argv[]) 
{		
	int sock_data, retcode;
	char buffer[MAXSIZE];
	struct command cmd;	

	if (argc != 3)
	{
		printf("Usage: %s <Server IP> <port>\n",argv[0]);
		exit(0);
	}

	int sock_cli = socket_connect(atoi(argv[2]), argv[1]);
	if(sock_cli < 0) {
		perror("socket_connect");
		exit(1);
	}
	print_reply(read_reply(sock_cli)); //welcome code
	ftpcli_login(sock_cli);  
	
	while (1) 
	{ 
		if ( ftpcli_read_command(buffer, sizeof buffer, &cmd) < 0)
		{
			printf("Invalid command\n");
			continue;	
		}
		if (send(sock_cli, buffer, (int)strlen(buffer), 0) < 0 )
		{
			close(sock_cli);
			exit(1);
		}
		retcode = read_reply(sock_cli);	
		if (retcode == 221) //bye
		{
			print_reply(221);		
			break;
		}		
		if (retcode == 502) 
			printf("%d Invalid command.\n", retcode);
		else //process cmd
		{			
			if ((sock_data = ftpcli_open_conn(sock_cli)) < 0) 
			{
				perror("Error opening socket for data connection");
				exit(1);
			}			
			if (strcmp(cmd.code, "LIST") == 0) 
				ftpcli_list(sock_data, sock_cli);			
			else if (strcmp(cmd.code, "RETR") == 0) 
			{
				if (read_reply(sock_cli) == 550) 
				{
					print_reply(550);  //File unavailable		
					close(sock_data);  
					continue; 
				}
				ftpcli_get(sock_data, cmd.arg);
				print_reply(read_reply(sock_cli)); 
			}
			close(sock_data);
		}
	} 
	close(sock_cli); 
    return 0;  
}

int read_reply(int sockfd)
{
	int retcode = 0;
	if (recv(sockfd, &retcode, sizeof retcode, 0) < 0) 
	{
		perror("client: error reading message from server\n");
		return -1;
	}	
	return ntohl(retcode);
}

void print_reply(int rspcode) 
{
	switch (rspcode)
	{
		case 220:
			printf("220 Welcome, server ready.\n");
			break;
		case 221:
			printf("221 Goodbye!\n");
			break;
		case 226:
			printf("226 Closing data connection. Requested file action successful.\n");
			break;
		case 550:
			printf("550 Requested action not taken. File unavailable.\n");
			break;
	}
}

void ftpcli_login(int sockfd)
{
	struct command cmd;
	memset(&cmd.arg, 0, sizeof cmd.arg);
	memset(&cmd.code, 0, sizeof cmd.code);
	char user[256];
	memset(user, 0, 256);

	printf("Name: ");	
	fflush(stdout); 		
	read_input(user, 256); //read Login Name from terminal 
	strcpy(cmd.code, "USER");
	strcpy(cmd.arg, user);
	ftpcli_send_cmd(sockfd, &cmd);
	
	int wait;  //read request code: 331
	recv(sockfd, &wait, sizeof wait, 0);

	memset(&cmd.arg, 0, sizeof cmd.arg);
	memset(&cmd.code, 0, sizeof cmd.code);
	fflush(stdout);	
	char *pass = getpass("Password: ");	 //get Login password
	strcpy(cmd.code, "PASS");
	strcpy(cmd.arg, pass);
	ftpcli_send_cmd(sockfd, &cmd);
	
	int retcode = read_reply(sockfd);
	switch (retcode) 
	{
		case 430:
			printf("Invalid username/password.\n");
			exit(0);
		case 230:
			printf("Successful login.\n");
			break;
		default:
			perror("error reading message from server");
			exit(1);		
			break;
	}
}

int ftpcli_send_cmd(int sockfd, struct command *cmd)
{
	char buffer[MAXSIZE];
	int rspcode;

	sprintf(buffer, "%s %s", cmd->code, cmd->arg);
	rspcode = send(sockfd, buffer, (int)strlen(buffer), 0);	
	if (rspcode < 0) {
		perror("Error sending command to server");
		return -1;
	}
	
	return 0;
}

int ftpcli_read_command(char* buf, int size, struct command *cmd)
{
	memset(cmd->code, 0, sizeof(cmd->code));
	memset(cmd->arg, 0, sizeof(cmd->arg));
	
	printf("ftp> ");	
	fflush(stdout); 	
	read_input(buf, size); 
	char *arg = NULL;
	arg = strtok (buf," ");
	arg = strtok (NULL, " ");

	if (arg != NULL)
		strncpy(cmd->arg, arg, strlen(arg));
	if (strcmp(buf, "list") == 0) 
		strcpy(cmd->code, "LIST");
	else if (strcmp(buf, "get") == 0)
		strcpy(cmd->code, "RETR");
	else if (strcmp(buf, "quit") == 0) 
		strcpy(cmd->code, "QUIT");	
	else 
		return -1; 

	memset(buf, 0, MAXSIZE);  
	strcpy(buf, cmd->code);

	if (arg != NULL) 
	{
		strcat(buf, " ");
		strncat(buf, cmd->arg, strlen(cmd->arg));
	}
	return 0;
}

int ftpcli_open_conn(int sockfd)
{
	int sock_listen = open_listenfd(CLIENT_PORT_ID);  //Date Server
	int ack = 1;
	if ((send(sockfd, (char*) &ack, sizeof(ack), 0)) < 0) { 
		printf("client: ack write error :%d\n", errno);
		exit(1);
	}	
	int sock_conn = socket_accept(sock_listen);
	if(sock_conn < 0) {
		close(sock_listen);
		return -1;
	}
	close(sock_listen); 
	return sock_conn;
}

int ftpcli_list(int sock_data, int sockfd)
{
	size_t num_recvd;			
	char buf[MAXSIZE];			
	int status = 0;

	if (recv(sockfd, &status, sizeof status, 0) < 0)  //wait for ftp server setup
	{
		perror("client: error reading message from server\n");
		return -1;
	}
	
	memset(buf, 0, sizeof(buf));
	while ((num_recvd = recv(sock_data, buf, MAXSIZE, 0)) > 0) 
	{
		printf("%s", buf);
		memset(buf, 0, sizeof(buf));
	}
	
	if (num_recvd < 0) 
		perror("error");
	
	if (recv(sockfd, &status, sizeof status, 0) < 0) 
	{
		perror("client: error reading message from server\n");
		return -1;
	}
	return 0;
}

int ftpcli_get(int sock_data, char* arg)
{
    char data[MAXSIZE];
    int size;
    FILE* fd = fopen(arg, "w");  //arg --> filename
    while ((size = recv(sock_data, data, MAXSIZE, 0)) > 0) 
		fwrite(data, 1, size, fd); 

    if (size < 0) 
		perror("error\n");

    fclose(fd);
    return 0;
}

