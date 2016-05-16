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

/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/

/* Variable definition for sender and receiver */
#define A 0
#define B 1

typedef enum {
	false, true
} bool;

/* Global variables for A state*/
int base;
int nextSequenceNum;
int windowSize;
int senderPacketCounter;

struct pkt senderBuffer[2500]; //common storage space for storing all the messages


/* Global variables for B state*/
int expectedSeqNum;

/*Timeout value*/
float timeout = 12;

/*End of Variable Declaration*/

/*Function create data packet with necessary fields in it*/
void createDataPacket(struct pkt *packet,int seq, int ack, int checksum, char *message)
{

	memset(&packet->payload,0,sizeof(packet->payload)+1); //this adds a null char at the end of payload
	packet->seqnum = seq;
	packet->acknum = ack;
	packet->checksum = checksum;
	memcpy(&packet->payload,message,20); //it doesn't add null at the end

	//printf("######creating data packet seqnum %d\tacknum %d\tand message %s \n"
		//							,packet->seqnum,packet->acknum,packet->payload);

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

	//printf("\nCHECKSUM printing checksummm in function%d\n",packet->checksum);

	//printf("\nprinting payload in function DUDEDEDEDEDEDDE %s\n",packet->payload);

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

		//printf("\nCHECKSUM printing checksum in VALIDATIONVALIDATION function%d  %d\n",sum,checksum);

		if(sum==checksum)
			return false;
		else
			return true;

}


/*extracting 20 bytes message from packet and storing in a message structure */
void createMessageforLayer5(struct pkt *packet, struct msg *message)
{

	memset(&message->data,0,sizeof(message->data)+1); //this adds a null char at the end of payload
	memcpy(&message->data,&packet->payload,20);

	//printf("message value after extracting in B side %s###\n",message->data);

}


/*Initial state of receiver side*/
void B_Initial_State(struct pkt packet)
{
	struct msg message;

	createMessageforLayer5(&packet,&message);

	bool corrupt = isPacketCorrupt(packet.checksum,packet.seqnum,packet.acknum,message.data);
	int seqnum = packet.seqnum;

	struct pkt datapacket; //creating packet containing acknowledgment
	int checksum=0;

	if(!corrupt && seqnum==expectedSeqNum)
	{
		int ack=expectedSeqNum; //send ack for received packet
		createDataPacket(&datapacket,0,ack,checksum,"");
		createCheckSum(&datapacket); //set checksum
		datapacket.acknum = expectedSeqNum; //re assigning value for correctness

		tolayer3(B,datapacket);
		tolayer5(B,message.data);

		int i;
					for(i=0;i<20;i++)
					{
						printf("%c",message.data[i]);
					}

					printf("\n");
		
		
		//printf("\nB SIDE : Printing Packet payload Sent Above %s\n",message.data);
		//printf("\nB SIDE : Printing Acknum %d sent from B to A \n",datapacket.acknum);

		//incrementing the expected sequence number
		expectedSeqNum++;
	}

	else //send previous acknowledgment
	{
				int ack =expectedSeqNum-1;
				createDataPacket(&datapacket,0,ack,checksum,"");
				createCheckSum(&datapacket); //set checksum
				datapacket.acknum = expectedSeqNum-1; //reassign for correctness
				tolayer3(B,datapacket);
	}
}


/* Initial State of Sender SIde*/
void A_StorePacketInBuffer(char *message)
{
	struct pkt datapacket;
	int checksum=0;
	createDataPacket(&datapacket,senderPacketCounter,0,checksum,message);
	createCheckSum(&datapacket); //modify checksum

	datapacket.seqnum = senderPacketCounter;

	//store packet in common senderBuffer for re-sending/sending
	memcpy(&senderBuffer[senderPacketCounter],&datapacket,sizeof(datapacket));

	/*printf("\nPrinting value is senderBuffer, SeqNum : %d\t checksum : %d\t and payload : %s\n"
					,senderBuffer[senderPacketCounter].seqnum
					,senderBuffer[senderPacketCounter].checksum
					,senderBuffer[senderPacketCounter].payload);
	*/
	//increment sender packet counter
	senderPacketCounter++;
}


/*Function sends packets in between the window base and (nextSeqNum-1)
 * It is called during timeout of oldest unAcknowledged packet*/
void reSendAllIntermediatePackets()
{
	int i;
	int tempBase =base;
	int msgCount = nextSequenceNum-base-1; //number of packets to be sent
	//printf("msgcount value %d\n",msgCount);
	for(i=0;i<msgCount;i++)
	{
		//printf("tempBase value %d\n",tempBase);
		tolayer3(A,senderBuffer[tempBase]);
		//printf("^^^^^^^^^^^^^^^^^^^^^ RESENT MESSAGES %s\n",senderBuffer[tempBase].payload);
		tempBase++;
	}
}

/*Sender State that handles acknowledgment*/
void A_Receiver_State(struct pkt packet)
{
	bool corrupt = isPacketCorrupt(packet.checksum,packet.seqnum,packet.acknum,packet.payload);
		int acknum = packet.acknum;

		if(!corrupt)
		{
			base = acknum+1;

			int endOfWindow = base +windowSize;
			//send all packets from nextseq to end of new windowsize which are in buffer
			while(nextSequenceNum<endOfWindow && nextSequenceNum<senderPacketCounter)
			{
					tolayer3(A,senderBuffer[nextSequenceNum]);
					nextSequenceNum++;
			}

			if(base==nextSequenceNum) //All acknowledgements have been have been received
			{
			stoptimer(A);
			}

			//one ack have been received, thus send one packet from new window
			else
			{
			stoptimer(A);
			starttimer(A,timeout);
			}
		}
}


/*Initial Sender State*/
void A_Initial_State()
{
	//printf("before if loop nextseq %d, base+window %d\n",nextSequenceNum,(base+windowSize));
		if(nextSequenceNum<(base+windowSize))
		{
			//printf("Entered if loop\n");
			tolayer3(A,senderBuffer[nextSequenceNum]);
			if(base==nextSequenceNum)
				starttimer(A,timeout);
			nextSequenceNum++;
		}

}



/* called from layer 5, passed the data to be sent to other side */
void A_output(message)
  struct msg message;
{
	//keep adding every message into a senderBuffer packet strcuture for future transmission
	//printf("\n******************************* A_OUTPUT Message from above *********************************\n");
	A_StorePacketInBuffer(message.data);

	A_Initial_State();

}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(packet)
  struct pkt packet;
{
	A_Receiver_State(packet);

}

/* called when A's timer goes off */
void A_timerinterrupt()
{

	//printf("$$$$$$$$$$$$TIMEOUT$$$$$$$$$$$$$$$$$\n");
	starttimer(A,timeout);
	reSendAllIntermediatePackets();

}  

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
	base = 0;
	nextSequenceNum = 0;
	senderPacketCounter =0;
	windowSize = getwinsize();
	//need to check if malloc required for struct array variables.
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */


/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(packet)
  struct pkt packet;
{
	B_Initial_State(packet);
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
	expectedSeqNum = 0;
}
