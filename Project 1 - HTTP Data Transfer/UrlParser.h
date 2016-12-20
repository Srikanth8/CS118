#ifndef UrlParser_h
#define UrlParser_h
#include <string.h>

class UrlParser{
 private:
  std::string url;
  std::string hostname;
  std::string port;
  std::string filename;
 public:
  UrlParser(std::string urlPassed);
  void parseUrl();
  std::string getHostName();
  std::string getPortName();
  std::string getFileName();
};


#endif
