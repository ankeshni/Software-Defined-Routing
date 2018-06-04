/**
 * @connection_manager
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
 * Connection Manager listens for incoming connections/messages from the
 * controller and other routers and calls the desginated handlers.
 */
#include <sys/types.h>
#include <sys/select.h>
#include<time.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include<string.h>

#include "../include/connection_manager.h"
#include "../include/global.h"
#include "../include/control_handler.h"
#include "timer_queue.h"
#include "control_header_lib.h"


void save_neighbour_r_table(int);
void main_loop()
{
    int selret, sock_index, fdaccept,fd_data_accept,sign =0;

    while(TRUE){
        watch_list = master_list;
        selret = select(head_fd+1, &watch_list, NULL, NULL, &timeout);

        if(selret < 0)
            ERROR("select failed.");
        
        if(selret!=0)
        {

       
                    /* Loop through file descriptors to check which ones are ready */
                    for(sock_index=0; sock_index<=head_fd; sock_index+=1){

                        if(FD_ISSET(sock_index, &watch_list)){

                            /* control_socket */
                            if(sock_index == control_socket){
                                fdaccept = new_control_conn(sock_index);
                               /* Add to watched socket list */
                                FD_SET(fdaccept, &master_list);
                                if(fdaccept > head_fd) head_fd = fdaccept;
                            }

                            /* router_socket */
                            else if(sock_index == router_socket){
                                //printf("Router Sock initialised on %d",sock_index);
                                //call handler that will call recvfrom() .....
                                //if(rcv_flag==1)
                                //{
                                   save_neighbour_r_table(sock_index);
                                   rcv_flag=0;
                                //}
                                
                            }

                            /* data_socket */
                            else if(sock_index == data_socket){
                                printf("data Sock initialised on %d",sock_index);
                                fd_data_accept= new_data_conn(sock_index);//make function
                                FD_SET(fd_data_accept, &master_list);
                                if(fd_data_accept > head_fd) head_fd = fd_data_accept;
                            }

                            /* Existing connection */
                            else{
                                if(isControl(sock_index)){
                                    if(!control_recv_hook(sock_index)) FD_CLR(sock_index, &master_list);
                                }
                                //else if isData(sock_index);
                                else ERROR("Unknown socket index");
                            }
                        }
                    }
                    
        }            
            else
            {
                    process_timeout();  //deal with timers            
            }
        
        }
}

void init()
{
    control_socket = create_control_sock();
    timeout.tv_sec=0xffffffffffffff;//wait for init
    timeout.tv_usec=0;
    //router_socket and data_socket will be initialized after INIT from controller

    FD_ZERO(&master_list);
    FD_ZERO(&watch_list);

    /* Register the control socket */
    FD_SET(control_socket, &master_list);
    head_fd = control_socket;

    main_loop();
}

ssize_t recv_udp(int sock_index, char *buffer, ssize_t nbytes,struct sockaddr_in *client_address,int* client_address_len)
{
    ssize_t bytes = 0;
    bytes = recvfrom(sock_index, buffer, nbytes, 0,(struct sockaddr *)client_address,client_address_len);

    if(bytes == 0) return -1;
    while(bytes != nbytes)
        bytes += recvfrom(sock_index, buffer+bytes, nbytes-bytes, 0,(struct sockaddr *)client_address,client_address_len);

    return bytes;
}
void belman_ford(int i);
uint16_t cost_through_neighbour(uint16_t id,int i);
uint16_t initial_cost(uint16_t id);
void save_neighbour_r_table(int r_sock)
{
    char buffer[sizeof(*routing_table)*5];
    struct sockaddr_in client_address;
    int client_address_len = sizeof(struct sockaddr_in);
//    if(recv_udp(router_socket, buffer, sizeof(buffer),&client_address,&client_address_len)<0)
    if(recvfrom(router_socket, buffer, sizeof(buffer),0,(struct sockaddr*)&client_address,&client_address_len)<0)
    printf("recv_udp failed");
    printf("received update \n");
    
    for(int i=0;i<tot_neighbours;i++)
    {
        if(neighbour_list[i]->ip==ntohl(client_address.sin_addr.s_addr))
        {
           // for(int j=0;j<=5;j++)
            //{     
            neighbour_list[i]->update_rcv_flag=0;     
            memcpy(neighbour_list[i]->neighbour_r_table.info,buffer,sizeof(neighbour_list[i]->neighbour_r_table.info));  
            belman_ford(i);            
                //neighbour_list[i]->neighbour_r_table.info[j]=*((uint64_t*)(buffer+j));
            //}
            int c=1;
            while(c)
            {
                if(c==100000000000000)
                    c=0;
                c++;
            }
            
            //insert_timer(neighbour_list[i]->id);
            for(int x=0; x<tot_neighbours ;x++)
             send_update(neighbour_list[x]->r_port,neighbour_list[x]->ip);
            
           printf("received update from %d\n",neighbour_list[i]->id);
            //uint16_t x = (uint16_t*)&(neighbour_list[i]->neighbour_r_table.info[0]);
           belman_ford(i);
            
        }
        
        
    }
    
    //fill up neighbour routing table struct and insert timer and figure out neighbour recv flag update here and in elese conditon of timeout
    //apply belman ford as soon as neighbours r table received
    
    
}

void belman_ford(int i)
{
  for(int j=0;j<5;j++)
  {
     if(initial_cost(*((uint16_t*)(&(routing_table->info[j])+0)))>cost_through_neighbour(*((uint16_t*)(&(neighbour_list[i]->neighbour_r_table.info[j])+0)),i))
     {
         *((uint16_t*)(&(routing_table->info[j])+3))= cost_through_neighbour(*((uint16_t*)(&(neighbour_list[i]->neighbour_r_table.info[j])+0)),i);
     }
  }
      
}

uint16_t cost_through_neighbour(uint16_t id,int i)
{
    uint16_t cost;
     for(int j=0;j<5;j++)
     {
         if( id  ==     *((uint16_t*)(&(neighbour_list[i]->neighbour_r_table.info[j])+0))     )
         {
             cost =  *((uint16_t*)(&(neighbour_list[i]->neighbour_r_table.info[j])+3));
         }
     }
    
    
    return (cost+neighbour_list[i]->cost);
}

uint16_t initial_cost(uint16_t id)
{
    uint16_t cost;
    
    for(int j=0;j<5;j++)
     {
         if( id  ==     *((uint16_t*)(&(routing_table->info[j])+0))     )
         {
             cost =  *((uint16_t*)(&(routing_table->info[j])+0));
         }
     }
            
    return cost;
}