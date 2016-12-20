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
  char* url = argv[1];
  UrlParser parser(url);
  parser.parseUrl();
  
  //cerr << "Host Name: " << parser.getHostName() << endl;
  //cerr << "Port Name: " << parser.getPortName() << endl;
  //cerr << "File Name: " << parser.getFileName() << endl;
  
  const char* hostname = parser.getHostName().c_str();
  const char* portname = parser.getPortName().c_str();
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
  HttpRequest request;

  request.setMethod("GET");
  request.setUrl(parser.getFileName());
  request.setVersion("HTTP/1.0");

  request.setHeader("Accept", "*/*");
  //cerr << parser.getPortName() << endl;
  if (parser.getPortName() != "http")
    request.setHeader("Host", parser.getHostName()+":"+parser.getPortName());
  else
    request.setHeader("Host", parser.getHostName());
  //request.setHeader("Connection", "Keep-Alive");
  

  vector<uint8_t> requestMessage = request.encodeRequest();
  //cerr << &requestMessage[0] << endl;
  if (send(sockfd, &requestMessage[0], requestMessage.size(), 0) == -1) {
      perror("send");
      return 4;
  }

  char buffer[10001] = {0};
  stringstream responseStream;

  //HttpResponse response;

  int size_recv , total_size= 0;
  int payloadLength = -1;
  int currentLength = -2;
  int payloadStart;

  while(currentLength < payloadLength)
  {
      memset(buffer, '\0', sizeof(buffer));  //clear the buffer

      size_recv = recv(sockfd, buffer, 10000, 0);

      if(size_recv == -1) { 
          perror("recv");
          return 5;
      }
      else if (size_recv == 0) {
      	 // cerr << "got here" << endl;
          break;
      }
      else {
          total_size += size_recv;
          responseStream << buffer;
      }

      if (payloadLength == -1)
        payloadLength = findPayloadLength(responseStream);

      payloadStart = findPayloadStartAndCurrentLength(responseStream, currentLength);

      if (payloadLength == -1)
      	currentLength = -2;
  }

  string response_str = responseStream.str();
  string payload_str;

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
    if (payloadLength != -1)
    {
      payload_str = response_str.substr(payloadStart, payloadLength);
      //vector<uint8_t> payload_vec(payload_str.begin(), payload_str.end());
      //response.setPayLoad(payload_vec);
    }
    else
    {
      int startPos = response_str.find("\r\n\r\n") + 4;
      payload_str = response_str.substr(startPos);
    }

   // cerr << "Extracted Length of Payload: " << payloadLength << endl;
   // cerr << "Length of Payload: " << payload_str.size() << endl;

    ofstream myfile;
    myfile.open(parser.getFileName().substr(1, parser.getFileName().size()-1));
    myfile << payload_str;
    myfile.close();
  }
    
  close(sockfd);
  
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