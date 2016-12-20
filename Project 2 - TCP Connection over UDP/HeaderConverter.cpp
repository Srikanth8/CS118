#include <string>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include "HeaderConverter.h"

HeaderConverter::HeaderConverter(std::vector<uint8_t> header)
{
	sequencenumber = ((1<<8) * header.at(0)) + header.at(1);
	ack = ((1<<8) * header.at(2)) + header.at(3);
	window = ((1<<8) * header.at(4)) + header.at(5);
	A = header.at(7) & 4;
	S = header.at(7) & 2;
	F = header.at(7) & 1;
}
int HeaderConverter::getAck()
{
	return ack;
}
int HeaderConverter::getSequenceNumber()
{
	return sequencenumber;
}
int HeaderConverter::getWindow()
{
	return window;
}
bool HeaderConverter::getA()
{
	return A;
}
bool HeaderConverter::getS()
{
	return S;
}
bool HeaderConverter::getF()
{
	return F;
}