#include <string>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include "Packets.h"
#include "TCP.h"

const int MAX_DATA = 1024;
const int MAX_SEQ = 30720;

Packets::Packets(std::vector<uint8_t> fileContents, int initialACK)
{
	int N = fileContents.size() / MAX_DATA;
	if (fileContents.size() % MAX_DATA)
		N++;

	// Three extra packets for establishing connection (one) and closing connection (two)
	m_numberOfPackets = N + 3;	

	int initialSEQ = Packets::generateSequenceNumber();	

	m_packets.push_back(new TCP(initialSEQ, initialACK, 1, 1, 0, std::vector<uint8_t>()));

	int seqNumber = initialSEQ + 1;
	int ackNumber = initialACK;

	for (int i = 0; i < N; i++)
	{
		std::vector<uint8_t>::const_iterator first = fileContents.begin() + i*MAX_DATA;
		std::vector<uint8_t>::const_iterator last;

		if (fileContents.size() > (i+1)*MAX_DATA)
			last = fileContents.begin() + (i+1)*MAX_DATA;
		else
			last = fileContents.begin() + fileContents.size();

		std::vector<uint8_t> data(first, last);

		m_packets.push_back(new TCP(seqNumber, ackNumber, 1, 0, 0, data));

		if (fileContents.size() > (i+1)*MAX_DATA)
			seqNumber = (seqNumber + MAX_DATA) % (MAX_SEQ + 1);
		else
			seqNumber = (seqNumber + data.size()) % (MAX_SEQ + 1);

		//ackNumber = ackNumber + 1;
	}

	m_packets.push_back(new TCP(seqNumber, ackNumber, 0, 0, 1, std::vector<uint8_t>()));
	m_packets.push_back(new TCP(seqNumber + 1, ackNumber, 1, 0, 1, std::vector<uint8_t>()));

	// To keep track of retransmissions
	transmitted = std::vector<bool>(m_numberOfPackets, false);
}

int Packets::generateSequenceNumber()
{
	return 0;
	/*std::srand(std::time(0));
    int random_variable = std::rand();
    return (random_variable % 30720) + 1;*/
}

TCP* Packets::getPacket(int packetNumber)
{	
	return m_packets[packetNumber];
}

int Packets::getNumberOfPackets()
{
	return m_numberOfPackets;
}

void Packets::setTransmitted(int packetNumber)
{
	transmitted[packetNumber] = true;
}

bool Packets::getTransmitted(int packetNumber)
{
	return transmitted[packetNumber];
}