#ifndef NETWORK_CLIENT_H
#define NETWORK_CLIENT_H

#include <Arduino.h>

namespace stockdata {

struct HttpResponse {
  int statusCode;
  // Mutable transport-owned memory permits zero-copy JSON parsing. Valid until
  // the next request made by that transport instance.
  char *body;
  size_t bodyLength;
};

struct HttpHeader {
  const char *name;
  const char *value;
};

class NetworkClient {
public:
  virtual ~NetworkClient() {}

  virtual bool get(const char *url, const HttpHeader *headers,
                   uint8_t headerCount, HttpResponse &response) = 0;
};

} // namespace stockdata

#endif