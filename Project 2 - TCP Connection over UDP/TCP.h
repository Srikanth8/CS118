#include <string>
#include <iostream>
#include <vector>
#include <cstdint>

class TCP
{
private:
	// Packet
	std::vector <uint8_t> m_packet;
	int m_packetLength;
	// Headers
	uint16_t m_seqNumber;
	uint16_t m_ackNumber;
	uint16_t m_window;
	bool m_ACK;
	bool m_SYN;
	bool m_FIN;
	// Data
	std::vector <uint8_t> m_data;
	// Helper Functions
	uint8_t getMSB(uint16_t number);
	uint8_t getLSB(uint16_t number);
public:
	TCP(uint16_t seqNumber, uint16_t ackNumber, bool ACK, bool SYN, bool FIN, std::vector <uint8_t> data);
	void setWindow(uint16_t window);
	void createPacket();
	int getSequenceNumber();
	std::vector <uint8_t> getPacket();
	int getPacketLength();
};