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

/* Linkedlist for maintaining individual logical timers*/
struct ListNode {
	int seqNum;
	float time;
	struct ListNode *next;
};

struct TimerList {
	struct ListNode *head;
	struct ListNode *tail;
	int size;
};

struct TimerList myTimerList;

/* Global variables for A state*/
int base;
int nextSequenceNum;
int windowSize;
int senderPacketCounter;

struct pkt senderBuffer[2200]; //common storage space for storing all the messages
bool ackReceived[2200]; //maintains Ack status of each packet

/*Global variables for B state*/

int receiverBase;
int receiverWindowSize;
struct msg receiverBuffer[1200]; //buffering message structures to be sent above at receiver
int ackSent[1200] = { 0 }; //takes value "1" if ack sent and value "2" if message sent to Layer5
int receiverMessageCounter;

/*Timeout value*/
float timeout = 26.0;

/*End of Variable Declaration*/

/*Function create data packet with necessary fields in it*/
void createDataPacket(struct pkt *packet, int seq, int ack, int checksum,
		char *message) {

	memset(&packet->payload, 0, sizeof(packet->payload) + 1); //this adds a null char at the end of payload
	packet->seqnum = seq;
	packet->acknum = ack;
	packet->checksum = checksum;
	memcpy(&packet->payload, message, 20); //it doesn't add null at the end

	//printf("######creating data packet seqnum %d\tacknum %d\tand message %s \n",
		//	packet->seqnum, packet->acknum, packet->payload);

}

/*Function creates checksum using all fields in the packet*/
void createCheckSum(struct pkt *packet) {
	unsigned int i, sum = 0;
	sum += packet->acknum;
	sum += packet->seqnum;
	for (i = 0; i < 20; i++) {
		sum += packet->payload[i];
	}

	packet->checksum = sum;

	//printf("\nCHECKSUM printing checksummm in function%d\n", packet->checksum);
}

/*Function validates checksum by checking sum of all the fields in packet along with checksum value*/
bool isPacketCorrupt(int checksum, int seqnum, int acknum, char *message) {

	unsigned int i, sum = 0;
	sum += acknum;
	sum += seqnum;
	for (i = 0; i < 20; i++) {
		sum += message[i];
	}


	if (sum == checksum)
		return false;
	else
		return true;

}

/*Display timerList*/
void displayTimerList()
{
	struct ListNode *curNode;

		curNode = malloc(sizeof(struct ListNode)); // create the node to be traversed
		curNode = myTimerList.head;

		if(curNode ==NULL)
			printf(" List is Empty\n");

		while(curNode !=NULL)
		{
			printf(" List elements seqNum %d, time %f\n",curNode->seqNum,curNode->time);
			curNode = curNode->next;
		}
}


/* Function inserts packets with time values in List*/
void insertLinkedList(int seqNum, float currentTime) {

	struct ListNode *newNode;

	newNode = malloc(sizeof(struct ListNode)); // create the node to be inserted

	newNode->seqNum = seqNum;
	newNode->time = currentTime;

	// linkedlist is empty, inserting as first element
	if (myTimerList.head == NULL ) {
		myTimerList.head = newNode;
		myTimerList.tail = newNode;

		myTimerList.head->next = NULL;

		myTimerList.size++;
		return;
	}

	//Else insert at end
	myTimerList.tail->next = newNode;

	myTimerList.tail = myTimerList.tail->next;
	myTimerList.tail->next = NULL;

	myTimerList.size++;

}

/*Function update elements in TimerList during timeout and starts Timer*/
void updateTimerList() {

	//Make second element as head, find elapsed time, start timer with new timeout value

	struct ListNode *current = myTimerList.head;

	myTimerList.head = myTimerList.head->next;

	free(current);

	if (myTimerList.head != NULL ) {

		float elapsedTime = get_sim_time() - myTimerList.head->time;
		float newTimout = timeout - elapsedTime;
		starttimer(A, newTimout);
	}

	else  //only one element is in list, and there is a timeout. restart timer here
	{
		//printf("One Element Only, Timer started at %f \n",get_sim_time());
		starttimer(A,timeout);
	}



	myTimerList.size--;

}

/* Function return first element of timerList*/
int returnFirstElementOfTimerList() {
	return myTimerList.head->seqNum;
}

/*Function update elements in TimerList during acknowledgment and starts Timer*/
void updateTimerListForAcknowledgement(int SequenceNum) {

	struct ListNode *current, *prev, *temp;
	current = malloc(sizeof(struct ListNode)); // node for traversal
	prev = malloc(sizeof(struct ListNode));

	temp = myTimerList.head;
	prev = myTimerList.head;
	current = myTimerList.head->next;

	//printf("Inside Update Timer with ACKKKKK Function : received SequenceNumber is  %d , current time is %f\n",SequenceNum,get_sim_time());

	//First value is Ack'ed
	if (myTimerList.head->seqNum == SequenceNum) {

		if (current != NULL ) {
			float elapsedTime = get_sim_time() - current->time; //Taking values from next element
			float newTimout = timeout - elapsedTime;

			//printf("Inside Update Timer with ACKKKKK Function : new timeout is  %f\n",newTimout);

			//printf("Stopping timer and starting again at %f\n", newTimout);
			stoptimer(A);
			starttimer(A, newTimout);


			myTimerList.head = current; //update head


		}

		//only one element, stop the timer and remove head
		else
			{
			//printf("One Element Only, Timer stopped at %f \n",get_sim_time());
			stoptimer(A);
			myTimerList.head = current;
			}

		free(temp);

	}

	//remove any other node just like that
	else {
		while (current != NULL ) {

			//if its a tail element, update tail
			if (current->seqNum == SequenceNum
					&& myTimerList.tail->seqNum == current->seqNum) {

				prev->next = NULL;
				myTimerList.tail = prev; //updating tail
				//printf("updated tail is %d  at time %f\n", myTimerList.tail->seqNum,get_sim_time());
				break;
			}

			else if (current->seqNum == SequenceNum) {
				prev->next = current->next;
				//printf("It comes to Else loop too at time %f",get_sim_time());
				break;
			}
			current = current->next;
			prev = prev->next;
		} //END of While Loop

	} //END of Else Loop

}

/* common function for sender and receiver, used for moving window at sender side,
 *  sending packets to layer5 at receiver side*/
int returnNextMaximumContiguousSequenceAcknowledged(int AorB) {
	int returnVal = -1, counter;

	if (AorB == A) {
		int startOfWindow = base;
		int endOfWindow = base + windowSize - 1;

		for (counter = startOfWindow; counter <= endOfWindow; counter++) {
			if (ackReceived[counter])
				returnVal = counter;
			else
				break;
		}

	}

	if (AorB == B) {
		int startOfWindow = receiverBase;
		int endOfWindow = receiverBase + receiverWindowSize - 1;

		for (counter = startOfWindow; counter <= endOfWindow; counter++) {
			if (ackSent[counter] == 1)
				returnVal = counter;
			else
				break;
		}
	}

	/*checks from beginning of window and returns zero if first packet is not yet acknowledged, else
	 * returns value of maximum sequence number acknowledged contiguously*/

	//printf("Maximum contiguous value Returned in 'AorB' %d side : %d\t\n", AorB		returnVal);
	return returnVal;
}

/*Function send packets to layer5, using the value returned by function returnNextMaximumContiguousSequenceAcknowledged()*/
void sendMessagestoLayer5(int tillThisPacket) {
	int startOfWindow = receiverBase;

	while (startOfWindow <= tillThisPacket) {
		if (ackSent[startOfWindow] != 2) {
			tolayer5(B, receiverBuffer[startOfWindow].data);

			//printf("#B SIDE : Printing Packet payload Sent Above %s    with a size of %zu\n",
				//	receiverBuffer[startOfWindow].data,sizeof(receiverBuffer[startOfWindow].data));


			//printf("Previous Receiver Base value is : %d  and New receiver base value is %d\n",receiverBase,receiverBase+1);
			receiverBase++; //every time you send a message, increment receiver base

			ackSent[startOfWindow] = 2;

		}
		startOfWindow++;
	}

}

/*extracting 20 bytes message from packet and storing in a message structure */
void createMessageforLayer5(struct pkt *packet, struct msg *message) {

	memset(&message->data, 0, sizeof(message->data) + 1); //this adds a null char at the end of payload
	memcpy(&message->data, &packet->payload, 20);

	//printf("message value after extracting in B side %s###\n", message->data);

}

/* If received packet is not corrupt, add it into receiver buffer*/
void B_StoreMessageInReceiverBuffer(int seqnum, struct msg *message) {

	if (ackSent[seqnum] == 0) {
		memcpy(&receiverBuffer[seqnum], message, 20);
		receiverMessageCounter++;
		ackSent[seqnum] = 1;     //marking the packet as true
	}
}

/*Initial state of receiver side*/
void B_Initial_State(struct pkt packet) {
	struct msg message;

	createMessageforLayer5(&packet, &message);

	bool corrupt = isPacketCorrupt(packet.checksum, packet.seqnum,
			packet.acknum, message.data);
	int seqnum = packet.seqnum;

	struct pkt datapacket; //creating packet containing acknowledgment
	int checksum = 0;

	if (!corrupt) {

		//send ack for all packets lying under below range
		if (seqnum >= (receiverBase - receiverWindowSize)
				&& seqnum <= (receiverBase + receiverWindowSize - 1)) {

			int ack = seqnum; //send ack for received packet
			createDataPacket(&datapacket, 0, ack, checksum, "");
			createCheckSum(&datapacket); //set checksum
			datapacket.acknum = seqnum; //re assigning value for correctness

			tolayer3(B, datapacket); //this is done irrespective of any action

			//printf("\nB SIDE : Printing Acknum %d sent from B to A \n",
				//	datapacket.acknum);

			B_StoreMessageInReceiverBuffer(seqnum, &message); //marks AckSent as true

		}

		//for values lying under current window
		if (seqnum >= receiverBase
				&& seqnum <= (receiverBase + receiverWindowSize - 1)) {

			/* Handling data sent to layer5*/
			int returnVal = returnNextMaximumContiguousSequenceAcknowledged(B);

			if (returnVal != -1) {
				sendMessagestoLayer5(returnVal);
			}

		}

	} //Un_corrupt packet Loop Ends

}

/*Initial State of Sender Side*/
void A_StorePacketInBuffer(char *message) {
	struct pkt datapacket;
	int checksum = 0;
	createDataPacket(&datapacket, senderPacketCounter, 0, checksum, message);
	createCheckSum(&datapacket); //modify checksum

	datapacket.seqnum = senderPacketCounter;

	//store packet in common senderBuffer for re-sending/sending
	memcpy(&senderBuffer[senderPacketCounter], &datapacket, sizeof(datapacket));

	/*printf(
			"\nPrinting value is senderBuffer, SeqNum : %d\t checksum : %d\t and payload : %s\n",
			senderBuffer[senderPacketCounter].seqnum,
			senderBuffer[senderPacketCounter].checksum,
			senderBuffer[senderPacketCounter].payload);*/

	//increment sender packet counter
	senderPacketCounter++;
}

/*Function sends packet with "Id" nextSeqNumber, Also add event to timer List*/
void A_sendPacket(int sequence) {

	insertLinkedList(sequence, get_sim_time());

	tolayer3(A, senderBuffer[sequence]);

	//printf("Packet sent from A to B Sequence Number : %d \n",sequence);

	if (nextSequenceNum == base) //whenever it starts fresh after receiving all Acks, start the timer
		{
		//printf("TIMER HAS BEEN STARTED at %f\n",get_sim_time());
		//stoptimer(A);
		starttimer(A, timeout);
		}

}

/*Sender State that handles acknowledgment*/
void A_Receiver_State(struct pkt packet) {
	bool corrupt = isPacketCorrupt(packet.checksum, packet.seqnum,
			packet.acknum, packet.payload);
	int acknum = packet.acknum;

	//if Ack lies under current window
	if (!corrupt && acknum >= base && acknum <= (base + windowSize - 1)) {

		ackReceived[acknum] = true;
		int returnVal = returnNextMaximumContiguousSequenceAcknowledged(A);

		//displayTimerList();

		updateTimerListForAcknowledgement(acknum); //update timerlist accordingly

		//For sending all buffered packets which falls within new window
		if (returnVal != -1) {
			base = returnVal + 1; //updating base value

			//printf("Updated Base value is %d \n",base);
			//transmit packets in new window
			int endOfWindow = base + windowSize;

			//printf("Next sequence is %d,  EndofWindow is %d, senderpacketCounter is %d \n",nextSequenceNum,endOfWindow,senderPacketCounter);

			while (nextSequenceNum < endOfWindow
					&& nextSequenceNum < senderPacketCounter) { //As senderpacket becomes 1 after storing 0th value
				A_sendPacket(nextSequenceNum);
				nextSequenceNum++;
			}//END Of While
		}



	}//END If Loop

}

/*Initial Sender State*/
void A_Initial_State() {

	if (nextSequenceNum < (base + windowSize)) {

		A_sendPacket(nextSequenceNum);
		nextSequenceNum++;
	}
}


/* called from layer 5, passed the data to be sent to other side */
void A_output(message)
	struct msg message; {

	//printf("\n******************************* A_OUTPUT Message from above *********************************\n");
	A_StorePacketInBuffer(message.data);

	A_Initial_State();

}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(packet)
	struct pkt packet; {

	A_Receiver_State(packet);

}


/* Function called when A's timer goes off */
void A_timerinterrupt() {
	//displayTimerList();

	int SeqtobeResent = returnFirstElementOfTimerList();

	//printf("Sequence Num to be resent %d \n",SeqtobeResent);

	updateTimerList(); //removes head and starts timer of next element with updated timeout

	A_sendPacket(SeqtobeResent); //resends that packet, add it to the list

	//displayTimerList();

}


/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init() {

	base = 0;
	nextSequenceNum = 0;
	senderPacketCounter = 0;
	windowSize = getwinsize();

	int i;
	for (i = 0; i < 1200; i++)
		ackReceived[i] = false;

	myTimerList.head = NULL;
	myTimerList.tail = NULL;
	myTimerList.size = 0;

}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(packet)
	struct pkt packet; {

	B_Initial_State(packet);
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init() {

	receiverBase = 0;
	receiverMessageCounter = 0;
	receiverWindowSize = getwinsize();

}
