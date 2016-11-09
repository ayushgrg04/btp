/*
 * Copyright (c) 2007, Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 */

/**
 * \file
 *         Testing the broadcast layer in Rime
 * \author
 *         Adam Dunkels <adam@sics.se>
 */

#include "contiki.h"
#include "net/rime/rime.h"
#include "random.h"

#include "dev/button-sensor.h"

#include "dev/leds.h"

#include <stdio.h>
/*---------------------------------------------------------------------------*/
PROCESS(example_broadcast_process, "Broadcast example");
PROCESS(example_unicast_process, "Example unicast"); // Process for sending a unicast message
PROCESS(modified_ptp, "Modified PTP");
AUTOSTART_PROCESSES(&example_broadcast_process, &example_unicast_process, &modified_ptp);
/*---------------------------------------------------------------------------*/


struct message{ //You can put this structure declaration in an archive called example-uni-temp.h
 
  int msgtype; //To save the sequence number
  int label; // To save the temperature value
    // To save the light value
  unsigned long ctime;
 
};
 

unsigned long sstime;
clock_time_t start_time;
struct message msg1;
struct message msg2;
int label = 1;
int sync_sending_time;

static void
broadcast_recv(struct broadcast_conn *c, const linkaddr_t *from)
{
  struct message *mseg = (struct message*)packetbuf_dataptr();
  printf("broadcast message received from %d.%d: '%d' '%d' \n",
         from->u8[0], from->u8[1], mseg->msgtype, mseg->label);

  if(mseg->msgtype == 2 && mseg->label == label+1){
    msg2.msgtype = 3; // sending Dresponse message after every 50 sec.
    msg2.label = label; // To save the temperature in the struct envir
    process_post(&example_unicast_process, PROCESS_EVENT_CONTINUE , &(msg2) );
  }

}

static void
recv_uc(struct unicast_conn *c, const linkaddr_t *from)
{
  printf("unicast message received from %d.%d\n",
   from->u8[0], from->u8[1]);
}






static const struct broadcast_callbacks broadcast_call = {broadcast_recv};
static struct broadcast_conn broadcast;
static const struct unicast_callbacks unicast_callbacks = {recv_uc};  //Every time a packet arrives the function recv_uc is called.
static struct unicast_conn uc; 
struct rtimer rt;
/*---------------------------------------------------------------------------*/


PROCESS_THREAD(modified_ptp, ev, data)  // Process for reading the temperature and light values
{
  static struct etimer et; // Struct used for the timer
  start_time = clock_time();

  PROCESS_BEGIN();  // Says where the process starts 
   
  while(1){
   
  etimer_set(&et, CLOCK_SECOND * 35); // Configure timer to expire in 5 seconds
  
  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et)); // Wait until timer expires 
  
  printf("Sending sync message\n"); // Print the string "Data"
  
  msg1.msgtype = 1; // sending syn message after every 15 sec.
  msg1.label = label; // with label = node label

  process_post(&example_broadcast_process, PROCESS_EVENT_CONTINUE , &(msg1) ); // This function posts an asynchronous event to the process example_unicast_process with the information of the structure called envir
  
  etimer_reset(&et); // Reset timer
 
  
  }
   
  PROCESS_END();//Says where the process ends
 
}






PROCESS_THREAD(example_unicast_process, ev, data) // Process for sending a unicast message
{
  PROCESS_EXITHANDLER(unicast_close(&uc);)//Specify an action when a process exits. 
     
  PROCESS_BEGIN();  // Says where the process starts
 
  unicast_open(&uc, 146, &unicast_callbacks);  //Opens a unicast connection
 
  while(1) {
 
    linkaddr_t addr; //Declares the addr variable
     
    PROCESS_WAIT_EVENT(); //Wait for an event to be posted to the process. 
 
    struct message *mseg =  data; //Saves the information that comes from the other process (read_temperature_light) into a structure pointer called *envirRX
 
 
    printf("Data\t"); // Print the string "Data"
    printf("%d\t", mseg->msgtype );  // Print the sequence number
    printf("%d\t", mseg->label ); // Print the temperature value
    
    packetbuf_copyfrom(  mseg , sizeof(  (*mseg)  ) ); 
 
    addr.u8[0] = 2; //This is the sink's address
    addr.u8[1] = 0; //This is the sink's address
    if(!linkaddr_cmp(&addr, &linkaddr_node_addr)) { //if the address is diferent from the current's node
      unicast_send(&uc, &addr); //Send a unicast message to the sink
    }
 
  }
 
  PROCESS_END();  //Says where the process ends
}

 static void myfunctn(struct rtimer *rt, struct message *mseg) {
    // mseg->rtime = rt;
    // packetbuf_copyfrom(  mseg , sizeof(  (*mseg)  ) ); 
    //   // packetbuf_copyfrom("Hello", 6);
    //   broadcast_send(&broadcast);
    //   printf("broadcast message sent\n");
    
  }


PROCESS_THREAD(example_broadcast_process, ev, data) // Process for sending a unicast message
{
  PROCESS_EXITHANDLER(broadcast_close(&broadcast);)
 
  PROCESS_BEGIN();

  broadcast_open(&broadcast, 129, &broadcast_call);
  
   while(1) {

    /* Delay 2-4 seconds */
   PROCESS_WAIT_EVENT(); //Wait for an event to be posted to the process. 
 
    struct message *mseg =  data; //Saves the information that comes from the other process (read_temperature_light) into a structure pointer called *envirRX
 
 
    printf("Data\t"); // Print the string "Data"
    printf("message type %d\t", mseg->msgtype );  // Print the msgtype number
    printf("label %d\t", mseg->label ); // Print the label value
    
    printf("Real Time Event Recorded\n");
    // rtimer_set(&rt, RTIMER_NOW()+RTIMER_ARCH_SECOND,1,myfunctn,mseg);
    mseg->ctime = (unsigned long)clock_time();
    packetbuf_copyfrom(  mseg , sizeof(  (*mseg)  ) ); 
    //   // packetbuf_copyfrom("Hello", 6);
      broadcast_send(&broadcast);
      printf("broadcast message sent\n");
       printf("clock Time is: %lu \n", mseg->ctime);
    
    // unsigned short tt = 10;
    // printf("Real Time is: %u \n", tt); 
    

   }

  PROCESS_END();

}


/*---------------------------------------------------------------------------*/