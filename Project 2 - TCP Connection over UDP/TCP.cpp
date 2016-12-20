#include <string>
#include <iostream>
#include <vector>
#include "TCP.h"

TCP::TCP(uint16_t seqNumber, uint16_t ackNumber, bool ACK, bool SYN, bool FIN, std::vector<uint8_t> data)
{
	m_seqNumber = seqNumber;
	m_ackNumber = ackNumber;
	m_ACK = ACK;
	m_SYN = SYN;
	m_FIN = FIN;
	m_data = data;
}

void TCP::setWindow(uint16_t window)
{
	m_window = window;
}

void TCP::createPacket()
{	
	if (m_packet.size() != 0)
		return;
	m_packet.push_back(TCP::getMSB(m_seqNumber));
	m_packet.push_back(TCP::getLSB(m_seqNumber));
	m_packet.push_back(TCP::getMSB(m_ackNumber));
	m_packet.push_back(TCP::getLSB(m_ackNumber));
	m_packet.push_back(TCP::getMSB(m_window));
	m_packet.push_back(TCP::getLSB(m_window));
	m_packet.push_back(0);
	m_packet.push_back(m_ACK*4 + m_SYN*2 + m_FIN*1);
	m_packet.insert(m_packet.end(), m_data.begin(), m_data.end());
	m_packetLength = m_packet.size();
}

int TCP::getSequenceNumber()
{
	return m_seqNumber;
}

std::vector <uint8_t> TCP::getPacket()
{
	return m_packet;
}

int TCP::getPacketLength()
{
	return m_packetLength;
}

uint8_t TCP::getMSB(uint16_t number)
{
	//return (number >> 8) & 255;
	return (uint8_t)((number & 0xFF00) >> 8);
}

uint8_t TCP::getLSB(uint16_t number)
{
	//return number & 255;
	return (uint8_t)(number & 0x00FF);
}

