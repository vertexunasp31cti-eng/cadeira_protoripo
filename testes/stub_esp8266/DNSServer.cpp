#include "DNSServer.h"

DNSServer::DNSServer()
    : iniciado(false),
      porta(0),
      codigo_erro(DNSReplyCode::NonExistentDomain),
      consultas_atendidas(0) {}

void DNSServer::processNextRequest() { ++consultas_atendidas; }

void DNSServer::setErrorReplyCode(const DNSReplyCode& replyCode) {
  codigo_erro = replyCode;
}

void DNSServer::setTTL(const uint32_t&) {}

bool DNSServer::start(const uint16_t& port, const String& domainName,
                      const IPAddress& resolvedIP) {
  iniciado = true;
  porta = port;
  dominio = domainName.c_str();
  ip = resolvedIP;
  return true;
}

void DNSServer::stop() { iniciado = false; }
