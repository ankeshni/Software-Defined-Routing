/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "../include/global.h"
#include "../include/connection_manager.h"
#include "timer_queue.h"
#include "control_handler.h"
#include "connection_manager.h"


void init_timers()
{
    front=NULL;
    rear= NULL;
    insert_timer(my_id())
    
}
void insert_timer(uint16_t id)
{
   struct timers *newtimer;
   newtimer = (struct timers*)malloc(sizeof(struct timers));
   newtimer->id = id;
    gettimeofday(&(newtimer->expiry), NULL); 
   (newtimer->expiry->tv_sec)+=update_interval;   
   newtimer -> next = NULL;
   if(front == NULL)
      front = rear = newtimer;
   else{
      rear -> next = newtimer;
      rear = newtimer;
   }
   printf("\nInsertion is Success!!!\n");
}
struct timers delete_timer()
{
   if(front == NULL)
      printf("\nQueue is Empty!!!\n");
   else{
      struct timers *temp = front;
      front = front -> next;
      printf("\nDeleted element: %d\n", temp->data);
      return temp;
   }
}

void timeval_subtract (struct timeval *result, struct timeval *x, struct timeval *y)
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
 
 void process_timeout()
 {
     struct timers *timeout_timer = delete_timer();
     if (timeout_timer->id == my_id())
     {
         printf("time to send routing update");//send routing update
         
         insert_timer(my_id());
     }
     else
     {
         //increment neighbour timeout flag
         insert_timer(timeout_timer->id);
     }
     struct timeval current_time;
     gettimeofday(&current_time);
     
     if(timeval_subtract (&timeout, &(front->expiry), &current_time))
     {
         printf("deadline missed");
         
     }
          
 }
 
 