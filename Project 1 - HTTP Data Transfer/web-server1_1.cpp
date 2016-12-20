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
#include <thread>
#include <sys/select.h>
#include "HttpMessage.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "UrlParser.h"

using namespace std;

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

  fd_set readFds;
  fd_set errFds;
  fd_set watchFds;
  fd_set writeFds;
  FD_ZERO(&readFds);
  FD_ZERO(&errFds);
  FD_ZERO(&watchFds);
  FD_ZERO(&writeFds);

  // create a socket using TCP IP
  int listenSockfd = socket(AF_INET, SOCK_STREAM, 0);
  int maxSockfd = listenSockfd;

  // put the socket in the socket set
  FD_SET(listenSockfd, &watchFds);

  // allow others to reuse the address
  int yes = 1;
  if (setsockopt(listenSockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
    perror("setsockopt");
    return 1;
  }


  // Name resolution

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
    
    if(bind(listenSockfd, p->ai_addr, p->ai_addrlen) == -1){
      close (listenSockfd);
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

  

  // set the socket in listen status
  if (listen(listenSockfd, 10) == -1) {
    perror("listen");
    return 3;
  }

  map<int, vector <char>> socket_buffer;

  bool closedConnections = true;

  // initialize timer (2s)
  struct timeval tv;
  while (true) {
    // set up watcher
    int nReadyFds = 0;
    readFds = watchFds;
    errFds = watchFds;
    tv.tv_sec = 15;
    tv.tv_usec = 0;
    if ((nReadyFds = select(maxSockfd + 1, &readFds, &writeFds, &errFds, &tv)) == -1) {
      perror("select");
      return 4;
    }

    if (nReadyFds == 0) 
    {
      if (!closedConnections)
      {
	      std::cerr << "No data has been received for 15 seconds! Closing all connections" << std::endl;
	      for (int fd = 0; fd <= maxSockfd; fd++)
	      {
	      	if (FD_ISSET(fd, &watchFds))
	      	{
	      		if (fd == listenSockfd)
	      			continue;
	      		close(fd);
	      		FD_CLR(fd, &watchFds);
	      	}
	      	if (FD_ISSET(fd, &readFds))
	      	{
	      		if (fd == listenSockfd)
	      			continue;
	      		close(fd);
	      		FD_CLR(fd, &readFds);
	      	}
	      	if(FD_ISSET(fd, &writeFds))
	      	{
	      		close(fd);
	      		FD_CLR(fd, &writeFds);
	      	}      	
	      }
	      closedConnections = true;
  	  }
    }
    else 
    {
      closedConnections = false;

      for(int fd = 0; fd <= maxSockfd; fd++) 
      {
        // get one socket for reading
        if (FD_ISSET(fd, &readFds))
        {
          if (fd == listenSockfd) { // this is the listen socket
            struct sockaddr_in clientAddr;
            socklen_t clientAddrSize = sizeof(clientAddr);
            int clientSockfd = accept(fd, (struct sockaddr*)&clientAddr, &clientAddrSize);

            if (clientSockfd == -1) {
              perror("accept");
              return 5;
            }

            char ipstr[INET_ADDRSTRLEN] = {'\0'};
            inet_ntop(clientAddr.sin_family, &clientAddr.sin_addr, ipstr, sizeof(ipstr));
            //std::cerr << "Accept a connection from: " << ipstr << ":" <<
              //ntohs(clientAddr.sin_port) << " with Socket ID: " << clientSockfd << std::endl;

            // update maxSockfd
            if (maxSockfd < clientSockfd)
              maxSockfd = clientSockfd;

            // add the socket into the socket set
            FD_SET(clientSockfd, &watchFds);
          }
          else 
          { 
            // this is the normal socket

            // read/write data from/into the connection
            char buf[1] = {0};
            //vector <char> request;

            memset(buf, '\0', sizeof(buf));

            int read_size = recv(fd, buf, 1, 0);
            if (read_size ==  -1) {
              perror("recv");          
            }
            //cerr << read_size  << endl;
            int i = 0;

            while (i < read_size)
            {
              socket_buffer[fd].push_back(buf[i]);
              i++;
            }

            vector <char> request = socket_buffer[fd];   

            if (request.size() > 4)
              if (request.at(request.size() - 3) == '\n' && request.at(request.size() - 1) == '\n' && request.at(request.size() - 2) == '\r' && request.at(request.size() - 4)=='\r') 
              {
                FD_CLR(fd, &watchFds);
                FD_SET(fd, &writeFds);
              }
          }
        }
        else if (FD_ISSET(fd, &writeFds)) 
        {
          //get one socket for writing

          vector <char> request = socket_buffer[fd];

          HttpRequest parsefirstline;
          vector<uint8_t> firstline;

          for (int i = 0; i < request.size() && request.at(i) != '\r'; i++) {
            firstline.push_back(request.at(i));
          }

          parsefirstline.decodeFirstLine(firstline);
          //cerr << "File Name: " << parsefirstline.getUrl() << endl;        
          UrlParser p(parsefirstline.getUrl());
          p.parseUrl();
          string filename = p.getFileName();
          string totalPath(filedir);
          totalPath += filename;
         
          ifstream file(totalPath.c_str());
          HttpResponse response;
          vector<uint8_t> payload;
          response.setVersion(parsefirstline.getVersion());
          if (parsefirstline.getErr())
          {
          	response.setStatus("400");
          	response.setDescription("Bad request");
          }
          else if (!file.good()) 
          {
            response.setStatus("404");
            response.setDescription("Not found");
            response.setPayLoad(vector<uint8_t>());
          }
          else 
          {
            char c;
            while(file.get(c))
              payload.push_back(c);
            response.setStatus("200");
            response.setDescription("OK");
            file.close();
          }

          response.setHeader("Content-Length", to_string(payload.size()));  
          //cerr << payload.size() << endl; 
          response.setPayLoad(payload);
          response.setHeader("Connection", "Close");
          vector<uint8_t> totalresponse = response.encodeResponse();
          totalresponse.push_back(0);
          //cerr << &totalresponse[0] << endl;

       
          if (send(fd, &totalresponse[0], totalresponse.size(), 0) == -1) {
            perror("send");
            return 6;
          }          

          socket_buffer[fd].clear();

          string checkForEnd = string(request.begin(), request.end());
          HttpRequest req;
          if (req.findClose(checkForEnd))
          {
            close(fd);
          }
          else
            FD_SET(fd, &watchFds);

          FD_CLR(fd, &writeFds);
        }
      }
    }
  }
  return 0;
}