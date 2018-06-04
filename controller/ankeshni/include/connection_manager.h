#ifndef CONNECTION_MANAGER_H_
#define CONNECTION_MANAGER_H_
#include<time.h>

struct timeval timeout;
fd_set master_list, watch_list;
int head_fd;
int control_socket, router_socket, data_socket;

void init();

#endif