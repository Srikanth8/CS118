#include <string>
#include <iostream>
#include <vector>


class HeaderConverter
{
private:
	int ack;
	int sequencenumber;
	int window;
	bool A;
	bool S;
	bool F;
public:
	HeaderConverter(std::vector<uint8_t> header);
	int getAck();
	int getSequenceNumber();
	int getWindow();
	bool getA();
	bool getS();
	bool getF();
};