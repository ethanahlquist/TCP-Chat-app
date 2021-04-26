#ifndef __MESSAGE_H__
#define __MESSAGE_H__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>


#define HDR_LEN 2

int msg_receive(int socket_fd, char *buf);
int msg_send(int socket_fd, char *sendBuf);

#endif
