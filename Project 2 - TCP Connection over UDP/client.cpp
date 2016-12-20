#include <string>
#include <thread>
#include <iostream>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sstream>
#include <fstream>
#include <vector>
#include <unordered_map>
#include "HeaderConverter.h"
#include "TCP.h"
using namespace std;

const int MSS = 1024;
bool wrapAround = false;

struct PacketRec{
	int seq;
	int nextSeqNum;
	vector<uint8_t> packet;
};
	
void outputReceiveMessage(int sequenceNumber);	
void outputSendMessage(int ACKNumber, bool retransmission, bool syn, bool fin, bool fin_ack);


int arrayLocation(int sequenceNumber);

int sequenceNum(int arrayLocation);

bool isGreater(int expectedSeq, int seq, int seqInArr);

vector <PacketRec> packetsReceived;
int offset;

int generateSequenceNumber();
int startConnection(int &sequenceNumber,  int sockfd, struct sockaddr* serverAddr, int serverAddrLen);

fd_set readFds;
fd_set writeFds;

int sequenceNumberToSend = 1;
int ackNumberToSend = 1;

int expectedSeq = 1;




int main(int argc, char *argv[])
{
	// create a socket using UDP IP

	if (argc != 3)
	{
		cerr << "NOT ENOUGH ARGUMENTS" << endl;
		return -1;
	}

	char* hostname = argv[1];
	char* portname = argv[2];

	struct addrinfo hints;
    struct addrinfo* res;

    memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET; // IPv4
	hints.ai_socktype = SOCK_DGRAM; // UDP

	  // get address
	int status = 0;
	if ((status = getaddrinfo(hostname, portname, &hints, &res)) != 0) {
	  cerr << "getaddrinfo: " << gai_strerror(status) <<endl;
	  return 2;
	}

	  //std::cout << "IP addresses for " << argv[1] << ": " << std::endl;
	struct sockaddr_in* ipv4;
	for(struct addrinfo* p = res; p != 0; p = p->ai_next) {
	  // convert address to IPv4 address
	  ipv4 = (struct sockaddr_in*)p->ai_addr;
    // convert the IP to a string and print it:
	  char ipstr[INET_ADDRSTRLEN] = {'\0'};
	  inet_ntop(p->ai_family, &(ipv4->sin_addr), ipstr, sizeof(ipstr));
	  break;
	   // std::cout << "  " << ipstr << ":" << ntohs(ipv4->sin_port) << std::endl;
	}
	freeaddrinfo(res); // free the linked list
	struct sockaddr_in serverAddr = *ipv4;
	int serverAddrLen = sizeof(serverAddr);

	FD_ZERO(&readFds);
	FD_ZERO(&writeFds);


	int sockfd = socket(AF_INET, SOCK_DGRAM, 0);

	int maxSockFd = sockfd;

	int seqNumber = generateSequenceNumber();


	startConnection(seqNumber, sockfd,(struct sockaddr*)&serverAddr, serverAddrLen);

	ofstream myfile;
	myfile.open("result.data");

	int bytes;

	sockaddr_in recAddr;
	socklen_t length = sizeof(recAddr);


	FD_ZERO(&readFds);
	FD_ZERO(&writeFds);
	FD_SET(sockfd, &readFds);

	struct timeval tv;

	while (true)
	{
		int nReadyFds = 0;
	  	tv.tv_sec = 100;
	  	tv.tv_usec = 0;
	  	FD_ZERO(&readFds);
	  	FD_SET(sockfd, &readFds);
	  	if ((nReadyFds = select(maxSockFd + 1, &readFds, &writeFds, NULL, &tv)) == -1) {
	   		perror("select");
	    	return 4;
	  	}

	  	if (nReadyFds == 0){

	  		vector <uint8_t> packetToSend;
	  		TCP ACKPacket(sequenceNumberToSend, expectedSeq, true, false, false, packetToSend);
	  		ACKPacket.setWindow(30720);
	  		ACKPacket.createPacket();
	  		packetToSend = ACKPacket.getPacket();
	  		vector<uint8_t>::iterator first2 = packetToSend.begin();
	  		vector<uint8_t>::iterator last2 = packetToSend.begin() + 8;
	  		outputSendMessage(expectedSeq, true, false, false, false);
	  		sendto(sockfd, &packetToSend[0], 8, 0, (struct sockaddr*)&serverAddr, serverAddrLen);

	  		FD_ZERO(&readFds);
	  		FD_SET(sockfd, &readFds);
	  	} 
	  	else if (FD_ISSET(sockfd, &readFds)){

	  		uint8_t received[1033] = {0};
	  		bytes = recvfrom(sockfd, received, 1032, 0, (sockaddr *)&recAddr, &length);
	  		
	  		vector <uint8_t> receivedPacket;
	  		for (int i = 0; i < bytes; i++) 
	  			receivedPacket.push_back(received[i]);
	  

	  		vector<uint8_t>::const_iterator first = receivedPacket.begin();
	  		vector<uint8_t>::const_iterator last = receivedPacket.begin() + 8;
	  		vector<uint8_t> header(first,last);
	  		HeaderConverter convert(header);

	  		outputReceiveMessage(convert.getSequenceNumber());

	  		if (convert.getF())
	  		{
	  			expectedSeq++;
	  			break;
	  		}

	  		/*
	  		cout << "RECEIVED:" << endl;
		 	cout << "Sequence Number: " << convert.getSequenceNumber() << endl;
		 	cout << "Acknowledgement Number: " << convert.getAck() << endl;
		  	cout << "Window: " << convert.getWindow() << endl;
		  	cout << "Flag A: " << convert.getA() << endl;
		 	cout << "Flag S: " << convert.getS() << endl;
		 	cout << "Flag F: " << convert.getF() << endl;
		 	cout << "Packet size: " << receivedPacket.size() << endl;
		  	cout << endl;
		  	*/

	  		PacketRec rec;
	  		rec.seq = convert.getSequenceNumber();
	  		rec.packet = receivedPacket;
	  		rec.nextSeqNum = (convert.getSequenceNumber() + receivedPacket.size() - 8) % 30721;

	  		if (rec.seq == expectedSeq || isGreater(expectedSeq, rec.seq, expectedSeq))
	  		{
		  		if (packetsReceived.size() == 0)
		  			packetsReceived.push_back(rec);
		  		else{
		  			int positionToEnter = packetsReceived.size() -1;
		  			bool addPacket = true;
		  				while (positionToEnter >= 0){
		  					if (rec.seq == packetsReceived[positionToEnter].seq)
		  					{
		  						addPacket = false;
		  						break;
		  					}
		  					else if (isGreater(expectedSeq, rec.seq, packetsReceived[positionToEnter].seq))
		  						break;
		  					positionToEnter--;
		  				}
		  				if (addPacket){
			  				if (positionToEnter == packetsReceived.size()-1)
			  					packetsReceived.push_back(rec);
			  				else
			  				{
			  					vector<PacketRec>::iterator p = packetsReceived.begin() + (positionToEnter + 1);
			  					packetsReceived.insert(p, rec);
			  				}
			  			}
		  		}
		  	}
	  		if (packetsReceived.size() > 0 && packetsReceived[0].seq == expectedSeq)
	  		{
	  			int covered = 0; 
	  			while (covered < packetsReceived.size() - 1)
	  			{
	  				if (packetsReceived[covered].nextSeqNum != packetsReceived[covered+1].seq)
	  					break;
	  				covered++;
	  			}

	  			expectedSeq = packetsReceived[covered].nextSeqNum;

	  			for (int i = 0; i <= covered; i++)
	  			{

	  				for (int j = 8; j < packetsReceived[i].packet.size(); j++)
	  				{
	  					myfile << packetsReceived[i].packet[j];
	  				}
	  				//packetsReceived.erase(packetsReceived.begin() + i);
	  			}
	  			if (covered == packetsReceived.size() - 1)
	  				packetsReceived.clear();
	  			else
	  				packetsReceived.erase(packetsReceived.begin(), packetsReceived.begin() + covered + 1);

	  		}
	  		//cout << "vector size " << packetsReceived.size() << endl;
	  		//cout << " Packet Contents " << endl;
	  		/*
	  		for (int i = 0; i < packetsReceived.size(); i++)
	  		{
	  			cout << packetsReceived[i].seq << " ";
	  		}
	  		cout << endl;
			*/

 	 		vector <uint8_t> packetToSend;
	  		TCP ACKPacket(sequenceNumberToSend, expectedSeq, true, false, false, packetToSend);
	  		int windowSize = 0;
	  		if (packetsReceived.size() == 0 )
	  			windowSize = 15360;
	  		else if(packetsReceived.size() >= 15)
	  			windowSize = 0;
	  		else
	  			windowSize = 15360 - (packetsReceived.size()*1024);
	  		ACKPacket.setWindow(windowSize);
	  		ACKPacket.createPacket();
	  		packetToSend = ACKPacket.getPacket();
	  		vector<uint8_t>::iterator first2 = packetToSend.begin();
	  		vector<uint8_t>::iterator last2 = packetToSend.begin() + 8;

	  		vector<uint8_t> header2(first2,last2);
	  		HeaderConverter convert2(header2);
	  		/*
	  		cout << "SENT:" << endl;
		 	cout << "Sequence Number: " << convert2.getSequenceNumber() << endl;
		 	cout << "Acknowledgement Number: " << convert2.getAck() << endl;
		  	cout << "Window: " << convert2.getWindow() << endl;
		  	cout << "Flag A: " << convert2.getA() << endl;
		 	cout << "Flag S: " << convert2.getS() << endl;
		 	cout << "Flag F: " << convert2.getF() << endl;
		 	cout << "Packet size: " << packetToSend.size() << endl;
		  	cout << endl;
			*/
			outputSendMessage(expectedSeq, false, false, false, false);
	  		sendto(sockfd, &packetToSend[0], 8, 0, (struct sockaddr*)&serverAddr, serverAddrLen);

		}

	 
	}

	vector <uint8_t> finAckPacket;
	TCP finACK(sequenceNumberToSend, expectedSeq, true,false,true, finAckPacket );
	finACK.setWindow(15360);
	finACK.createPacket();
	finAckPacket = finACK.getPacket();
	outputSendMessage(expectedSeq, false, false, false, true);
	sendto(sockfd, &finAckPacket[0], 8, 0,(struct sockaddr*)&serverAddr, serverAddrLen);
	HeaderConverter h3(finAckPacket);

	vector <uint8_t> finPacket;
	TCP fin(sequenceNumberToSend + 1, expectedSeq, false, false, true, finPacket);
	fin.setWindow(15360);
	fin.createPacket();
	finPacket = fin.getPacket();
	outputSendMessage(expectedSeq, false, false, true, false);
	sendto(sockfd, &finPacket[0], 8, 0,(struct sockaddr*)&serverAddr, serverAddrLen);
	HeaderConverter h4(finPacket);


	bool ACKReceived = false;

	int countResends = 0;
	while (ACKReceived == false)
	{
		int nReadyFds = 0;
		tv.tv_sec = 10;
		tv.tv_usec = 0;
	  	FD_ZERO(&readFds);
	  	FD_SET(sockfd, &readFds);
	  	
		if ((nReadyFds = select(sockfd + 1, &readFds, &writeFds, NULL, &tv)) == -1) {
		   	perror("select");
		    return 4;
		 }
		 if (nReadyFds == 0){
		 	if (countResends == 3)
		 	{
		 		//cout << "Server not responding. Will terminate execution." << endl;
		 		return 0;
		 	}
		 	countResends++;
		 	FD_ZERO(&readFds);
	  		FD_SET(sockfd, &readFds);
	  		outputSendMessage(expectedSeq, true, false, false, true);
		 	sendto(sockfd, &finAckPacket[0], 8, 0,(struct sockaddr*)&serverAddr, serverAddrLen);
			outputSendMessage(expectedSeq,true, false, true, false);
		 	sendto(sockfd, &finAckPacket[0], 8, 0,(struct sockaddr*)&serverAddr, serverAddrLen);

		 }
		 else{
		 		uint8_t received[1033] = {0};
		 		int bytes = recvfrom(sockfd, received, 1032, 0, (sockaddr *)&recAddr, &length);
		  		vector <uint8_t> receivedPacket;
		  		for (int i = 0; i < bytes; i++) 
		  			receivedPacket.push_back(received[i]);
		  
		  		vector<uint8_t>::const_iterator first = receivedPacket.begin();
		  		vector<uint8_t>::const_iterator last = receivedPacket.begin() + 8;
		  		vector<uint8_t> header(first,last);
		  		HeaderConverter convert(header);

		  		outputReceiveMessage(convert.getSequenceNumber());

		  		if (convert.getSequenceNumber() == expectedSeq){
		  			ACKReceived = true;
		  			myfile.close();
		  			return 0;
		  		}
		  		else
		  		{
		  			FD_ZERO(&readFds);
	  				FD_SET(sockfd, &readFds);

		 			outputSendMessage(expectedSeq, true, false, false, true);
		 			sendto(sockfd, &finAckPacket[0], 8, 0,(struct sockaddr*)&serverAddr, serverAddrLen);
		 			outputSendMessage(expectedSeq,true, false, true, false);
		 			sendto(sockfd, &finAckPacket[0], 8, 0,(struct sockaddr*)&serverAddr, serverAddrLen);
		  		}
		 }

	}	
	myfile.close();
	exit(0);
}


/* HELPER FUNCTIONS */

	int startConnection(int& seqNumber, int sockfd, struct sockaddr* serverAddr, int serverAddrLen)
	{

	  	vector <uint8_t> synPacket;
	  	TCP SYN(0, 0, false, true, false, synPacket);
	  	SYN.setWindow(15360);
	  	SYN.createPacket();
	  	synPacket = SYN.getPacket();
	  	HeaderConverter h(synPacket);

	  	outputSendMessage(0, false, true, false, false);

	  	sendto(sockfd, &synPacket[0], 8, 0,serverAddr, serverAddrLen);


	  	sockaddr_in recAddr;
		socklen_t length = sizeof(recAddr);

		uint8_t received[1033] = {0};
		bool ACKReceived = false;

		struct timeval tv;

		while (ACKReceived == false){

			int nReadyFds = 0;
		  	tv.tv_sec = 2;
		  	tv.tv_usec = 0;
		  	FD_ZERO(&readFds);
		  	FD_SET(sockfd, &readFds);
	  	
		  	if ((nReadyFds = select(sockfd + 1, &readFds, &writeFds, NULL, &tv)) == -1) {
		   		perror("select");
		    	return 4;
		  	}
		  	if (nReadyFds == 0)
		  	{
		  		outputSendMessage(0, true, true,false, false);
		  		sendto(sockfd, &synPacket[0], 8, 0,serverAddr, serverAddrLen);
		  	}
		  	else{

		 		int bytes = recvfrom(sockfd, received, 1032, 0, (sockaddr *)&recAddr, &length);

		  		vector <uint8_t> receivedPacket;
		  		for (int i = 0; i < bytes; i++) 
		  			receivedPacket.push_back(received[i]);
		  		

		  		vector<uint8_t>::const_iterator first = receivedPacket.begin();
		  		vector<uint8_t>::const_iterator last = receivedPacket.begin() + 8;
		  		vector<uint8_t> header(first,last);
		  		HeaderConverter convert(header);
		  		
		  		outputReceiveMessage(convert.getSequenceNumber());

		  		if (convert.getAck() == 1){
		  			expectedSeq = convert.getSequenceNumber() + 1;
		  			ACKReceived = true;
		  		}
		  	}
		 }


	  	vector <uint8_t> synACKPacket;
	  	TCP SYNACK(1, expectedSeq, true, false, false, synACKPacket);
	  	SYNACK.setWindow(15360);
	  	SYNACK.createPacket();
	 	synACKPacket = SYNACK.getPacket();
	  	HeaderConverter h2(synACKPacket);

	  	sequenceNumberToSend = 1;

		outputSendMessage(expectedSeq, false, false,false, false);
	 	sendto(sockfd, &synACKPacket[0], 8, 0,serverAddr, serverAddrLen);

	  return 0;

	}

	bool isGreater(int expectedSeq, int seq, int seqInArr)
	{
		if (expectedSeq <= 15360)
		{
			if (seq <= seqInArr + 15360){
				if (seq >= seqInArr)
					return true;
			}
		}
		else
		{	
			if ((seq >= 15360 && seqInArr >= 15360) || (seq < 15360 && seq < 15360))
			{
				if (seq > seqInArr)
					return true;
			}
			if (seqInArr > 15360 && seq < 15360)
			{
				if (seq <= (seqInArr + 15360) % 30721)
					return true;
			}
			
		}
		return false;	
	}

	//  4 1

	int arrayLocation(int sequenceNumber)
	{	
		if (sequenceNumber == 0 && !wrapAround)
			return 0;
		if (sequenceNumber == 1 && !wrapAround)
			return 1;
		if (wrapAround)
			return sequenceNumber / 1024;
		else
			return (sequenceNumber/ 1024) + 1;
	}


	//Return a sequence number between 1 and 30720
	int generateSequenceNumber()
	{
		return 0; 
	}


	void outputReceiveMessage(int sequenceNumber)
	{
		cout << "Receiving packet " << sequenceNumber << endl;
	}

	void outputSendMessage(int ACKNumber, bool retransmission, bool syn, bool fin, bool fin_ack)
	{
	
		if (syn && retransmission)
			cout << "Sending packet Retranmission SYN" << endl;
		else if (syn)
			cout << "Sending packet SYN" << endl;
		else if (fin && retransmission)
			cout << "Sending packet Retranmission" <<  " FIN" << endl;
		else if (fin)
			cout << "Sending packet FIN" << endl;
		else if (fin_ack && retransmission)
			cout << "Sending packet " << ACKNumber << " Retranmission" <<  " FIN-ACK" << endl;
		else if (fin_ack)
			cout << "Sending packet " << ACKNumber << " FIN-ACK" << endl;
		else if (retransmission)
			cout <<"Sending packet " << ACKNumber << " Retranmission"<< endl;
		else
			cout << "Sending packet " << ACKNumber << endl;
	}




