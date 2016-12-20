#include "HttpRequest.h"
#include <vector>
#include <iostream>
using namespace std;

void HttpRequest::decodeFirstLine(vector<uint8_t> line)
{
	int spaces = 0;

	string method; 
	string url; 
	string version;

	setErr(false);

	for (unsigned int i=0; i < line.size(); i++)	
	{
		if (line[i] == '\r')
			break;

		if (line[i] == ' ') {
		  if (i == 0)
		    setErr(true);
		  if (line[i-1] == ' ')
		    setErr(true);
		     spaces++;
		     continue;
		}

		switch(spaces)	
		{
		case 0:
			method += line[i];
			break;
		case 1:
			url += line[i];
			break;
		case 2:
			version += line[i];
			break;
		}	
	}

	if (method != "GET") {
			cerr << "Invalid method in request line!" << endl;
			setErr(true);
	}

	if (version != "HTTP/1.0" && version != "HTTP/1.1") {
		cerr << "Unsupported version in request line!" << endl;
		setErr(true);
	}
	if (url.size() < 1)
	  setErr(true);

	setMethod(method);
	setUrl(url);
	setVersion(version);
}

bool HttpRequest::findClose(string request)
{
  size_t posOfConnectionHeader = request.find("Connection: Close");
  size_t posOfConnectionHeader2 = request.find("Connection: close");
  size_t posOfConnectionHeader3 = request.find("connection: Close");
  size_t posOfConnectionHeader4 = request.find("connection: close");

  if (posOfConnectionHeader == string::npos && posOfConnectionHeader2 == string::npos && posOfConnectionHeader3 == string::npos && posOfConnectionHeader4 == string::npos)
    return false;
  return true;
  
}

string HttpRequest::getMethod()
{
  return m_method;
}

void HttpRequest::setMethod(string method)
{
  m_method = method;
}

string HttpRequest::getUrl()
{
  return m_url;
}

void HttpRequest::setUrl(string url)
{
  m_url = url;
}

vector<uint8_t> HttpRequest::encodeRequest()
{
	string method = getMethod();
	string url = getUrl();
	string version = getVersion();

	vector<uint8_t> request;

	for (int i=0; i<method.size(); i++)
		request.push_back(method[i]);

	request.push_back(' ');

	for (int i=0; i<url.size(); i++)
		request.push_back(url[i]);

	request.push_back(' ');

	for (int i=0; i<version.size(); i++)
		request.push_back(version[i]);

	request.push_back('\r');
	request.push_back('\n');

	vector<uint8_t> headers = getFormattedHeaders();

	request.insert(request.end(), headers.begin(), headers.end());

	request.push_back('\r');
	request.push_back('\n');
	

	return request;
}

void HttpRequest::setErr(bool e)
{
  err = e;
}

bool HttpRequest::getErr()
{
  return err;
}
