#include "../include/simulator.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ******************************************************************
 ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: VERSION 1.1  J.F.Kurose

 This code should be used for PA2, unidirectional data transfer
 protocols (from A to B). Network properties:
 - one way network delay averages five time units (longer if there
 are other messages in the channel for GBN), but can be larger
 - packets can be corrupted (either the header or the data portion)
 or lost, according to user-defined probabilities
 - packets will be delivered in the order in which they were sent
 (although some can be lost).
 **********************************************************************/

/********* STUDENTS WRITE THE NEXT SIX ROUTINES *********/

/* Defining necessary variables */

/* Variable definition for sender and receiver */
#define A 0
#define B 1

typedef enum {
	false, true
} bool;

/* Global variables for A state*/

int A_currentState;
#define A_waitforMessage0  0
#define A_waitforAck0  1
#define A_waitforMessage1  2
#define A_waitforAck1  3

int transitMessageCount =0;

struct pkt pktbuf0;
struct pkt pktbuf1;

/* Global variables for B state*/

int B_currentState;
#define B_waitforMessage0  0
#define B_waitforMessage1  1

/*Timeout value*/
float timeout = 13.0;

/*End of Variable Declaration*/


/*Function create data packet with necessary fields in it*/
void createDataPacket(struct pkt *packet,int seq, int ack, int checksum, char *message)
{

	memset(&packet->payload,0,sizeof(packet->payload)+1); //this adds a null char at the end of payload
	packet->seqnum = seq;
	packet->acknum = ack;
	packet->checksum = checksum;
	memcpy(&packet->payload,message,20); //it doesn't add null at the end

	printf("######creating data packet seqnum %d and acknum %d and message %s \n"
									,packet->seqnum,packet->acknum,packet->payload);

}


/*Function creates checksum using all fields in the packet*/
void createCheckSum(struct pkt *packet)
{
	unsigned int i,sum =0;
	sum += packet->acknum;
	sum += packet->seqnum;
	for(i=0;i<20;i++)
	{
		sum += packet->payload[i];
	}

	packet->checksum = sum;

}


/*Function validates checksum by checking sum of all the fields in packet along with checksum value*/
bool isPacketCorrupt(int checksum, int seqnum,int acknum, char *message)
{

	unsigned int i,sum =0;
		sum += acknum;
		sum += seqnum;
		for(i=0;i<20;i++)
		{
			sum += message[i];
		}

		printf("\nprinting checksummm in validation side %d\n",sum);

		if(sum==checksum)
			return false;
		else
			return true;

}


/* Initial State of Sender SIde*/
void A_First_State(char *message)
{

	if(transitMessageCount == 0) //only one message should be in transit
	{
	struct pkt datapacket;
	int seq =0;
	int checksum=0;
	createDataPacket(&datapacket,seq,0,checksum,message);
	createCheckSum(&datapacket); //modify checksum
	datapacket.seqnum = 0;

	//store packet in "pktbuf0" for re-sending
	memcpy(&pktbuf0,&datapacket,sizeof(datapacket));

	tolayer3(A,datapacket);
	starttimer(A,timeout);

	A_currentState = A_waitforAck0;

	transitMessageCount = 1;
	}
}


void A_Second_State(struct pkt packet)
{
	bool corrupt = isPacketCorrupt(packet.checksum,packet.seqnum,packet.acknum,packet.payload);
		int acknum = packet.acknum;

		if(!corrupt && acknum==0)
		{
			stoptimer(A);
			A_currentState = A_waitforMessage1;
			transitMessageCount = 0;
		}

}



void A_Third_State(char *message)
{

	if(transitMessageCount == 0) //only one message should be in transit
	{
	struct pkt datapacket;
		int seq =1;
		int checksum=0;
		createDataPacket(&datapacket,seq,0,checksum,message);
		createCheckSum(&datapacket); //modify checksum
		datapacket.seqnum = 1;

		//store message in "buffer1" for re-sending
		memcpy(&pktbuf1,&datapacket,sizeof(datapacket));

		A_currentState = A_waitforAck1;
		tolayer3(A,datapacket);
		starttimer(A,timeout);

		transitMessageCount = 1;
	}
}


void A_Fourth_State(struct pkt packet)
{
	bool corrupt = isPacketCorrupt(packet.checksum,packet.seqnum,packet.acknum,packet.payload);
		int acknum = packet.acknum;

		if(!corrupt && acknum==1)
		{
			stoptimer(A);
			A_currentState = A_waitforMessage0;
			transitMessageCount = 0;
		}

}


/*extracting message from packet and storing in a structure. Also assigning 20 bytes
 * of message back to packet payload, as we might have received junk */
void createMessageforLayer5(struct pkt *packet, struct msg *message)
{

	memset(&message->data,0,sizeof(message->data)+1); //this adds a null char at the end of payload
	memcpy(&message->data,&packet->payload,20);

}




void B_First_State(struct pkt packet)
{
	struct msg message;

	createMessageforLayer5(&packet,&message);

	bool corrupt = isPacketCorrupt(packet.checksum,packet.seqnum,packet.acknum,message.data);
	int seqnum = packet.seqnum;

	struct pkt datapacket; //creating packet containing acknowledgment
	int checksum=0;

	if(!corrupt && seqnum==0)
	{
		int ack=0;
		createDataPacket(&datapacket,0,ack,checksum,"");
		createCheckSum(&datapacket); //set checksum
		datapacket.acknum = 0; //re assigning value for correctness

		tolayer3(B,datapacket);
		tolayer5(B,message.data);


		printf("\nprinting packet payload sent above %s\n",message.data);
		printf("\nprinting acknum %d sent from B to A and seqnum %d\n",datapacket.acknum,datapacket.seqnum);

		B_currentState = B_waitforMessage1;
	}

	else if(corrupt || seqnum==1) //corrupt packet or duplicate data, re-send Ack 1
	{
		int ack =1;
		createDataPacket(&datapacket,0,ack,checksum,"");
		createCheckSum(&datapacket); //set checksum
		datapacket.acknum = 1;
		tolayer3(B,datapacket);
	}

}


void B_Second_State(struct pkt packet)
{

	struct msg message;

	createMessageforLayer5(&packet,&message);

	bool corrupt = isPacketCorrupt(packet.checksum,packet.seqnum,packet.acknum,message.data);
	printf("\n2222printing corrupt seq value %d\n",packet.seqnum);

	int seqnum = packet.seqnum;

	struct pkt datapacket; //creating packet containing acknowledgment
	int checksum=0;

	if(!corrupt && seqnum==1)
	{
		int ack=1;
		createDataPacket(&datapacket,0,ack,checksum,"");
		createCheckSum(&datapacket); //modify checksum
		datapacket.acknum = 1; //re assigning value for correctness

		tolayer3(B,datapacket);
		tolayer5(B,message.data);

		printf("\n222printing packet payload sent above %s\n",message.data);
		printf("\n222printing acknum %d sent from B to A and seqnum %d\n",datapacket.acknum,datapacket.seqnum);


		B_currentState = B_waitforMessage0;
	}

	else if(corrupt || seqnum==0) //corrupt packet or duplicate data, re-send Ack 0
	{
		int ack =0;
		createDataPacket(&datapacket,0,ack,checksum,"");
		createCheckSum(&datapacket); //modify checksum
		datapacket.acknum = 0;
		tolayer3(B,datapacket);
	}

}


/* called from layer 5, passed the data to be sent to other side */
void A_output(message)
	struct msg message; {

	if(A_currentState == A_waitforMessage0)
					A_First_State(message.data);

	else if(A_currentState == A_waitforMessage1)
		A_Third_State(message.data);

}


/* called from layer 3, when a packet arrives for layer 4 */
void A_input(packet)
	struct pkt packet; {


	if(A_currentState == A_waitforAck0)
				A_Second_State(packet);

	else if(A_currentState == A_waitforAck1)
				A_Fourth_State(packet);

}


/* called when A's timer goes off */
void A_timerinterrupt() {

	if(A_currentState == A_waitforAck0)
	{
		tolayer3(A,pktbuf0);
		starttimer(A,timeout);
	}

	else if(A_currentState ==  A_waitforAck1)
	{
		tolayer3(A,pktbuf1);
		starttimer(A,timeout);
	}
}

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init() {

	A_currentState = A_waitforMessage0;

}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(packet)
	struct pkt packet; {


		if(B_currentState == B_waitforMessage0)
			B_First_State(packet);

		else if(B_currentState == B_waitforMessage1)
			B_Second_State(packet);

}


/* the following routine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init() {
	B_currentState = B_waitforMessage0;
}
