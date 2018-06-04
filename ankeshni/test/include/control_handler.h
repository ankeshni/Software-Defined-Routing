#ifndef CONTROL_HANDLER_H_
#define CONTROL_HANDLER_H_

struct __attribute__((__packed__)) router
 {
   uint16_t id;
   uint16_t router_port;
   uint16_t data_port;
   uint16_t cost;
   uint32_t ip;
   
 };
 struct __attribute__((__packed__)) initpayload
 {
    uint16_t total_routers;
    uint16_t update_interval;
    struct router router_list[5];
 }*init_payload;



int new_data_conn(int sock_index);
int create_control_sock();
int new_control_conn(int sock_index);
bool isControl(int sock_index);
bool control_recv_hook(int sock_index);
uint16_t my_id(struct initpayload* init_payload);
uint16_t update_interval;

#endif