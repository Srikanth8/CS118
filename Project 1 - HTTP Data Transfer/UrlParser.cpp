#include <string>
#include <stdlib.h>
#include <stdio.h>
#include "UrlParser.h"
#include <iostream>
using namespace std;

UrlParser::UrlParser(string urlPassed)
{
  url = urlPassed;
  port = "http";
  filename = "/index.html";
  hostname = "";
}

string UrlParser::getHostName()
{
  return hostname;
}

string UrlParser::getFileName()
{
  return filename;
}

string UrlParser::getPortName()
{
  return port;
}

void UrlParser::parseUrl()
{
  size_t posOfHtml = url.find("http://");
  int beginHostName = 0;
  bool searchForFile = true;
  
  if (posOfHtml == string::npos){
    beginHostName = 0;
    // cout<<"HTML NOT FOUND"<<endl;
  }
  else
    beginHostName = 7;
  size_t endOfHostName = url.find("/", beginHostName);
  size_t posOfPort = url.find(":", beginHostName);
  if (endOfHostName == string::npos){
      endOfHostName = url.length();
      searchForFile = false;
    }
  if (posOfPort == string::npos){    
    hostname = url.substr(beginHostName, endOfHostName - (beginHostName)); 
  }
  else{
    port = url.substr(posOfPort + 1, endOfHostName - (posOfPort + 1));
    hostname = url.substr(beginHostName, posOfPort - (beginHostName)); 
  }
  if (!searchForFile || endOfHostName == url.length()-1)
    return;
  filename = url.substr(endOfHostName, url.length()-endOfHostName);
}
