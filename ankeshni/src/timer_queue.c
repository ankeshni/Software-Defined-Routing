/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include <sys/socket.h>
#include <netinet/in.h>
#include <strings.h>
#include <sys/queue.h>
#include <unistd.h>
#include <string.h>
#include "../include/global.h"
#include "../include/connection_manager.h"
#include "../include/timer_queue.h"
#include "../include/control_handler.h"
#include "../include/connection_manager.h"
#include "../include/control_header_lib.h"
#include <sys/time.h>


void init_timers()
{
    front=NULL;
    rear= NULL;
    insert_timer(my_id(init_payload));
    timeout.tv_sec = update_interval;
    timeout.tv_usec =0;        
    
    
}
void insert_timer(uint16_t id)
{
   struct timers *newtimer;
   newtimer = (struct timers*)malloc(sizeof(struct timers));
   newtimer->id = id;
    gettimeofday(&(newtimer->expiry), NULL); 
   (newtimer->expiry.tv_sec)+=update_interval;   
   newtimer -> next = NULL;
   if(front == NULL)
      front = rear = newtimer;
   else{
      rear -> next = newtimer;
      rear = newtimer;
   }
   
   printf("\nInsertion is Success!!!\n");
}

//void insert_timer(uint16_t id)
//{
//   struct timers *newtimer,*bef_i=front,*i=front;
//   newtimer = (struct timers*)malloc(sizeof(struct timers));
//   newtimer->id = id;
//   gettimeofday(&(newtimer->expiry), NULL); 
//   (newtimer->expiry.tv_sec)+=update_interval;   
//   newtimer -> next = NULL;
//   
//   
//   if(front == NULL)
//      front = rear = newtimer;
//   
//   
//    else if(front->next==NULL)
//           {
//               newtimer->next=front;
//               front=newtimer;
//           }
//  
//   else{
//       
//       while(i->next!=NULL)
//    {   
//            
//          
//             struct timeval t;
//           timersub(&(newtimer->expiry),&i->next->expiry ,&t);
//           if(t.tv_sec<=0)
//           {
//              newtimer->next=i->next;
//              bef_i->next=newtimer;
//           }
//           bef_i=i;
//           i=i->next;
//        }
//      
//       if(i->next==NULL)
//       {
//         rear -> next = newtimer;
//         rear = newtimer;  
//       }
//    }
//   printf("\nInsertion is Success!!!\n");
//}


struct timers* delete_timer()
{
   if(front == NULL)
      printf("\nQueue is Empty!!!\n");
   else{
      struct timers *temp = front;
      front = front -> next;
      printf("\nDeleted element\n");
      return temp;
   }
}

int timeval_subtract (struct timeval *result, struct timeval *x, struct timeval *y)//adopted from linux manual
{
  /* Perform the carry for the later subtraction by updating y. */
  if (x->tv_usec < y->tv_usec) {
    int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
    y->tv_usec -= 1000000 * nsec;
    y->tv_sec += nsec;
  }
  if (x->tv_usec - y->tv_usec > 1000000) {
    int nsec = (x->tv_usec - y->tv_usec) / 1000000;
    y->tv_usec += 1000000 * nsec;
    y->tv_sec -= nsec;
  }

  /* Compute the time remaining to wait.
     tv_usec is certainly positive. */
  result->tv_sec = x->tv_sec - y->tv_sec;
  result->tv_usec = x->tv_usec - y->tv_usec;

  /* Return 1 if result is negative. */
    return x->tv_sec < y->tv_sec;
}
ssize_t send_udp(int sock_index, char *buffer, ssize_t nbytes,struct sockaddr_in *si_other,int slen)
{
    ssize_t bytes = 0;
    bytes = sendto(sock_index, buffer, nbytes, 0,(struct sockaddr*)si_other, slen);

    if(bytes == 0) return -1;
    while(bytes != nbytes)
        bytes += sendto(sock_index, buffer+bytes, nbytes-bytes, 0,(struct sockaddr*)si_other, slen);

    return bytes;
}
void send_update(uint16_t port, uint32_t ip)
{
        struct sockaddr_in si_other;
        int s, slen=sizeof(si_other);
        char *buf = malloc(sizeof(*routing_table));
        //bzero(buf,sizeof(*routing_table));
        memcpy(buf,routing_table , sizeof(*routing_table));
    
        if ((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))==-1)
        perror("socket");
    
        bzero(&si_other, sizeof(&si_other));
        si_other.sin_family = AF_INET;
        si_other.sin_port = htons(port);
        si_other.sin_addr.s_addr =htonl(ip);
    
        
          printf("Sending packet \n");
         // sprintf(buf, "This is packet \n");
        
          
          if (send_udp(s, buf, sizeof(*routing_table),&si_other, slen)<0)
          perror("sendto() error");
       
          

}
 //int rcv_flag=1;
 void process_timeout()
 {
     
     struct timers *timeout_timer = delete_timer();
     if (timeout_timer->id == my_id(init_payload))
     {
         
             for(int x=0; x<tot_neighbours ;x++)
             send_update(neighbour_list[x]->r_port,neighbour_list[x]->ip);
         
         
            
          //send_update(9582,2160927790);
             
             //rcv_flag=1;
         insert_timer(my_id(init_payload));
     }
     else
     {   
         for(int i=0;i<tot_neighbours;i++)
         {
             if(timeout_timer->id==neighbour_list[i]->id)
             {
                 neighbour_list[i]->update_rcv_flag++;
                 if(neighbour_list[i]->update_rcv_flag>=3)
                 {
                     //make cost of this guy inf in my routing table
                 }
             }
         }
         
         
         
         insert_timer(timeout_timer->id);
         
     }
     struct timeval current_time; 
     gettimeofday(&current_time,NULL);
     
     timersub(&(front->expiry),&current_time ,&timeout);
     if(timeout.tv_sec<0)
     {   
         printf("-ve time");
         //timeout.tv_sec=(int)(-(timeout.tv_sec/update_interval));
         timeout.tv_sec=0;
         
         
         
     }
     if(timeout.tv_usec<0)
     {
         printf("-ve u time");
         timeout.tv_usec=0;
         
     }
     
//     if(timeval_subtract (&timeout, &(front->expiry), &current_time))
//     {
//         printf("deadline missed");
//         
//     }
          
 }
 
 