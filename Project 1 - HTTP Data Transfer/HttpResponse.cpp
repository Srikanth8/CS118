#include <string>
#include "HttpResponse.h"
#include <vector>
#include <iostream>
using namespace std;

void HttpResponse::decodeFirstLine(vector<uint8_t> line)
{
	int spaces = 0;

	string version;
	string status;
	string statusDescription;

	for (unsigned int i=0; i < line.size(); i++)	
	{
		if (line[i] == '\r')
			break;
		
		if (line[i] == ' ') {
			spaces++;
			continue;
		}

		switch(spaces)	
		{
		case 0:
			version += line[i];
			break;
		case 1:
			status += line[i];
			break;
		case 2:
			statusDescription += line[i];
			break;
		}	
	}

	if (version != "HTTP/1.0" && version != "HTTP/1.1") {
		cerr << "Unsupported version in status line!" << endl;
	}	

	setVersion(version);
	setStatus(status);
	setDescription(statusDescription);
}

string HttpResponse::getStatus()
{
  return m_status;
}

void HttpResponse::setStatus(string status)
{
  m_status = status;
}

string HttpResponse::getDescription()
{
  return m_statusDescription;
}

void HttpResponse::setDescription(string description)
{
  m_statusDescription = description;
}

vector<uint8_t> HttpResponse::encodeResponse()
{
	string version = getVersion();
	string status = getStatus();
	string statusDescription = getDescription();

	vector<uint8_t> response;

	for (int i=0; i<version.size(); i++)
		response.push_back(version[i]);

	response.push_back(' ');

	for (int i=0; i<status.size(); i++)
		response.push_back(status[i]);

	response.push_back(' ');

	for (int i=0; i<statusDescription.size(); i++)
		response.push_back(statusDescription[i]);

	response.push_back('\r');
	response.push_back('\n');

	vector<uint8_t> headers = getFormattedHeaders();

	response.insert(response.end(), headers.begin(), headers.end());

	response.push_back('\r');
	response.push_back('\n');

	vector<uint8_t> payload = getPayload();

	response.insert(response.end(), payload.begin(), payload.end());

	return response;
}