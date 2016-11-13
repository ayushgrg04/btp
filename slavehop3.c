
 /* Copyright (c) 2007, Swedish Institute of Computer Science.
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
/*---------------------------------------------------------------------------*/
PROCESS(example_broadcast_process, "Broadcast example");
PROCESS(example_unicast_process, "Example unicast"); // Process for sending a unicast message
PROCESS(modified_ptp, "Modified PTP");
AUTOSTART_PROCESSES(&example_broadcast_process, &example_unicast_process, &modified_ptp);
/*---------------------------------------------------------------------------*/

int label  = 3;


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

struct bufer{
  unsigned long rtime;
  linkaddr_t addr;
};
 

// clock_time_t start_time;

struct SDreqPacket synMsg;
unsigned long syn_rec_time;
unsigned long dreq_sending_time;
int isdreqforsyncrecv = 0;
int is_synchronised = 0;
long long x;
int islistempty=0;
struct bufer bfr;
struct SDreqPacket dresMsg;
struct dresPacket send_dres;
struct tempsendingPacket tsmsg;
struct SDreqPacket msg1;
long long offset;
unsigned long startingtime;
int tt1=0;
struct bufer bufferarray[50];

static void
broadcast_recv(struct broadcast_conn *c, const linkaddr_t *from)
{
  
  unsigned long rec_time = (unsigned long)clock_seconds();
  printf("receiving time for dreq at slave: %lu \n", rec_time);
  struct SDreqPacket *tmpMsg = (struct SDreqPacket*)packetbuf_dataptr();
  printf("broadcast message received from %d.%d: '%d' '%d' \n",
         from->u8[0], from->u8[1], tmpMsg->msgtype, tmpMsg->label);

  if(isdreqforsyncrecv==0 && tmpMsg->msgtype==1 && tmpMsg->label==label-1){    // for receiving sync message.
    printf("case 1\n");       // when sync message is received from master
    isdreqforsyncrecv=1;
    syn_rec_time = rec_time;
    dresMsg.msgtype = 2; // sending syn message after every 15 sec.
    dresMsg.label = label; // with label = node LIABLE
    dresMsg.addr = *(from);
    printf("message details case 1: %d.%d: '%d' '%d' \n",
         (dresMsg.addr).u8[0], (dresMsg.addr).u8[1], dresMsg.msgtype, dresMsg.label);
    process_post(&example_broadcast_process, PROCESS_EVENT_CONTINUE , &(dresMsg) );  //sending dreq msg.
  }
  // else if(is_synchronised==0 && tmpMsg->msgtype==3){ // for receiving dres message
  //     is_synchronised=1;
  //     //set clock time after receiving dresponse.
  //     printf("synchronising clock time after receiving dresponse.\n");

  // }
  else if(linkaddr_cmp(&(tmpMsg->addr), &linkaddr_node_addr) && tmpMsg->msgtype==2 && tmpMsg->label==label+1){  //receiving dreq from next hop
      if(is_synchronised==1){
          printf("case 2\n");       //when dreq is received from next hop and is already syncronised
          
          send_dres.msgtype = 3;
          send_dres.label = label;
          send_dres.ctime = x + ((long long)syn_rec_time-(long long)rec_time);
          tsmsg.pkt = send_dres;
          tsmsg.addr = *from;
          process_post(&example_unicast_process, PROCESS_EVENT_CONTINUE , &(tsmsg) );
          
  //         msg1.msgtype = 0; // sending syn message after every 50 sec.
  // process_post(&example_broadcast_process, PROCESS_EVENT_CONTINUE , &(msg1) );
      }
      else{
          printf("case 3\n");
          islistempty++;
          bufferarray[islistempty].rtime = rec_time;
          bufferarray[islistempty].addr = *from;
 // msg1.msgtype = 0; // sending syn message after every 50 sec.
 //  process_post(&example_broadcast_process, PROCESS_EVENT_CONTINUE , &(msg1) );
      }
  }
  else if(isdreqforsyncrecv==0 && tmpMsg->msgtype==2 && tmpMsg->label==label-1){    // for receiving dreq message from prev label.
    printf("case 4\n");
    isdreqforsyncrecv=1;
    syn_rec_time = rec_time;
    dresMsg.msgtype = 2; // sending syn message after every 15 sec.
    dresMsg.label = label; // with label = node LIABLE
    dresMsg.addr = *(from);
    printf("message details case 1: %d.%d: '%d' '%d' \n",
         (dresMsg.addr).u8[0], (dresMsg.addr).u8[1], dresMsg.msgtype, dresMsg.label);
    process_post(&example_broadcast_process, PROCESS_EVENT_CONTINUE , &(dresMsg) );  //sending dreq msg.
  }
  else{
    printf("no type matched\n");
  }







  // msg2.msgtype = 3; // sending Dresponse message after every 50 sec.
  // msg2.label = 1; // To save the temperature in the struct envir
  // process_post(&example_unicast_process, PROCESS_EVENT_CONTINUE , &(msg2) );


}

static void
recv_uc(struct unicast_conn *c, const linkaddr_t *from)
{
  
  struct dresPacket *tmpMsg = (struct dresPacket*)packetbuf_dataptr();
  printf("unicast message received from %d.%d '%d' '%d' '%lld'\n",
    from->u8[0], from->u8[1], tmpMsg->msgtype, tmpMsg->label, tmpMsg->ctime );
  x = tmpMsg->ctime;
  offset = (x + (long long)syn_rec_time + (long long)dreq_sending_time)/2;
  printf("offset: %lld , x: %lld, starting time: %lu\n", offset, x, startingtime);
  is_synchronised=1;

  while(islistempty!=0){
    // struct dresPacket send_dres;
    // struct tempsendingPacket tsmsg;
    send_dres.msgtype = 3;
    send_dres.label = label;
    send_dres.ctime = x + ((long long)syn_rec_time-(long long)(bufferarray[islistempty].rtime));
    tsmsg.pkt = send_dres;
    tsmsg.addr = bufferarray[islistempty].addr;
    process_post(&example_unicast_process, PROCESS_EVENT_CONTINUE , &(tsmsg) );
    islistempty--;
  }

  // msg1.msgtype = 0; // sending syn message after every 50 sec.
 
  // process_post(&example_broadcast_process, PROCESS_EVENT_CONTINUE , &(msg1) );
}






static const struct broadcast_callbacks broadcast_call = {broadcast_recv};
static struct broadcast_conn broadcast;
static const struct unicast_callbacks unicast_callbacks = {recv_uc};  //Every time a packet arrives the function recv_uc is called.
static struct unicast_conn uc; 
/*---------------------------------------------------------------------------*/


PROCESS_THREAD(modified_ptp, ev, data)  // Process for reading the temperature and light values
{
  // clock_init();
  if(tt1==0){
    clock_set_seconds(5000);
    tt1++;  
  } 
  startingtime = (unsigned long)clock_seconds();
  printf("starting time of slave clock: %lu \n", startingtime);
  printf("node rime address: %d.%d", linkaddr_node_addr.u8[0], linkaddr_node_addr.u8[1]);
  static struct etimer et; // Struct used for the timer
  // start_time = clock_seconds();

  PROCESS_BEGIN();  // Says where the process starts 
   
  while(1){
   
  etimer_set(&et, CLOCK_SECOND * 40); // Configure timer to expire in 5 seconds
  
  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et)); // Wait until timer expires 
  
  is_synchronised=0;
  isdreqforsyncrecv=0;
  printf("Data\t"); // Print the string "Data"
  
  msg1.msgtype = 0; // sending syn message after every 50 sec.
 
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
 
    // linkaddr_t addr; //Declares the addr variable
     
    PROCESS_WAIT_EVENT(); //Wait for an event to be posted to the process. 
 
    struct tempsendingPacket *tsMsg =  data; //Saves the information that comes from the other process (read_temperature_light) into a structure pointer called *envirRX
 
 
   struct dresPacket *tmpMsg = &(tsMsg->pkt);
    printf("Data\t"); // Print the string "Data"
    printf("%d\t", tmpMsg->msgtype );  // Print the sequence number
    printf("%d\t", tmpMsg->label ); // Print the temperature value
    
    packetbuf_copyfrom(  tmpMsg , sizeof(  (*tmpMsg)  ) ); 
 
    // addr.u8[0] = 2; //This is the sink's address
    // addr.u8[1] = 0; //This is the sink's address
    if(!linkaddr_cmp(&(tsMsg->addr), &linkaddr_node_addr)) { //if the address is diferent from the current's node
      unicast_send(&uc, &(tsMsg->addr)); //Send a unicast SDreqPacket to the sink
    printf("unicast message sent\n");
    }
 
  }
 
  PROCESS_END();  //Says where the process ends
}


PROCESS_THREAD(example_broadcast_process, ev, data) // Process for sending a unicast message
{
  PROCESS_EXITHANDLER(broadcast_close(&broadcast);)
 
  PROCESS_BEGIN();

  broadcast_open(&broadcast, 129, &broadcast_call);
  
   while(1) {

    /* Delay 2-4 seconds */
   PROCESS_WAIT_EVENT(); //Wait for an event to be posted to the process. 
 
    struct SDreqPacket *tmpMsg =  data; //Saves the information that comes from the other process (read_temperature_light) into a structure pointer called *envirRX
 
 
   
    if(tmpMsg->msgtype !=0 ){
      printf("Data\t"); // Print the string "Data"
      printf("message type %d\t", tmpMsg->msgtype );  // Print the msgtype number
      printf("label %d\t", tmpMsg->label ); // Print the label value
      packetbuf_copyfrom(  tmpMsg , sizeof(  (*tmpMsg)  ) ); 
        // packetbuf_copyfrom("Hello", 6);
       
        printf("Clock Time Event Recorded\n");
        broadcast_send(&broadcast);
        dreq_sending_time = (unsigned long)clock_seconds();
        printf("broadcast message sent\n");
        printf("dreq sending Time at slave is: %lu \n", dreq_sending_time);
    }
    

   }

  PROCESS_END();

}


/*---------------------------------------------------------------------------*/