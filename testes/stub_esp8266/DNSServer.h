/*
 * DNSServer.h - Stub do servidor DNS do core ESP8266 (portal cativo).
 */
#ifndef DNSSERVER_H_STUB_ESP8266
#define DNSSERVER_H_STUB_ESP8266

#include "Arduino.h"
#include "IPAddress.h"

enum class DNSReplyCode {
  NoError = 0,
  FormError = 1,
  ServerFailure = 2,
  NonExistentDomain = 3,
  NotImplemented = 4,
  Refused = 5,
  YXDomain = 6,
  YXRRSet = 7,
  NXRRSet = 8
};

class DNSServer {
 public:
  DNSServer();

  void processNextRequest();
  void setErrorReplyCode(const DNSReplyCode& replyCode);
  void setTTL(const uint32_t& ttl);
  bool start(const uint16_t& port, const String& domainName,
             const IPAddress& resolvedIP);
  void stop();

  // --- Ajudantes de teste ---------------------------------------------------
  bool iniciado;
  uint16_t porta;
  std::string dominio;
  IPAddress ip;
  DNSReplyCode codigo_erro;
  unsigned long consultas_atendidas;  // chamadas de processNextRequest()
};

#endif  // DNSSERVER_H_STUB_ESP8266
