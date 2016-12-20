#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <thread>
#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <netdb.h>
#include <fstream>
#include "HttpMessage.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "UrlParser.h"
#include <thread>
using namespace std;

vector <thread> threads;
int nthreads;

void serve(int threadnum, int sockfd, int clientSockfd, char* filedir);

int main(int argc, char *argv[])
{
  char* hostname = "localhost";
  char* port = "4000";
  char* filedir = ".";
  if (argc != 1 && argc != 4)
  {
    perror("THIS IS NOT A VALID AMOUNT OF INPUTS.\nMust input at least 3 parameters\n");
    return -1;
  }
  if (argc == 4){
    hostname = argv[1];
    port = argv[2];
    filedir = argv[3];
  }
  
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  int yes = 1;
  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1){
    perror("setsockopt failure");
    return 1;
  }

  // Trying some name resolution stuff:

  struct addrinfo hints; 
  struct addrinfo* res;
  
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET; // IPv4
  hints.ai_socktype = SOCK_STREAM; // TCP

  // get address
  int status = 0;
  if ((status = getaddrinfo(hostname, port, &hints, &res)) != 0) {
    cerr << "getaddrinfo: " << gai_strerror(status) << endl;
    return 2;
  }

  //std::cerr << "IP addresses for " << hostname << ": " << endl;
  struct sockaddr_in* ipv4;
  for(struct addrinfo* p = res; p != 0; p = p->ai_next) {
    // convert address to IPv4 address
    ipv4 = (struct sockaddr_in*)p->ai_addr;

    // convert the IP to a string and print it:
    char ipstr[INET_ADDRSTRLEN] = {'\0'};
    inet_ntop(p->ai_family, &(ipv4->sin_addr), ipstr, sizeof(ipstr));
    //cerr << "  " << ipstr << endl;
    //cerr << "  " << ipstr << ":" << ntohs(ipv4->sin_port) << std::endl;
    
    if(bind(sockfd, p->ai_addr, p->ai_addrlen) == -1){
      close (sockfd);
      perror("bind");
      continue;
    }
    
    break;
  }

  if (ipv4 == NULL) {
    // looped off the end of the list with no successful bind
    fprintf(stderr, "failed to bind socket\n");
    exit(2);
  }

  freeaddrinfo(res); // free the linked list
  

  while(1){
    nthreads = 0; 
    if (listen(sockfd, 1) == -1)
      {
	perror("listen");
	return 3;
      }
    struct sockaddr_in clientAddr;
    socklen_t clientAddrSize = sizeof(clientAddr);
    int clientSockfd = accept(sockfd, (struct sockaddr*)&clientAddr, &clientAddrSize);
    
    if (clientSockfd == -1){
      perror("accept");
      return 4;
    }
    threads.push_back(thread(serve, nthreads, sockfd, clientSockfd, filedir));
    nthreads++;
  }
  return 0;
}


void serve(int threadnum, int sockfd, int clientSockfd, char* filedir){ 
    // read/write data from/into the connection
    bool isEnd = false;
    char buf[20] = {0};
    // std::stringstream ss;
    vector <char> request;
    while (!isEnd) {
      memset(buf, '\0', sizeof(buf));

      int read_size = recv(clientSockfd, buf, 20, 0);
      if (read_size ==  -1) {
	perror("recv");
	//threads.at(threadnum).join();
      }
      // cerr << read_size  << endl;
      int i = 0;
      while (i < read_size){
	//cerr << "success" << endl;
	request.push_back(buf[i]);
	i++;
      }
    
      if (request.at(request.size() - 3) == '\n' && request.at(request.size() - 1) == '\n' && request.at(request.size() - 2) == '\r' && request.at(request.size() - 4)=='\r'){
	break;
      }
    }
  
   // cerr << "success" << endl;
    /*string test = "";
    while (test!="stop")
      {
	cin >> test;
      }*/
    
    HttpRequest parsefirstline;
    vector<uint8_t> firstline;
    for (int i = 0; i < request.size() && request.at(i) != '\r'; i++){
      firstline.push_back(request.at(i));
    }
    parsefirstline.decodeFirstLine(firstline);
    //cerr << "URL: " << parsefirstline.getUrl() << endl;
    UrlParser p(parsefirstline.getUrl());
    p.parseUrl();
    string filename = p.getFileName();
    string totalPath(filedir);
    totalPath += filename;

   // cerr << "total path:" << totalPath << endl;
  
    //char* totalFileName = totalPath.c_str();
   
    ifstream file(totalPath.c_str());
    HttpResponse response;
    vector<uint8_t> payload;
    response.setVersion(parsefirstline.getVersion());
    if (parsefirstline.getErr())
    {
      response.setStatus("400");
      response.setDescription("Bad request");
    }
    else if (!file.good()){
      response.setStatus("404");
      response.setDescription("Not found");
      response.setPayLoad(vector<uint8_t>());
    }
    else{
      char c;
      while(file.get(c))
	       payload.push_back(c);
      response.setStatus("200");
      response.setDescription("OK");
      file.close();
    }
    response.setHeader("Content-Length", to_string(payload.size()));   
    response.setPayLoad(payload);
    response.setHeader("Connection", "Close");
    vector<uint8_t> totalresponse = response.encodeResponse();
    //cerr << &totalresponse[0] << endl;

    //cerr << payload.size() << endl;
 
    if (send(clientSockfd, &totalresponse[0], totalresponse.size(), 0) == -1) {
      perror("send");
      return;
      //threads.at(threadnum).join();
    }

    close(clientSockfd);
    //threads.at(threadnum).join();
  } 

