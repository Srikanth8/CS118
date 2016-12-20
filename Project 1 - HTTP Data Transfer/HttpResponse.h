#ifndef Http_Resp
#define Http_Resp

#include <string>
#include "HttpMessage.h"

class HttpResponse: public HttpMessage
{
 private:
  std::string m_status;
  std::string m_statusDescription;
 public:
  virtual void decodeFirstLine(std::vector<uint8_t> line);
  std::string getStatus();
  void setStatus(std::string status);
  std::string getDescription();
  void setDescription(std::string description);
  std::vector<uint8_t> encodeResponse();
};


#endif
