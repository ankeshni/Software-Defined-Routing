#ifndef CONTROL_HANDLER_H_
#define CONTROL_HANDLER_H_

int new_data_conn(int sock_index);
int create_control_sock();
int new_control_conn(int sock_index);
bool isControl(int sock_index);
bool control_recv_hook(int sock_index);
uint16_t my_id(struct initpayload* init_payload);
uint16_t update_interval;

#endif