#ifndef __TABLE_H__
#define __TABLE_H__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>


const char *table_get_next_value(int restart);
void table_iter(int (*func)(int, char*), char* data);
void table_init();
void table_add(char * handle_name, int socket);
int table_get(char* handle_name);
void table_remove(int fd);
uint32_t table_count();

#endif
