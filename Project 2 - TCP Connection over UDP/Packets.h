#include <string>
#include <iostream>
#include <vector>

class TCP;

class Packets
{
private:
	std::vector<TCP*> m_packets;
	int m_numberOfPackets;
	std::vector<bool> transmitted;
	int generateSequenceNumber();
public:
	Packets(std::vector<uint8_t> fileContents, int initialACK);
	TCP* getPacket(int packetNumber);
	int getNumberOfPackets();
	void setTransmitted(int packetNumber);
	bool getTransmitted(int packetNumber);
};