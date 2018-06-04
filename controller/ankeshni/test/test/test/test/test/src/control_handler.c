/**
 * @control_handler
 * @author  Swetank Kumar Saha <swetankk@buffalo.edu>
 * @version 1.0
 *
 * @section LICENSE
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details at
 * http://www.gnu.org/copyleft/gpl.html
 *
 * @section DESCRIPTION
 *
 * Handler for the control plane.
 */

#include <sys/socket.h>
#include <netinet/in.h>
#include <strings.h>
#include <sys/queue.h>
#include <unistd.h>
#include <string.h>

#include "../include/global.h"
#include "../include/network_util.h"
#include "../include/control_header_lib.h"
#include "../include/author.h"

#ifndef PACKET_USING_STRUCT
    #define CNTRL_CONTROL_CODE_OFFSET 0x04
    #define CNTRL_PAYLOAD_LEN_OFFSET 0x06
#endif

/* Linked List for active control connections */
struct ControlConn
{
    int sockfd;
    LIST_ENTRY(ControlConn) next;
}*connection, *conn_temp;
LIST_HEAD(ControlConnsHead, ControlConn) control_conn_list;

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

void init_response(int , char*);
void save_init_payload(char*);
void control_table_response(int, struct initpayload*);
void make_ctrl_tbl_response(struct initpayload*);

int create_control_sock()
{
    int sock;
    struct sockaddr_in control_addr;
    socklen_t addrlen = sizeof(control_addr);

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock < 0)
        ERROR("socket() failed");

    /* Make socket re-usable */
    if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (int[]){1}, sizeof(int)) < 0)
        ERROR("setsockopt() failed");

    bzero(&control_addr, sizeof(control_addr));

    control_addr.sin_family = AF_INET;
    control_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    control_addr.sin_port = htons(CONTROL_PORT);

    if(bind(sock, (struct sockaddr *)&control_addr, sizeof(control_addr)) < 0)
        ERROR("bind() failed");

    if(listen(sock, 5) < 0)
        ERROR("listen() failed");

    LIST_INIT(&control_conn_list);

    return sock;
}

int new_control_conn(int sock_index)
{
    int fdaccept, caddr_len;
    struct sockaddr_in remote_controller_addr;

    caddr_len = sizeof(remote_controller_addr);printf("waiting for controller message");
    fdaccept = accept(sock_index, (struct sockaddr *)&remote_controller_addr, &caddr_len);
    if(fdaccept < 0)
        ERROR("accept() failed");

    /* Insert into list of active control connections */
    connection = malloc(sizeof(struct ControlConn));
    connection->sockfd = fdaccept;
    LIST_INSERT_HEAD(&control_conn_list, connection, next);

    return fdaccept;
}

void remove_control_conn(int sock_index)
{
    LIST_FOREACH(connection, &control_conn_list, next) {
        if(connection->sockfd == sock_index) LIST_REMOVE(connection, next); // this may be unsafe?
        free(connection);
    }

    close(sock_index);
}

bool isControl(int sock_index)
{
    LIST_FOREACH(connection, &control_conn_list, next)
        if(connection->sockfd == sock_index) return TRUE;

    return FALSE;
}

bool control_recv_hook(int sock_index)
{
    char *cntrl_header, *cntrl_payload;
    uint8_t control_code;
    uint16_t payload_len;

    /* Get control header */
    cntrl_header = (char *) malloc(sizeof(char)*CNTRL_HEADER_SIZE);
    bzero(cntrl_header, CNTRL_HEADER_SIZE);

    if(recvALL(sock_index, cntrl_header, CNTRL_HEADER_SIZE) < 0){
        remove_control_conn(sock_index);
        free(cntrl_header);
        return FALSE;
    }

    /* Get control code and payload length from the header */
    #ifdef PACKET_USING_STRUCT
        /** ASSERT(sizeof(struct CONTROL_HEADER) == 8) 
          * This is not really necessary with the __packed__ directive supplied during declaration (see control_header_lib.h).
          * If this fails, comment #define PACKET_USING_STRUCT in control_header_lib.h
          */
        BUILD_BUG_ON(sizeof(struct CONTROL_HEADER) != CNTRL_HEADER_SIZE); // This will FAIL during compilation itself; See comment above.

        struct CONTROL_HEADER *header = (struct CONTROL_HEADER *) cntrl_header;
        control_code = header->control_code;
        payload_len = ntohs(header->payload_len);
    #endif
    #ifndef PACKET_USING_STRUCT
        memcpy(&control_code, cntrl_header+CNTRL_CONTROL_CODE_OFFSET, sizeof(control_code));
        memcpy(&payload_len, cntrl_header+CNTRL_PAYLOAD_LEN_OFFSET, sizeof(payload_len));
        payload_len = ntohs(payload_len);
    #endif

    free(cntrl_header);

    /* Get control payload */
    if(payload_len != 0){
        cntrl_payload = (char *) malloc(sizeof(char)*payload_len);
        bzero(cntrl_payload, payload_len);

        if(recvALL(sock_index, cntrl_payload, payload_len) < 0){
            remove_control_conn(sock_index);
            free(cntrl_payload);
            return FALSE;
        }
    }
   

    /* Triage on control_code */
    switch(control_code){
        case 0: author_response(sock_index);
                break;
        case 1: init_response(sock_index, cntrl_payload);
                break;
        case 2: control_table_response(sock_index, init_payload);
                break;
                

            
    }

    if(payload_len != 0) free(cntrl_payload);
    return TRUE;
}
 void init_response(int sock_index, char *cntrl_payload)
    {
        uint16_t payload_len, response_len;
	char *cntrl_response_header, *cntrl_response_payload, *cntrl_response;

	payload_len = 0;//sizeof(AUTHOR_STATEMENT)-1; // Discount the NULL chararcter
	cntrl_response_payload = 0; //(char *) malloc(payload_len);
	//memcpy(cntrl_response_payload, AUTHOR_STATEMENT, payload_len);

	cntrl_response_header = create_response_header(sock_index, 1, 0, payload_len);

	response_len = CNTRL_RESP_HEADER_SIZE+payload_len;
	cntrl_response = (char *) malloc(response_len);
	/* Copy Header */
	memcpy(cntrl_response, cntrl_response_header, CNTRL_RESP_HEADER_SIZE);
	free(cntrl_response_header);
	/* Copy Payload */
	memcpy(cntrl_response+CNTRL_RESP_HEADER_SIZE, cntrl_response_payload, payload_len);
	free(cntrl_response_payload);

	sendALL(sock_index, cntrl_response, response_len);

	free(cntrl_response);
        printf("sent response to the controller");
        save_init_payload(cntrl_payload);
        
    }
 
 
 void save_init_payload(char *cntrl_payload)
 {
      //struct initpayload *init_payload;
      init_payload = (struct initpayload*) malloc(sizeof(struct initpayload));
      uint16_t total_routers=ntohs(*(uint16_t*) cntrl_payload);
      uint16_t update_interval=ntohs(*(((uint16_t*) cntrl_payload)+1));
      
      init_payload->total_routers=total_routers;
      init_payload->update_interval=update_interval;
      int u_16=2,u_32=3;
      for(int i=0;i<5;i++)
      {
          init_payload->router_list[i].id=ntohs(*(((uint16_t*) cntrl_payload)+u_16++));
          init_payload->router_list[i].router_port=ntohs(*(((uint16_t*) cntrl_payload)+u_16++));
          init_payload->router_list[i].data_port=ntohs(*(((uint16_t*) cntrl_payload)+u_16++));
          init_payload->router_list[i].cost=ntohs(*(((uint16_t*) cntrl_payload)+u_16++));
          init_payload->router_list[i].ip = (uint32_t)(htonl(*(((uint32_t*) cntrl_payload)+u_32)));u_32+=3;u_16+=2;
      }
 }
   // struct CTRL_TABLE_RESPONSE *ctrl_table_response;

 
 void control_table_response(int sock_index, struct initpayload* init_payload)
 {
      ctrl_table_response =(struct CTRL_TABLE_RESPONSE*) malloc(sizeof(struct CTRL_TABLE_RESPONSE));

          
     uint16_t payload_len, response_len;
	char *cntrl_response_header, *cntrl_response_payload, *cntrl_response;

	
	//cntrl_response_payload =(char *) malloc(10*sizeof(uint32_t));
       
        //payload_len =sizeof(*init_payload); 
        
      // struct CTRL_TABLE_RESPONSE *ctrl_table_response=
        make_ctrl_tbl_response(init_payload);
        
        payload_len =sizeof(*ctrl_table_response); 
        cntrl_response_payload=(char*) malloc(payload_len);
	memcpy(cntrl_response_payload,ctrl_table_response, payload_len);

	cntrl_response_header = create_response_header(sock_index, 2, 0, payload_len);

	response_len = CNTRL_RESP_HEADER_SIZE+payload_len;
	cntrl_response = (char *) malloc(response_len);
	/* Copy Header */
	memcpy(cntrl_response, cntrl_response_header, CNTRL_RESP_HEADER_SIZE);
	free(cntrl_response_header);
	/* Copy Payload */
	memcpy(cntrl_response+CNTRL_RESP_HEADER_SIZE, cntrl_response_payload, payload_len);
	free(cntrl_response_payload);

	sendALL(sock_index, cntrl_response, response_len);

	free(cntrl_response);
        
        
 }
 
 uint16_t my_id(struct initpayload* init_payload)
    {
        for(int i=0;i<init_payload->total_routers;i++)
    {
        if(init_payload->router_list[i].cost==0)
        {
            return init_payload->router_list[i].id;
        }
        
    }
    
        perror("Unable to find my id");
    
    }
 
 //struct CTRL_TABLE_RESPONSE* make_ctrl_tbl_response(struct initpayload* init_payload)
 void make_ctrl_tbl_response(struct initpayload* init_payload)
  {
    
   /* id            padding
    * next hop      cost    */
    int u_16=0;
    for(int i=0;i<init_payload->total_routers;i++)
    {   //remove starting padding of 0x0000
        if(init_payload->router_list[i].cost==0)
        {
            *(((uint16_t*)(&(ctrl_table_response->info[i]))+u_16++))= htons(init_payload->router_list[i].id);
            *(((uint16_t*)(&(ctrl_table_response->info[i]))+u_16++))= htons(0);
            *(((uint16_t*)(&(ctrl_table_response->info[i]))+u_16++))= htons(init_payload->router_list[i].id);
            *(((uint16_t*)(&(ctrl_table_response->info[i]))+u_16++))= htons(0);
            //u_16=8;//skip padding  
        }
        else if(init_payload->router_list[i].cost==INF)
        {
            *(((uint16_t*)(&(ctrl_table_response->info[i]))+u_16++))= htons(init_payload->router_list[i].id);
            *(((uint16_t*)(&(ctrl_table_response->info[i]))+u_16++))= htons(0);
            *(((uint16_t*)(&(ctrl_table_response->info[i]))+u_16++))= htons(INF);
            *(((uint16_t*)(&(ctrl_table_response->info[i]))+u_16++))= htons(INF);
            //u_16=8;//skip padding
        }
        else
        {
            *(((uint16_t*)(&(ctrl_table_response->info[i]))+u_16++))= htons(init_payload->router_list[i].id);
            *(((uint16_t*)(&(ctrl_table_response->info[i]))+u_16++))= htons(0);
            *(((uint16_t*)(&(ctrl_table_response->info[i]))+u_16++))= htons(my_id(init_payload));
            *(((uint16_t*)(&(ctrl_table_response->info[i]))+u_16++))= htons(init_payload->router_list[i].cost);
           // u_16=8;//skip padding
        }
        u_16=0;
    }
    
    //return ctrl_table_response;
    
 }