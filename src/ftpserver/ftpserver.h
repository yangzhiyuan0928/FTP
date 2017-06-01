#ifndef __FTP_SERVER_H_
#define __FTP_SERVER_H_

#include "../util/util.h"

void ftpser_process(int sockfd);
int ftpser_login(int sockfd);
int ftpser_authen(char*user, char*pass);
int ftpser_recv_cmd(int sockfd, char*cmd, char*arg);
int ftpser_start_data_conn(int sockfd);
int ftpser_list(int sock_data, int sockfd);
void ftpser_retr(int sockfd, int sock_data, char* filename);

#endif
