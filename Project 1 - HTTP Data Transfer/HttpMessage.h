#ifndef Http_Mess
#define Http_Mess

#include <map>
#include <string>
#include <vector>
#include <stdint.h>


class HttpMessage{
 private:
  std::map <std::string, std::string> m_headers; 
  std::string m_version;
  std::vector <uint8_t> m_payload;
  int m_payloadLength;
 public:
  // HttpMessage();
  virtual void decodeFirstLine(std::vector<uint8_t> line)=0;
  void setVersion(std::string version);
  std::string getVersion();
  void setHeader(std::string key, std::string value);
  std::string getHeader(std::string key);
  void decodeHeaderLine(std::vector<uint8_t> line);
  void setPayLoad(std::vector<uint8_t> blob);
  std::vector<uint8_t> getPayload();
  std::vector<uint8_t> getFormattedHeaders();
};


#endif
