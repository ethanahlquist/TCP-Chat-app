/******************************************************************************
* myClient.c
*
*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include <ctype.h>

#include "networks.h"
#include "message.h"
#include "packets.h"
#include "pollLib.h"

#define DEBUG_FLAG 1

void checkArgs(int argc, char * argv[]);
int readFromStdin(char * buffer, int buffer_size);
void setClientHandle(char * s);
void requestClientHandle(int socketNum);

int client_loop(int socketNum);
void process_input(int socketNum, char *buf, int size);

void process_message(char * data);
void process_broadcast(char * data);
void process_packet(int socketNum, char *buf, int size);


char * myHandle = NULL;

/*
   Prompt for the user input
   Send PDU with user input
   Receive first server response and print
   Receive second server response and print
   Go back to prompt user input
*/

int main(int argc, char * argv[])
{
	int socketNum = 0;         //socket descriptor

	checkArgs(argc, argv);

    /* Set handle to command line arg */
    setClientHandle(argv[1]);

	/* set up the TCP Client socket  */
	socketNum = tcpClientSetup(argv[2], argv[3], DEBUG_FLAG);

    requestClientHandle(socketNum);

    addToPollSet(socketNum);
    addToPollSet(STDIN_FILENO);

    client_loop(socketNum);

	close(socketNum);
	return 0;
}


void setClientHandle(char * s)
{
    int s_len = strlen(s);

    if(s_len > 100){
        fprintf(stderr, "Invalid handle, handle longer than 100 characters: %s\n", s);
        exit(EXIT_FAILURE);
    }

    if(isdigit(*s)){
        fprintf(stderr, "Invalid handle, handle starts with a number\n");
        exit(EXIT_FAILURE);
    }

    myHandle = s;
}


void requestClientHandle(int socketNum)
{
    sendHandle(socketNum, REQ_HAND_FLAG, myHandle);

    char buf[MAXBUF];
    msg_receive(socketNum, buf);

    switch (getHeader(buf)->flag) {
        case REQ_GRANTED_FLAG:
            break;
        case REQ_DENIED_FLAG:
            fprintf(stderr, "Handle already in use: %s\n", myHandle);
            exit(EXIT_FAILURE);
            break;
        default:
            fprintf(stderr, "Received unexpected flag\n");
            exit(EXIT_FAILURE);
            break;
    }
}


void process_message(char * data)
{
    ONE_HANDLE_PDU(data, one_handle_pdu);
    struct one_handle_pdu * pdu = (struct one_handle_pdu *) data;

    uint8_t num_handles = *(uint8_t*)(data + sizeof(struct one_handle_pdu));
    void * growing_ptr = ((char*)(pdu + 1) + 1);

    int i;
    char handleBuf[MAX_HANDLE_SIZE+1];

    for (i = 0; i < num_handles; ++i) {
        growing_ptr = getHandle(handleBuf, growing_ptr);
    }

    /* Print msg, in "handle: msg" format */
    printf("%.*s: %s\n", pdu->handle_len, pdu->handle_name,
            (char*)growing_ptr);
}


void process_broadcast(char * data)
{
    ONE_HANDLE_PDU(data, one_handle_pdu);
    struct one_handle_pdu * pdu = (struct one_handle_pdu *) data;

    printf("%.*s: %s\n", pdu->handle_len, pdu->handle_name,
            (char*)(pdu+1));
}


void process_packet(int socketNum, char *buf, int size)
{
    struct pdu_header * header = getHeader(buf);
    ONE_HANDLE_PDU(buf, one_handle_pdu);
    struct one_handle_pdu * pdu = (struct one_handle_pdu *) buf;

    switch (header->flag) {
        case REQ_GRANTED_FLAG:
            // done earlier
            break;
        case REQ_DENIED_FLAG:
            // done earlier
            break;
        case BROADCAST_FLAG:
            process_broadcast(buf);
            break;
        case MSG_FLAG:
            process_message(buf);
            break;
        case HANDLE_DNE_FLAG:
            fprintf(stderr, "Client with handle %.*s does not exist\n",
                    pdu->handle_len, pdu->handle_name);
            break;
        case EXIT_AWK_FLAG:
            exit(EXIT_SUCCESS);
            break;
        case LIST_COUNT_FLAG:{
            struct list_count_pdu * count_pdu = (struct list_count_pdu *)buf;
            printf("Number of clients: %d\n", ntohl(count_pdu->count));
            break;
        }
        case LIST_HANDLE_FLAG:
            printf("\t%.*s\n", pdu->handle_len, pdu->handle_name);
            break;
        case LIST_DONE_FLAG:
            break;
        default:
            fprintf(stderr, "Client: Received unexpected flag\n");
            break;
    }
}


/*
   In process input, we are generally sending messages
   out via commands given by the user, we are also
   checking if these commands are valid.
*/
void process_input(int socketNum, char *buf, int size)
{
    char pdu[1400];

    char * cmd = strtok(buf, " ");

    if(strncmp(cmd, "%", 1)){
        fprintf(stderr, "Invalid command format\n");

    } else if (!strcasecmp(cmd, "%M")) {
        send_multi_handle_pdu(socketNum, pdu, myHandle, buf);

    } else if (!strcasecmp(cmd, "%B")){
        send_broadcast_pdu(socketNum, pdu, myHandle, buf);

    } else if (!strcasecmp(cmd, "%L")){
        sendHeader(socketNum, REQ_LIST_FLAG);

    } else if (!strcasecmp(cmd, "%E")){
        sendHeader(socketNum, CLIENT_EXIT_FLAG);

    } else {
        fprintf(stderr, "Invalid command\n");
    }
}


int client_loop(int socketNum)
{
	char receiveBuf[MAX_MSG_SIZE_STDIN];   //data buffer
    char input_buffer[MAX_MSG_SIZE_STDIN];

    int bytesReceived;
    int socket;

    while(1){

        printf("$: ");
        memset(input_buffer, 0, MAX_MSG_SIZE_STDIN);

        /* Add some data to input_buffer */
        if((socket = pollCall(-1)) == STDIN_FILENO){
            int buf_size = readFromStdin(input_buffer, MAX_MSG_SIZE_STDIN);
            process_input(socketNum, input_buffer, buf_size);
        }
        else if (socket == socketNum){
            bytesReceived = msg_receive(socketNum, receiveBuf);

            // If server dies, client will stop
            if(bytesReceived == 0){
                fprintf(stderr, "Server Terminated\n");
	            close(socketNum);
                return 1;
            } else {
                process_packet(socketNum, receiveBuf, bytesReceived);
            }
        } else {
            fprintf(stderr, "This shouldn't happen\n");
        }
    }
	close(socketNum);
	return 0;

}


int readFromStdin(char * buffer, int buffer_size)
{
	char aChar = 0;
	int inputLen = 0;

	// Important you don't input more characters than you have space
	buffer[0] = '\0';
	while (inputLen < (buffer_size - 1) && aChar != '\n')
	{
		aChar = getchar();
		if (aChar != '\n')
		{
			buffer[inputLen] = aChar;
			inputLen++;
		}
	}

	/* Null terminate the string */
	buffer[inputLen] = '\0';
	inputLen++;

	return inputLen;
}


void checkArgs(int argc, char * argv[])
{
	/* check command line arguments  */
	if (argc != 4)
	{
		printf("usage: %s host-name port-number \n", argv[0]);
		exit(1);
	}
}
