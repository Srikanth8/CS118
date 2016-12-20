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
#include <time.h>
#include "TCP.h"
#include "Packets.h"
#include "HeaderConverter.h"
using namespace std;

const int MAX_BYTES = 1032;
const int MSS = 1024;
const int MAX_CWND = 30720/2;

const int SLOW_START = 0;
const int CONGESTION_AVOIDANCE = 1;

int sendPacket(int packetNo, Packets &packet, int window, int threshold, string flags, int sockfd, sockaddr_in &recAddr, socklen_t length);
vector<uint8_t> receivePacket(int sockfd, sockaddr_in &recAddr, socklen_t &length);
int packetAcked(Packets packet, int packetNo, int ack);

int main(int argc, char *argv[])
{
  if (argc != 3)
  {
	cerr << "NOT ENOUGH ARGUMENTS" << endl;
	return -1;
  }
  
  char* portnumber = argv[1];
  char* filename = argv[2];

  int port = strtol (portnumber, NULL, 10);

  // create a socket using UDP IP
  int sockfd = socket(AF_INET, SOCK_DGRAM, 0);

  // bind address to socket
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);     // short, network byte order
  addr.sin_addr.s_addr = inet_addr("10.0.0.1");
  memset(addr.sin_zero, '\0', sizeof(addr.sin_zero));

  sockaddr_in recAddr;
  socklen_t length = sizeof(recAddr);

  if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
    perror("bind");
    return 2;
  }

  ifstream file(filename);
  vector<uint8_t> contents;
  char c;
  while(file.get(c))
  	contents.push_back(c);


  // Receive SYN packet
  vector<uint8_t> received_vec_syn = receivePacket(sockfd, recAddr, length);

  HeaderConverter convert_syn(received_vec_syn);
  int ackNumber = convert_syn.getSequenceNumber() + 1;

  Packets packet(contents, ackNumber);
  int numPackets = packet.getNumberOfPackets();
  //cout << "Number of Packets: " << numPackets << endl;

  fd_set readFds;
  FD_ZERO(&readFds);
  FD_SET(sockfd, &readFds);

  struct timeval tv; 

  int counter = 0;
  while(true)
  {
	// Send SYN-ACK packet
	sendPacket(0, packet, 1, MAX_CWND, "SA", sockfd, recAddr, length);

	tv.tv_sec = 0;
	tv.tv_usec = 500000;
	int nReadyFds = 0;	    
	FD_ZERO(&readFds);
	FD_SET(sockfd, &readFds);
	if ((nReadyFds = select(sockfd + 1, &readFds, NULL, NULL, &tv)) == -1) {
	  perror("select");
	  return 4;
	}

	if (nReadyFds == 0) {
	  continue;
	  counter++;
	  if (counter == 3)
	  {
		cerr << "Closing Server!" << endl;
		exit(1);
	  }
	}

	// Receive ACK packet
	received_vec_syn = receivePacket(sockfd, recAddr, length);

	HeaderConverter convert_syn(received_vec_syn);
	if (convert_syn.getA())
		break;	
  } 

  int packetNo = 1;
  int window = MSS;
  int upperBound = packetNo + window/MSS;
  int congestionControlState = SLOW_START;
  int threshold = MAX_CWND;

  // Fast Retransmit
  int mostRecentAck = 0;
  int ackCounter = 0;  

  //cerr << "----------------WINDOW: " << window/MSS << "----------------" << endl;

  while (packetNo < numPackets - 2) 
  {
  	for (int curPacket = packetNo; curPacket < upperBound && curPacket < numPackets - 2; curPacket++)
		  sendPacket(curPacket, packet, window, threshold, "", sockfd, recAddr, length);	

  	bool timeout = false;
  	bool duplicateAcks = false;

  	mostRecentAck = 0;
  	ackCounter = 0;

  	while (packetNo < upperBound && packetNo < numPackets - 2)
  	{
	    tv.tv_sec = 0;
	    tv.tv_usec = 500000;
	    int nReadyFds = 0;	    
	    FD_ZERO(&readFds);
	    FD_SET(sockfd, &readFds);
	    if ((nReadyFds = select(sockfd + 1, &readFds, NULL, NULL, &tv)) == -1) {
	      perror("select");
	      return 4;
	    }

	    if (nReadyFds == 0) {
	    	//cerr << "Timeout" << endl;
	    	timeout = true;
	    	break;
	    }
	    else {
			vector<uint8_t> received_vec = receivePacket(sockfd, recAddr, length);
			HeaderConverter convert(received_vec);			

			// Fast Retransmit
			if (mostRecentAck == convert.getAck()) {
				ackCounter++;
			}
			if (ackCounter == 3) {
				mostRecentAck = 0;
				ackCounter = 0;
				duplicateAcks = true;
				//cerr << "Fast Retransmit" << endl;
				break;
			}
			else {
				mostRecentAck = convert.getAck();
			}

			if (!duplicateAcks) {
				int nextPacket = packetAcked(packet, packetNo, convert.getAck());
				if (nextPacket >= 0) {
					switch (congestionControlState)
					{
					case SLOW_START:
						window += MSS;//*(nextPacket - packetNo);
						if (window > MAX_CWND)
							window = MAX_CWND;
						break;

					case CONGESTION_AVOIDANCE:
						// The divisor must be a multiple of MSS for correctly modifying thw window size
						int W = (window/MSS)*MSS; 
						//for (int i = 0; i < nextPacket - packetNo; i++)
						window += ((MSS*MSS)/W);
						if (window > MAX_CWND)
							window = MAX_CWND;
						break;
					}
					packetNo = nextPacket;		
				}

				// The smalles unacked packet has not reached client
				if (nextPacket - packetNo)
					continue;

				if (window > threshold)
					congestionControlState = CONGESTION_AVOIDANCE;
			}

			// If receive window < congestion window, set receive window as window 
			if (convert.getWindow() < window)
				window = convert.getWindow();
		}
  	}

  	if (timeout) {
  		threshold = window/2;
		congestionControlState = SLOW_START;
		window = MSS;
	}

	if (duplicateAcks) {
		threshold = window/2;
		congestionControlState = CONGESTION_AVOIDANCE;
		window = window/2;
		if (window < MSS)
			window = MSS;
	}

	//if (packetNo < packet.getNumberOfPackets() - 2)
  	//cerr << "----------------WINDOW: " << window/MSS << "----------------" << endl;
	upperBound = packetNo + window/MSS;
  }

  while(true)
  {
  	// Send FIN packet
  	sendPacket(numPackets - 2, packet, window, threshold, "F", sockfd, recAddr, length);

  	tv.tv_sec = 0;
	tv.tv_usec = 500000;
	int nReadyFds = 0;	    
	FD_ZERO(&readFds);
	FD_SET(sockfd, &readFds);
	if ((nReadyFds = select(sockfd + 1, &readFds, NULL, NULL, &tv)) == -1) {
	  perror("select");
	  return 4;
	}

	if (nReadyFds == 0)
	  continue;

  	// Receive FIN-ACK packet
  	vector<uint8_t> received_vec_finack = receivePacket(sockfd, recAddr, length);

  	tv.tv_sec = 0;
	tv.tv_usec = 500000;
	nReadyFds = 0;	    
	FD_ZERO(&readFds);
	FD_SET(sockfd, &readFds);
	if ((nReadyFds = select(sockfd + 1, &readFds, NULL, NULL, &tv)) == -1) {
	  perror("select");
	  return 4;
	}

	if (nReadyFds == 0)
	  continue;

  	// Receive FIN-ACK packet
  	vector<uint8_t> received_vec_fin = receivePacket(sockfd, recAddr, length);

  	HeaderConverter convert_finack(received_vec_finack);
  	HeaderConverter convert_fin(received_vec_fin);

  	if (convert_finack.getF() && convert_finack.getA() && convert_fin.getF())
		break;
  }

  int fin_counter = 0;
  while (true)
  {
  	// Send ACK packet
  	sendPacket(numPackets - 1, packet, window, threshold, "FA", sockfd, recAddr, length);

  	tv.tv_sec = 0;
	tv.tv_usec = 500000;
	int nReadyFds = 0;	    
	FD_ZERO(&readFds);
	FD_SET(sockfd, &readFds);
	if ((nReadyFds = select(sockfd + 1, &readFds, NULL, NULL, &tv)) == -1) {
	  perror("select");
	  return 4;
	}

	if (nReadyFds == 0 || fin_counter == 3)
	  break;
	else
	  fin_counter++;
  }

  exit(0);
}


int sendPacket(int packetNo, Packets &packet, int window, int threshold, string flags, int sockfd, sockaddr_in &recAddr, socklen_t length)
{
  (packet.getPacket(packetNo))->setWindow(window);
  (packet.getPacket(packetNo))->createPacket();  
  vector<uint8_t> segment = (packet.getPacket(packetNo))->getPacket();

  HeaderConverter convert(segment);  

  cout << "Sending packet " << convert.getSequenceNumber() << " " << window << " " << threshold;

  if (packet.getTransmitted(packetNo)) 
  	cout << " Retransmission";

  if (flags == "SA")
  	cout << " SYN-ACK";
  if (flags == "F")
  	cout << " FIN";
  if (flags == "FA")
  	cout << " FIN-ACK";

  cout << endl;

  packet.setTransmitted(packetNo);

  return sendto(sockfd, &segment[0], segment.size(), 0, (struct sockaddr*)&recAddr, length);
}

vector<uint8_t> receivePacket(int sockfd, sockaddr_in &recAddr, socklen_t &length)
{
  char received[MAX_BYTES];
  int bytes = recvfrom(sockfd, received, MAX_BYTES, 0, (sockaddr *)&recAddr, &length);
  vector<uint8_t> received_vec;
  for (int i = 0; i < bytes; i++)
  	received_vec.push_back(received[i]);

  HeaderConverter convert(received_vec);  

  cout << "Receiving packet " << convert.getAck() << endl;

  return received_vec;
}

// Returns packet number if exists, otherwise returns -1
int packetAcked(Packets packet, int packetNo, int ack)
{
	for (int i=packetNo; i < packet.getNumberOfPackets(); i++)
	{
		if ((packet.getPacket(i))->getSequenceNumber() == ack)
			return i;
	}
	return -1;
}




//Sending
/*cout << "SENT:" << endl;
  cout << "Sequence Number: " << convert.getSequenceNumber() << endl;
  cout << "Acknowledgement Number: " << convert.getAck() << endl;
  cout << "Window: " << convert.getWindow() << endl;
  cout << "Flag A: " << convert.getA() << endl;
  cout << "Flag S: " << convert.getS() << endl;
  cout << "Flag F: " << convert.getF() << endl;
  cout << "Packet size: " << segment.size() << endl;
  cout << endl;*/

//Receiving
  /*cout << "RECEIVED:" << endl;
  cout << "Sequence Number: " << convert.getSequenceNumber() << endl;
  cout << "Acknowledgement Number: " << convert.getAck() << endl;
  cout << "Window: " << convert.getWindow() << endl;
  cout << "Flag A: " << convert.getA() << endl;
  cout << "Flag S: " << convert.getS() << endl;
  cout << "Flag F: " << convert.getF() << endl;
  cout << "Packet size: " << received_vec.size() << endl;
  cout << endl;*/

/*uint64_t sec = 0;
uint64_t usec = 500000;
uint64_t SRTT = 500000;
uint64_t RTTDev = SRTT/2; 
uint64_t time_elapsed;
struct timespec start, end;
clock_gettime(CLOCK_MONOTONIC, &start);
bool measuredTime = false;

 if (!measuredTime && packetNo == upperBound - 1) {
clock_gettime(CLOCK_MONOTONIC, &end);
time_elapsed = (BILLION * (end.tv_sec - start.tv_sec) + end.tv_nsec - start.tv_nsec)/1000;	

cerr << time_elapsed << endl;

SRTT = ((MILLION-1)/MILLION)*SRTT + (1/MILLION)*time_elapsed;
RTTDev = ((MILLION-1)/MILLION)*RTTDev + (1/MILLION)*(time_elapsed - SRTT);

uint64_t RTO = SRTT + 4*RTTDev;
sec = RTO / MILLION;				
usec = RTO % MILLION;

measuredTime = true;
}*/