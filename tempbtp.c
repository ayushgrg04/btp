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
#include <sys/clock.h>
#include "dev/button-sensor.h"

#include "dev/leds.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
/*---------------------------------------------------------------------------*/
PROCESS(example_broadcast_process, "Broadcast example");
PROCESS(example_unicast_process, "Example unicast"); // Process for sending a unicast Message
PROCESS(modified_ptp, "Modified PTP");
AUTOSTART_PROCESSES(&example_broadcast_process, &example_unicast_process, &modified_ptp);
/*---------------------------------------------------------------------------*/

int label = 1;


struct SDreqPacket{ //You can put this structure declaration in an archive called example-uni-temp.h
 
  int msgtype; //message type is 1
  int label; 
  linkaddr_t addr;
};

struct dresPacket{ //You can put this structure declaration in an archive called example-uni-temp.h
 
  int msgtype; //message type is 3
  int label; 
  long long ctime;  // value of x ().
 
};


struct tempsendingPacket{ //You can put this structure declaration in an archive called example-uni-temp.h
  linkaddr_t addr;
  struct dresPacket pkt;
  
};
 

clock_time_t start_time;
struct SDreqPacket synMsg;
unsigned long sync_sending_time;
unsigned long startingtime;
struct dresPacket dresmsg;
struct tempsendingPacket tsmsg;
int tt1=0;
struct SDreqPacket msg1;


static void
broadcast_recv(struct broadcast_conn *c, const linkaddr_t *from)
{
    
    unsigned long dreq_receiving_time = (unsigned long)clock_seconds();
    printf("receiving time of dreq at master: %lu\n", dreq_receiving_time);

    char *tempstr = (char *)packetbuf_dataptr();
    printf("string is %s", tempstr);
  struct SDreqPacket *tmpMsg = (struct SDreqPacket *)malloc(sizeof(struct SDreqPacket));
  tmpMsg->msgtype = atoi(strtok(tempstr, "#"));
  tmpMsg->label = atoi(strtok(NULL, "#"));
  if(tmpMsg->msgtype==2){
    (tmpMsg->addr).u8[0] = atoi(strtok(NULL, "#"));
    (tmpMsg->addr).u8[1] = atoi(strtok(NULL, "#"));   
  }
    

  printf("broadcast dreqPacket received from %d.%d: '%d' '%d' \n",
         from->u8[0], from->u8[1], tmpMsg->msgtype, tmpMsg->label);

  if(tmpMsg->msgtype == 2 && tmpMsg->label == label+1 && linkaddr_cmp(&(tmpMsg->addr), &linkaddr_node_addr)){
      printf("case 1\n");
      dresmsg.msgtype = 3; // sending Dresponse SDreqPacket after every 50 sec.
      dresmsg.label = label; // To save the temperature in the struct envir
      dresmsg.ctime = -((long long)sync_sending_time+(long long)dreq_receiving_time);
      tsmsg.pkt = dresmsg;
      tsmsg.addr = *from;
       printf("message details case 1:'%d' '%d' \n",
          dresmsg.msgtype, dresmsg.label);
          printf("message details case 1 tsmsg: '%d' '%d' '%lld' %d.%d\n",
          (tsmsg.pkt).msgtype, (tsmsg.pkt).label, (tsmsg.pkt).ctime, (tsmsg.addr).u8[0], (tsmsg.addr).u8[1]);
      process_post(&example_unicast_process, PROCESS_EVENT_CONTINUE , &(tsmsg) );

      // printf("starting time of label 1: %lu \n", startingtime);
  }
  else{
    printf("msg type not matched\n");
  }

}

static void
recv_uc(struct unicast_conn *c, const linkaddr_t *from)
{
  printf("unicast SDreqPacket received from %d.%d\n",
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
  // clock_init();
  if(tt1==0){
    clock_set_seconds(500);
    tt1++;  
  }
    
  startingtime = (unsigned long)clock_seconds();
  printf("starting time of master clock: %lu \n", startingtime);
  printf("node rime address: %d.%d", linkaddr_node_addr.u8[0], linkaddr_node_addr.u8[1]);
  static struct etimer et; // Struct used for the timer
  start_time = clock_seconds();

  PROCESS_BEGIN();  // Says where the process starts 
   
  while(1){
   
  etimer_set(&et, CLOCK_SECOND * 50); // Configure timer to expire in 40 seconds
  
  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et)); // Wait until timer expires 
  
  printf("Sending sync SDreqPacket\n"); // Print the string "Data"
  
  synMsg.msgtype = 1; // sending syn SDreqPacket after every 40 sec.
  synMsg.label = label; // with label = node label

  process_post(&example_broadcast_process, PROCESS_EVENT_CONTINUE , &(synMsg) ); // This function posts an asynchronous event to the process example_unicast_process with the information of the structure called envir
  
  etimer_reset(&et); // Reset timer
 
  
  }
   
  PROCESS_END();//Says where the process ends
 
}






PROCESS_THREAD(example_unicast_process, ev, data) // Process for sending a unicast SDreqPacket
{
  PROCESS_EXITHANDLER(unicast_close(&uc);)//Specify an action when a process exits. 
     
  PROCESS_BEGIN();  // Says where the process starts
 
  unicast_open(&uc, 146, &unicast_callbacks);  //Opens a unicast connection

  while(1) {
 
    // linkaddr_t addr; //Declares the addr variable
     
    PROCESS_WAIT_EVENT(); //Wait for an event to be posted to the process. 
 
    struct tempsendingPacket *tsMsg =  data; //Saves the information that comes from the other process (read_temperature_light) into a structure pointer called *envirRX
 
  struct dresPacket *tmpMsg = &(tsMsg->pkt);
    printf("Data\t"); // Print the string "Data"
    printf("%d\t", tmpMsg->msgtype );  // Print the sequence number
    printf("%d\t", tmpMsg->label ); // Print the temperature value
    
    // struct dresPacket *tmpMsg = data;
    // packetbuf_copyfrom(  tmpMsg , sizeof(  (*tmpMsg)  ) ); 
 
    char str[50];
    sprintf(str, "%d#%d#%lld", tmpMsg->msgtype, tmpMsg->label, tmpMsg->ctime);
    // struct dresPacket *tmpMsg = data;
    // packetbuf_copyfrom(  tmpMsg , sizeof(  (*tmpMsg)  ) ); 
  printf("str %s", str);
  packetbuf_copyfrom(str, 50);

    // addr.u8[0] = 2; //This is the sink's address
    // addr.u8[1] = 0; //This is the sink's address
    if(!linkaddr_cmp(&(tsMsg->addr), &linkaddr_node_addr)) { //if the address is diferent from the current's node
      unicast_send(&uc, &(tsMsg->addr)); //Send a unicast SDreqPacket to the sink
    }
 
  }
 
  PROCESS_END();  //Says where the process ends
}


PROCESS_THREAD(example_broadcast_process, ev, data) // Process for sending a unicast SDreqPacket
{
  PROCESS_EXITHANDLER(broadcast_close(&broadcast);)
 
  PROCESS_BEGIN();

  broadcast_open(&broadcast, 129, &broadcast_call);
  
   while(1) {

    /* Delay 2-4 seconds */
   PROCESS_WAIT_EVENT(); //Wait for an event to be posted to the process. 
 
    struct SDreqPacket *tmpMsg =  data; //Saves the information that comes from the other process (read_temperature_light) into a structure pointer called *envirRX
 
 
    printf("Data\t"); // Print the string "Data"
    printf("SDreqPacket type %d\t", tmpMsg->msgtype );  // Print the msgtype number
    printf("label %d\t", tmpMsg->label ); // Print the label value
    if(tmpMsg->msgtype==1){
      char str[15];
      sprintf(str, "%d#%d", tmpMsg->msgtype, tmpMsg->label);
      printf("str %s", str);
      packetbuf_copyfrom(str, 15);
    }
    else{
      char str[50];
      sprintf(str, "%d#%d#%d#%d", tmpMsg->msgtype, tmpMsg->label, (tmpMsg->addr).u8[0], (tmpMsg->addr).u8[1]);
      printf("str %s", str);
      packetbuf_copyfrom(str, 50);
    }
    printf("Clock Time Event Recorded\n");
    // rtimer_set(&rt, RTIMER_NOW()+RTIMER_ARCH_SECOND,1,myfunctn,tmpMsg);
    // packetbuf_copyfrom(  tmpMsg , sizeof(  (*tmpMsg)  ) ); 
    //   // packetbuf_copyfrom("Hello", 6);    
    broadcast_send(&broadcast);
    sync_sending_time = (unsigned long)clock_seconds();
    printf("broadcast SDreqPacket sent\n");
    printf("syncsending Time is: %lu \n", sync_sending_time);
    
    

   }

  PROCESS_END();

}


/*---------------------------------------------------------------------------*/