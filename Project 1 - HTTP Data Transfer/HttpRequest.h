#ifndef Http_Req
#define Http_Req

#include <string>
#include <vector>
#include "HttpMessage.h"

class HttpRequest: public HttpMessage
{
 private:
  std::string m_url;
  std::string m_method;
  bool err;

 public:
  virtual void decodeFirstLine(std::vector<uint8_t> line);
  std::string getMethod();
  void setMethod(std::string method);
  std::string getUrl();
  void setUrl(std::string url);
  std::vector<uint8_t> encodeRequest();
  bool findClose(std::string checkForEnd);
  void setErr(bool err);
  bool getErr();
};


#endif
