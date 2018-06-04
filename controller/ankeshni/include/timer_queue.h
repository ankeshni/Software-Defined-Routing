/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   timer_queue.h
 * Author: Ankesh N. Bhoi
 *
 * Created on December 1, 2017, 2:59 PM
 */

#ifndef TIMER_QUEUE_H
#define TIMER_QUEUE_H

struct timers
{
 uint16_t id;
 struct timeval expiry;
 struct timers *next;
}*front; *rear;

void timers_init();

void insert_timer(uint16_t id);

struct timers delete_timer();

 void process_timeout();
 

#endif /* TIMER_QUEUE_H */

