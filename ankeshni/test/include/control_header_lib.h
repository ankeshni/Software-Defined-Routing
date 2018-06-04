#ifndef CONTROL_HANDLER_LIB_H_
#define CONTROL_HANDLER_LIB_H_

#define CNTRL_HEADER_SIZE 8
#define CNTRL_RESP_HEADER_SIZE 8

#define PACKET_USING_STRUCT // Comment this out to use alternate packet crafting technique

#define INF 65535

#ifdef PACKET_USING_STRUCT
    struct __attribute__((__packed__)) CONTROL_HEADER
    {
        uint32_t dest_ip_addr;
        uint8_t control_code;
        uint8_t response_time;
        uint16_t payload_len;
    };

    struct __attribute__((__packed__)) CONTROL_RESPONSE_HEADER
    {
        uint32_t controller_ip_addr;
        uint8_t control_code;
        uint8_t response_code;
        uint16_t payload_len;
    };
     struct __attribute__((__packed__)) CTRL_TABLE_RESPONSE
    {
        uint64_t info[5]; /* id            padding
                           * next hop      cost    */
    }*routing_table;
    
       
    struct __attribute__((__packed__)) NEIGHBOUR_list
    {
       uint16_t id;
       uint16_t r_port;
       uint16_t d_port;
       uint32_t ip;
       uint16_t cost;
       int link_active;
       int update_rcv_flag;
       struct CTRL_TABLE_RESPONSE neighbour_r_table;
       
       
    }*neighbour_list[5];    
    
    int tot_neighbours;
    
    
    
#endif

char* create_response_header(int sock_index, uint8_t control_code, uint8_t response_code, uint16_t payload_len);

#endif