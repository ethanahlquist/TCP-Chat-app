#include <string.h>  /* strcpy */
#include <stdlib.h>  /* malloc */
#include <stdio.h>   /* printf */
#include <stdint.h>   /* uint32_t */
#include "safeUtil.h"
#include "table.h"

char ** table = NULL;
unsigned int table_size = 3;


void table_init()
{
    if(table == NULL){
        table = srealloc(table, table_size);
    }
}


void table_add(char * handle_name, int socket)
{
    if(socket >= table_size){

        int next_table_size = table_size << 1;

        char ** tmp_table = (char **)sCalloc(next_table_size, sizeof(char *));
        memcpy(tmp_table, table, table_size*sizeof(char*));

        free(table);

        table = tmp_table;
        table_size = next_table_size;
    }

    if(table[socket] == NULL){
        // We can add data
        table[socket] = (char*)sCalloc(strlen(handle_name), sizeof(char));
        strcpy(table[socket], handle_name);
    }
}


void table_iter(int (*func)(int, char*), char* data)
{
    int i;
    for (i = 0; i < table_size; ++i) {
        if(table[i] != NULL){
            func(i, data);
        }
    }
}


void table_remove(int fd)
{
    if(table[fd] == NULL){
        fprintf(stderr, "Table: fd is already NULL\n");
        return;
    }

    free(table[fd]);
    table[fd] = NULL;
}

int table_get(char* handle_name)
{
    int i;
    for (i = 0; i < table_size; ++i) {
        if(table[i] != NULL && !strcmp(table[i], handle_name))
            return i;
    }
    return -1;
}


const char *table_get_next_value(int restart)
{
    static uint32_t i = 0;

    if(restart){
        i = 0;
        return NULL;
    }

    for (; i < table_size; i++) {
        if(table[i] != NULL){
            return table[i++];
        }
    }

    return NULL;
}


uint32_t table_count()
{
    uint32_t i;
    uint32_t count = 0;
    for (i = 0; i < table_size; ++i) {
        if(table[i] != NULL)
            count ++;
    }
    return count;
}
