#include "packets.h"
#include "message.h"

void sendHandle(int socket, uint8_t flag, const char* handle)
{
    int s_len = strlen(handle);

    struct one_handle_pdu {
        uint16_t length;
        uint8_t flag;
        uint8_t handle_len;
        char handle_name[s_len];
    } pdu;


    strncpy(pdu.handle_name, handle, s_len);
    pdu.handle_len = s_len;
    pdu.flag = flag;
    pdu.length = htons(sizeof(pdu));

    msg_send(socket, (char*)&pdu);
}


void sendHeader(int socket , uint8_t flag)
{
    char buf[sizeof(struct pdu_header)];
    struct pdu_header *header = getHeader(&buf);
    header->length =  htons(sizeof(struct pdu_header));
    header->flag = flag;

    msg_send(socket, buf);
}


/*
   Adds provided header data to the location of ptr
*/
void * add_pdu_header(void *ptr, int flag)
{
    struct pdu_header *header = getHeader(ptr);
    header->length = sizeof(*header); // The header itself should be 3 bytes size
    header->flag = flag;

    return header + 1;
}


/*
   Adds provided handle name and length data to the location of ptr
*/
void * add_pdu_handle(void *pdu, void *current_ptr, char *handle_name)
{
    uint8_t length = strlen(handle_name);

    struct pdu_handle {
        uint8_t length;
        char name[length];
    }__attribute__((packed));

    struct pdu_handle *handle = (struct pdu_handle *) current_ptr;

    handle->length = length;
    strncpy(handle->name, handle_name, length);

    /* Add handle size to pdu header length */
    getHeader(pdu)->length += sizeof(*handle);

    return handle + 1; // handle size + actual
}


void splitAndSend(int socketNum, void *pdu, char* build_loc, char* msg)
{

    struct pdu_header *header = getHeader(pdu);
    uint16_t pre_length = header->length;

    int i;
    for (i = 0; i < strlen(msg); i += MAX_MSG_SIZE) {
        int amount_left = strlen(&msg[i]);
        strncpy(build_loc, &msg[i], MAX_MSG_SIZE);

        int min_msg_len = MIN(amount_left, MAX_MSG_SIZE);
        build_loc[min_msg_len] = '\0';

        header->length = htons(pre_length + min_msg_len + 1);
        msg_send(socketNum, (char*)pdu);
    }

}


void send_broadcast_pdu(int socketNum, void * pdu, char *my_handle, char * user_input)
{
    char * build_ptr = pdu;
    build_ptr = add_pdu_header(build_ptr, BROADCAST_FLAG);
    build_ptr = add_pdu_handle(pdu, build_ptr, my_handle);

    char * msg = strtok(NULL, "\0");

    /* Don't send if msg is just a blank line */
    if(msg == NULL){
        return;
    }
    splitAndSend(socketNum, pdu, build_ptr, msg);
}


void send_multi_handle_pdu(int socketNum, void * pdu, char *my_handle, char * user_input)
{
    void * build_ptr = pdu;
    build_ptr = add_pdu_header(build_ptr, MSG_FLAG);
    build_ptr = add_pdu_handle(pdu, build_ptr, my_handle);


    /* If line is empty after %M, don't send anything */
    char * s_num_handles = strtok(NULL, " ");
    if(s_num_handles == NULL){
        fprintf(stderr, "Invalid command format\n");
        return;
    }

    /* Get num_handles */
    uint8_t num_handles = atoi(s_num_handles);

    if(num_handles < 1){
        fprintf(stderr, "number of dest handles need to be greater than 0\n");
        return;
    }

    // add uint8_t to data stream
    *(uint8_t *)build_ptr = (uint8_t)num_handles;
    build_ptr = ((uint8_t *)build_ptr) + 1;

    // add 1 byte to the PDU length, which is at [0]
    getHeader(pdu)->length += 1;


    // Get number of handles from string: "2 hand_1 hand_2"
    char * handle_name;
    int i;
    for (i = 0; i < num_handles; ++i)
    {
        // get first handle name
        handle_name = strtok(NULL, " ");
        if(strlen(handle_name) > MAX_HANDLE_SIZE){
            fprintf(stderr, "Handles must be under 100 bytes\n");
            return;
        }
        if(handle_name == NULL){
            fprintf(stderr, "Not enough handles provided\n");
            return;
        }
        build_ptr = add_pdu_handle(pdu ,build_ptr, handle_name);
    }

    /* Weird but gets past strtok's NULL char */
    char *msg = memchr(handle_name, '\0', 101) + 1;

    /* Don't send if msg is empty */
    if(msg == NULL){
        fprintf(stderr, "Invalid command format\n");
        return;
    }

    splitAndSend(socketNum, pdu, build_ptr, msg);
}


struct pdu_header * getHeader(void *data)
{
    return (struct pdu_header *)data;
}


void * getHandle(char *dest, void *data)
{
    uint8_t length = *(uint8_t*)data;
    strncpy(dest, data + 1, length);
    dest[length] = '\0';

    return data + 1 + length;
}

void * printHandles(void * data, uint8_t num_handles){
    char nameBuf[MAX_HANDLE_SIZE];
    int i;
    for (i = 0; i < num_handles; ++i) {
        data = getHandle(nameBuf, data);
        printf("%d %s \n", (int)strlen(nameBuf), nameBuf);
    }
    return data;
}

