/******************************************************************************
* tcp_server.c
*
* CPE 464 - Program 1
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

#include "networks.h"
#include "message.h"
#include "pollLib.h"
#include "packets.h"
#include "table.h"

#define MAXBUF 1024
#define DEBUG_FLAG 1

int process_event(int clientSocket);
void recvFromClient(int clientSocket);
int checkArgs(int argc, char *argv[]);

int main(int argc, char *argv[])
{
    int socket, serverSocket, clientSocket, portNumber;

    table_init();
	portNumber = checkArgs(argc, argv);

	//create the server socket
	serverSocket = tcpServerSetup(portNumber);

    // add server socket to the pollset
    addToPollSet(serverSocket);

    while(1){

        // wait for client to connect
        if((socket = pollCall(-1)) == serverSocket){
            clientSocket = tcpAccept(serverSocket, DEBUG_FLAG);
            addToPollSet(clientSocket);
        } else{
            process_event(socket);
        }
    }
	close(serverSocket);
	return 0;
}


/* Remove client data from pollCall, handle table and close fd */
void removeClient(int clientSocket){
    /*printf("client disconnected\n");*/

    // Removes field, given a fd (not handle name)
    table_remove(clientSocket);

    removeFromPollSet(clientSocket);
    close(clientSocket);
}


void process_message(int clientSocket, char * data)
{
    ONE_HANDLE_PDU(data, one_handle_pdu);
    struct one_handle_pdu * pdu = (struct one_handle_pdu *) data;
    uint8_t num_handles = *(uint8_t*)(data + sizeof(struct one_handle_pdu));

    void * growing_ptr = ((char*)(pdu + 1) + 1);

    int i;
    char handleBuf[MAX_HANDLE_SIZE+1];

    for (i = 0; i < num_handles; ++i) {
        growing_ptr = getHandle(handleBuf, growing_ptr);
        printf("Handle: %s\n", handleBuf);

        /* get fd dest for this handle name */
        int fd = table_get(handleBuf);

        if(fd > 0){
            msg_send(fd, (char*)data);
        } else {
            struct pdu_header invalid_pdu;
            void * build_ptr;

            build_ptr = add_pdu_header(&invalid_pdu, HANDLE_DNE_FLAG);
            build_ptr = add_pdu_handle(&invalid_pdu, build_ptr, handleBuf);
            invalid_pdu.length = htons(invalid_pdu.length);

            msg_send(clientSocket, (char*)&invalid_pdu);

        }
    }
}


void process_broadcast(int clientSocket, char * data)
{
    table_iter(msg_send, data);
}


void process_handle_request(int clientSocket, char * data)
{
    ONE_HANDLE_PDU(data, one_handle_pdu);
    struct one_handle_pdu * pdu = (struct one_handle_pdu *) data;

    char handle_key[pdu->handle_len +1];
    getHandle(handle_key , &pdu->handle_len);

    struct pdu_header response;
    response.length = htons(sizeof(response));

    /* Check if handle is availible */
    int fd = table_get(handle_key);
    if(fd < 0){
        response.flag = REQ_GRANTED_FLAG;
    } else {
        response.flag = REQ_DENIED_FLAG;
    }

    table_add(handle_key, clientSocket);
    msg_send(clientSocket, (char*)&response);
}


#define RESTART 1
#define CONTINUE 0
void process_list_req(int socketNum)
{
    /* Send List count */
    struct list_count_pdu count_pdu;

    count_pdu.length = htons(sizeof(struct list_count_pdu));
    count_pdu.flag = LIST_COUNT_FLAG;
    count_pdu.count = htonl(table_count());

    msg_send(socketNum, (char*)&count_pdu);

    /* Send Handle names */
    const char * handle;
    table_get_next_value(RESTART);

    while((handle = table_get_next_value(CONTINUE)) != NULL) {
        sendHandle(socketNum, LIST_HANDLE_FLAG, handle);
    }

    /* Send Done Flag */
    sendHeader(socketNum, LIST_DONE_FLAG);
}


int process_event(int clientSocket)
{
    int received;
    char receiveBuf[MAX_MSG_SIZE_STDIN];

    received = msg_receive(clientSocket, receiveBuf);

    if(received == 0){
        removeClient(clientSocket);
        return 0 ;
    }

    struct pdu_header * header = getHeader(receiveBuf);
    printf("Header length: %d\n", ntohs(header->length));
    printf("Header Flag: %d\n", header->flag);

    switch (header->flag) {
        case REQ_HAND_FLAG:
            process_handle_request(clientSocket, receiveBuf);
            break;
        case MSG_FLAG:
            process_message(clientSocket, receiveBuf);
            break;
        case BROADCAST_FLAG:
            process_broadcast(clientSocket, receiveBuf);
            break;
        case REQ_LIST_FLAG:
            process_list_req(clientSocket);
            break;
        case CLIENT_EXIT_FLAG:
            sendHeader(clientSocket, EXIT_AWK_FLAG);
            removeClient(clientSocket);
            break;
        default:
            msg_send(clientSocket, receiveBuf);
            break;
    }
    return 0;
}


int checkArgs(int argc, char *argv[])
{
	// Checks args and returns port number
	int portNumber = 0;

	if (argc > 2)
	{
		fprintf(stderr, "Usage %s [optional port number]\n", argv[0]);
		exit(-1);
	}

	if (argc == 2)
	{
		portNumber = atoi(argv[1]);
	}

	return portNumber;
}

