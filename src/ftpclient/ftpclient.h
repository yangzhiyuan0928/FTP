#ifndef __FTP_CLIENT_H_
#define __FTP_CLIENT_H_

#include "../util/util.h"

int read_reply(int sockfd);
void print_reply(int rspcode);
void ftpcli_login(int sockfd);
int ftpcli_send_cmd(int sockfd, struct command *cmd);
int ftpcli_read_command(char* buf, int size, struct command *cmd);
int ftpcli_open_conn(int sockfd);
int ftpcli_list(int sock_data, int sockfd);
int ftpcli_get(int sock_data, char* arg);

#endif
