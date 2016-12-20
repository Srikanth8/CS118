#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <iostream>
#include <sstream>
#include <thread>
#include <fstream>
#include <map>
#include <sys/select.h>
#include "UrlParser.h"
#include "HttpRequest.h"
#include "HttpResponse.h"

using namespace std;

int findPayloadLength(stringstream& responseStream);
int findPayloadStartAndCurrentLength(stringstream& responseStream, int& payloadLength);

int main(int argc, char *argv[])
{
  if (argc == 1){
  	cerr << "URL has not been specified!" << endl;
	return -1;
  }

  char* url;

  map<string, vector<UrlParser*>> urls;

  int j = 0;
  for (; j < argc-1; j++)
  {
  	
  	url = argv[j+1];
  	UrlParser temp(url);
  	temp.parseUrl();
  	string key = temp.getHostName() + temp.getPortName();
  	if(urls.find(key) == urls.end())
  	{
  		urls[key];
  		urls[key].push_back(new UrlParser(url));
  		urls[key][0]->parseUrl();
  		//urls.insert(temp.getHostName(), tempP);
  	}
  	else
  	{
  		urls[key].push_back(new UrlParser(url));
  		//cerr << urls[key].size()<< endl;
  		urls[key].back()->parseUrl();
  	} 

  	//cerr << "Host Name: " << parser[j]->getHostName() << endl;
  	//cerr << "Port Name: " << parser[j]->getPortName() << endl;
  	//cerr << "File Name: " << parser[j]->getFileName() << endl;
  }


	for (auto it = urls.begin(); it != urls.end(); ++it){
	  vector <UrlParser*> parser = it->second;
	  const char* hostname = (parser[0]->getHostName()).c_str();
	  const char* portname = (parser[0]->getPortName()).c_str();

	  // create a socket using TCP IP
	  int sockfd = socket(AF_INET, SOCK_STREAM, 0);

	  struct addrinfo hints;
	  struct addrinfo* res;

	  memset(&hints, 0, sizeof(hints));
	  hints.ai_family = AF_INET; // IPv4
	  hints.ai_socktype = SOCK_STREAM; // TCP

	  // get address
	  int status = 0;
	  if ((status = getaddrinfo(hostname, portname, &hints, &res)) != 0) {
	    cerr << "getaddrinfo: " << gai_strerror(status) <<endl;
	    return 2;
	  }

	  //std::cerr << "IP addresses for " << argv[1] << ": " << std::endl;

	  for(struct addrinfo* p = res; p != 0; p = p->ai_next) {
	    // convert address to IPv4 address
	    struct sockaddr_in* ipv4 = (struct sockaddr_in*)p->ai_addr;

	    // convert the IP to a string and print it:
	    char ipstr[INET_ADDRSTRLEN] = {'\0'};
	    inet_ntop(p->ai_family, &(ipv4->sin_addr), ipstr, sizeof(ipstr));
	    //cerr << "  " << ipstr << endl;
	    //cerr << "  " << ipstr << ":" << ntohs(ipv4->sin_port) << endl;

	    if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
	      perror("connect");
	      close(sockfd);
	      continue;
	    }
	    break;
	    // std::cerr << "  " << ipstr << ":" << ntohs(ipv4->sin_port) << std::endl;
	  }

	  freeaddrinfo(res); // free the linked list

	  struct sockaddr_in clientAddr;
	  socklen_t clientAddrLen = sizeof(clientAddr);
	  if (getsockname(sockfd, (struct sockaddr *)&clientAddr, &clientAddrLen) == -1) {
	    perror("getsockname");
	    return 3;
	  }

	  char ipstr[INET_ADDRSTRLEN] = {'\0'};
	  inet_ntop(clientAddr.sin_family, &clientAddr.sin_addr, ipstr, sizeof(ipstr));
	  //std::cerr << "Set up a connection from: " << ipstr << ":" <<
	    //ntohs(clientAddr.sin_port) << std::endl;


	  // Create http request class and set required values
	  vector<HttpRequest> request(parser.size());

	  for (int i = 0; i < request.size(); i++)
	  {
		  request[i].setMethod("GET");
		  request[i].setUrl(parser[i]->getFileName());
		  request[i].setVersion("HTTP/1.1");

		  if (i == request.size()-1)
		  	request[i].setHeader("Connection", "Close");
		  else
		  	request[i].setHeader("Connection", "Keep-Alive");

		  request[i].setHeader("Accept", "*/*");
		  //cerr << parser.getPortName() << endl;
		  if (parser[i]->getPortName() != "http")
		    request[i].setHeader("Host", parser[i]->getHostName()+":"+parser[i]->getPortName());
		  else
		    request[i].setHeader("Host", parser[i]->getHostName());
		  //request.setHeader("Connection", "Keep-Alive");
		  

		  vector<uint8_t> requestMessage = request[i].encodeRequest();
		  //cerr << &requestMessage[0] << endl;
		  if (send(sockfd, &requestMessage[0], requestMessage.size(), 0) == -1) {
		      perror("send");
		      return 4;
		  }
	  }

	  // For client timeout
	  fd_set watchFds;
	  FD_ZERO(&watchFds);
	  fd_set errFds;
	  FD_ZERO(&errFds);

	  int maxSockfd = sockfd;

	  FD_SET(sockfd, &watchFds);

	  struct timeval tv;

	  int nReadyFds = 0;

	  bool closeClient = false;

	  for (int i=0; i<request.size(); i++)
	  {
		  char buffer[2] = {0};
		  stringstream responseStream;

		  int size_recv , total_size= 0;
		  int payloadLength = -1;
		  int currentLength = -2;
		  int payloadStart;	  

		  while(currentLength < payloadLength)
		  {
		  	  tv.tv_sec = 15;
	  		  tv.tv_usec = 0;

		  	  if ((nReadyFds = select(maxSockfd + 1, &watchFds, NULL, &errFds, &tv)) == -1) {
			     perror("select");
			     return 4;
			  }

			  if (nReadyFds == 0) 
			  {
			    cerr << "No data has been received for 15 seconds! Closing Client Now" << std::endl;
			    closeClient = true;
			    break;
			  }

		      memset(buffer, '\0', sizeof(buffer));  //clear the buffer

		      size_recv = recv(sockfd, buffer, 1, 0);

		      if(size_recv == -1) { 
		          perror("recv");
		          return 5;
		      }
		      else if (size_recv == 0) {
		          break;
		      }
		      else {
		          total_size += size_recv;
		          responseStream << buffer;
		      }

		      if (payloadLength == -1)
		        payloadLength = findPayloadLength(responseStream);

		      payloadStart = findPayloadStartAndCurrentLength(responseStream, currentLength);
		  }

		  if (closeClient)
		  	break;

		  string response_str = responseStream.str();

		  HttpResponse parsefirstline;
		  vector<uint8_t> firstline;
		  for (int i = 0; i < response_str.size() && response_str[i] != '\r'; i++){
		     firstline.push_back(response_str[i]);
		  }
		  parsefirstline.decodeFirstLine(firstline);

		  string stat = parsefirstline.getStatus();

		  if (stat == "404" || stat == "400")
		  {
		    cerr << "Error " << stat << endl;
		  }
		  else
		  {
		  	  string payload_str = response_str.substr(payloadStart, payloadLength);

			  ofstream myfile;
			  myfile.open(parser[i]->getFileName().substr(1, parser[i]->getFileName().size()-1));
			  myfile << payload_str;
			  myfile.close();
		  }
	  }
	    
	  close(sockfd);
  }
  return 0;
}

// Returns the length of payload
// Returns -1 if the "Content-Length" header has not been encountered yet
int findPayloadLength(stringstream& responseStream)
{
  string response = responseStream.str();
  string contentLength = "Content-Length: ";
  size_t pos = response.find(contentLength);

  if (pos == string::npos)
    return -1;

  if (response.size() <= pos + contentLength.size())
    return -1;

  stringstream payloadLengthStream;
  for (size_t i=pos + contentLength.size(); response[i] != '\r'; i++) {
  	if (i == response.size()-1)
  		return -1;
    payloadLengthStream << response[i];
  }

  int payloadLength;
  payloadLengthStream >> payloadLength;

  return payloadLength;
}

// Stores the no. of payload characters received in payloadLength
// Returns the starting position of the payload
// Returns -2 if end of headers not reached
int findPayloadStartAndCurrentLength(stringstream& responseStream, int& payloadLength)
{
  string response = responseStream.str();
  string headerEnd = "\r\n\r\n";
  size_t pos = response.find(headerEnd);

  if (pos == string::npos)
    return -2;

  if (response.size() < pos + headerEnd.size())
    return -2;

  payloadLength = response.size() - (pos + headerEnd.size());
  return pos + headerEnd.size();
}

//cerr << "payloadLength: " << payloadLength << endl;
//cerr << "payloadStart: " << payloadStart << endl;

// for (int p=0; response_str.substr(p, 4) != "\r\n\r\n"; p++)
// 	cerr << response_str[p];
// cerr << endl;
//cerr << payload_str << endl;

//cerr << "Extracted Length of Payload: " << payloadLength << endl;
//cerr << "Length of Payload: " << payload_str.size() << endl;