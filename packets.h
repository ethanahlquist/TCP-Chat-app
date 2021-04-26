#ifndef __PACKETS_H__
#define __PACKETS_H__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define MAX_HANDLE_SIZE 100

#define MAX_MSG_SIZE_STDIN 1400
#define MAX_MSG_SIZE 200

#define REQ_HAND_FLAG 1
#define REQ_GRANTED_FLAG 2
#define REQ_DENIED_FLAG 3
#define BROADCAST_FLAG 4
#define MSG_FLAG 5
#define HANDLE_DNE_FLAG 7
#define CLIENT_EXIT_FLAG 8
#define EXIT_AWK_FLAG 9
#define REQ_LIST_FLAG 10
#define LIST_COUNT_FLAG 11
#define LIST_HANDLE_FLAG 12
#define LIST_DONE_FLAG 13


#define MIN(a,b) (((a)<(b))?(a):(b))

/*
   This is a MACRO that defines a struct
   (based on an provided packet), grabbing the
   first handle name and length into its fields.
*/
#define ONE_HANDLE_PDU(data,pdu_name) \
        struct pdu_name { uint16_t length; \
        uint8_t flag; \
        uint8_t handle_len; \
        char handle_name[*(uint8_t *)(data + 3)]; \
    }__attribute__((packed));


/* Not used much */
#define GET_8_BITS_AT(loc) *(uint8_t *)(loc)


struct pdu_header
{
    uint16_t length;
    uint8_t flag;
} __attribute__((packed));


struct list_count_pdu {
    uint16_t length;
    uint8_t flag;
    uint32_t count;
}__attribute__((packed));


void sendHandle(int socket, uint8_t flag, const char* handle);
void sendHeader(int socket , uint8_t flag);
void * add_pdu_header(void *ptr, int flag);
void * add_pdu_handle(void *pdu, void *current_ptr, char *handle_name);
void setHeaderNetOrder(void *data);
void setHeaderHostOrder(void *data);
void send_multi_handle_pdu(int socketNum, void * ptr, char *my_handle, char * handle_args);
void send_broadcast_pdu(int socketNum, void * pdu, char *my_handle, char * user_input);
void * printHandles(void * data, uint8_t num_handles);
struct pdu_header * getHeader(void *data);
void * getHandle(char *dest, void *data);

#endif
