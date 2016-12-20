#include "HttpMessage.h"
#include <map>
#include <string>
#include <vector>
#include <stdint.h>
#include <iostream>
using namespace std;

void HttpMessage::setVersion(string version)
{
	m_version = version;
}

string HttpMessage::getVersion()
{
  return m_version;
}

void HttpMessage::setHeader(string key, string value)
{
  m_headers[key] = value;
}

string HttpMessage::getHeader(string key)
{
  return m_headers[key];
}

void HttpMessage::setPayLoad(vector<uint8_t> blob)
{
  m_payload = blob;
}

vector<uint8_t> HttpMessage::getPayload()
{
  return m_payload;
}

vector<uint8_t> HttpMessage::getFormattedHeaders()
{
	vector<uint8_t> headers;

	for (auto it=m_headers.begin(); it!=m_headers.end(); ++it)
	{
		string first = it->first;

		for (int i=0; i<first.size(); i++)
			headers.push_back(first[i]);

		headers.push_back(':');
		headers.push_back(' ');

		string second = it->second;

		for (int i=0; i<second.size(); i++)
			headers.push_back(second[i]);

		headers.push_back('\r');
		headers.push_back('\n');
	}

	return headers;
}
