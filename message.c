#include "message.h"
#include "packets.h"

#define CONNECTION_CLOSED 0

int msg_receive(int socket_fd, char *buf)
{
    int messageLen;
    int bytesReceived;

    if((messageLen = recv(socket_fd, buf, HDR_LEN, MSG_WAITALL)) < 0)
    {
		perror("recv call");
		exit(-1);
    }
    else if (messageLen == CONNECTION_CLOSED)
    {
        return 0;
    }

    /* Get pdu length stored at the first 2 bytes */
    uint16_t pdu_length = ntohs(*(uint16_t *)buf);

    if((bytesReceived = recv(socket_fd, buf + HDR_LEN,
                    pdu_length - HDR_LEN, MSG_WAITALL)) < 0)
    {
        perror("recv call");
        exit(-1);
    }
    else if (bytesReceived == CONNECTION_CLOSED)
    {
        return 0;
    }

    return bytesReceived;
}


int msg_send(int socket_fd, char *sendBuf)
{
	int sent = 0;
    uint16_t pdu_length = ntohs(*(uint16_t *)sendBuf);

	sent =  send(socket_fd, sendBuf, pdu_length, 0);
	if (sent < 0)
	{
		perror("send call");
        fprintf(stderr, "socket: %d\n", socket_fd);
		exit(-1);
	}
    return sent;
}
