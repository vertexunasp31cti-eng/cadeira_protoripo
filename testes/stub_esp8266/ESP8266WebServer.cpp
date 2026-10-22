#include "ESP8266WebServer.h"

#include <ctype.h>

namespace {

int valorHex(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  return -1;
}

// Igual ao urlDecode do core: '+' vira espaco e %XX vira o byte.
std::string decodificar(const std::string& s) {
  std::string r;
  for (size_t i = 0; i < s.size(); ++i) {
    if (s[i] == '+') {
      r += ' ';
    } else if (s[i] == '%' && i + 2 < s.size() && valorHex(s[i + 1]) >= 0 &&
               valorHex(s[i + 2]) >= 0) {
      r += (char)(valorHex(s[i + 1]) * 16 + valorHex(s[i + 2]));
      i += 2;
    } else {
      r += s[i];
    }
  }
  return r;
}

bool mesmoNome(const std::string& a, const char* b) {
  size_t n = strlen(b);
  if (a.size() != n) return false;
  for (size_t i = 0; i < n; ++i) {
    if (tolower((unsigned char)a[i]) != tolower((unsigned char)b[i])) {
      return false;
    }
  }
  return true;
}

}  // namespace

ESP8266WebServer::ESP8266WebServer(int port)
    : porta_(port), iniciado_(false), voltas_(0), metodo_(HTTP_ANY) {}

ESP8266WebServer::ESP8266WebServer(IPAddress, int port)
    : porta_(port), iniciado_(false), voltas_(0), metodo_(HTTP_ANY) {}

void ESP8266WebServer::begin() { iniciado_ = true; }

void ESP8266WebServer::begin(uint16_t port) {
  porta_ = port;
  iniciado_ = true;
}

void ESP8266WebServer::handleClient() { ++voltas_; }

void ESP8266WebServer::close() { iniciado_ = false; }

void ESP8266WebServer::stop() { close(); }

void ESP8266WebServer::on(const Uri& uri, THandlerFunction handler) {
  on(uri, HTTP_ANY, handler);
}

void ESP8266WebServer::on(const Uri& uri, HTTPMethod method,
                          THandlerFunction fn) {
  Rota r;
  r.uri = uri.texto();
  r.metodo = method;
  r.fn = fn;
  rotas_.push_back(r);
}

void ESP8266WebServer::on(const Uri& uri, HTTPMethod method,
                          THandlerFunction fn, THandlerFunction) {
  on(uri, method, fn);
}

void ESP8266WebServer::onNotFound(THandlerFunction fn) { nao_encontrado_ = fn; }

const String& ESP8266WebServer::arg(const String& name) const {
  for (size_t i = 0; i < args_.size(); ++i) {
    if (args_[i].first == name) return args_[i].second;
  }
  return emptyString;
}

const String& ESP8266WebServer::arg(int i) const {
  if (i < 0 || i >= (int)args_.size()) return emptyString;
  return args_[i].second;
}

const String& ESP8266WebServer::argName(int i) const {
  if (i < 0 || i >= (int)args_.size()) return emptyString;
  return args_[i].first;
}

bool ESP8266WebServer::hasArg(const String& name) const {
  for (size_t i = 0; i < args_.size(); ++i) {
    if (args_[i].first == name) return true;
  }
  return false;
}

void ESP8266WebServer::registrar(int code, const std::string& tipo,
                                 const std::string& corpo) {
  ++resposta_.envios;
  resposta_.status = code;
  resposta_.tipo = tipo;
  resposta_.corpo = corpo;
  resposta_.cabecalhos = cabecalhos_pendentes_;
  cabecalhos_pendentes_.clear();  // o core tambem esvazia depois de enviar
}

// Sem tipo, o core manda text/html.
void ESP8266WebServer::send(int code, const char* content_type,
                            const String& content) {
  registrar(code, content_type != NULL ? content_type : "text/html",
            content.c_str());
}

void ESP8266WebServer::send(int code, char* content_type,
                            const String& content) {
  send(code, (const char*)content_type, content);
}

void ESP8266WebServer::send(int code, const String& content_type,
                            const String& content) {
  send(code, content_type.c_str(), content);
}

void ESP8266WebServer::send(int code, const char* content_type,
                            const char* content) {
  send_P(code, content_type, content);
}

void ESP8266WebServer::send(int code, const char* content_type,
                            const char* content, size_t content_length) {
  send_P(code, content_type, content, content_length);
}

void ESP8266WebServer::send_P(int code, PGM_P content_type, PGM_P content) {
  send_P(code, content_type, content, content != NULL ? strlen_P(content) : 0);
}

void ESP8266WebServer::send_P(int code, PGM_P content_type, PGM_P content,
                              size_t contentLength) {
  registrar(code, content_type != NULL ? content_type : "text/html",
            std::string(content != NULL ? content : "", contentLength));
}

void ESP8266WebServer::sendHeader(const String& name, const String& value,
                                  bool first) {
  std::pair<std::string, std::string> c(name.c_str(), value.c_str());
  if (first) {
    cabecalhos_pendentes_.insert(cabecalhos_pendentes_.begin(), c);
  } else {
    cabecalhos_pendentes_.push_back(c);
  }
}

std::string ESP8266WebServer::Resposta::cabecalho(const char* nome) const {
  for (size_t i = 0; i < cabecalhos.size(); ++i) {
    if (mesmoNome(cabecalhos[i].first, nome)) return cabecalhos[i].second;
  }
  return "";
}

void ESP8266WebServer::lerParametros(const std::string& texto) {
  size_t i = 0;
  while (i < texto.size()) {
    size_t fim = texto.find('&', i);
    if (fim == std::string::npos) fim = texto.size();
    std::string par = texto.substr(i, fim - i);
    if (!par.empty()) {
      size_t igual = par.find('=');
      std::string nome = par.substr(0, igual);
      std::string valor =
          igual == std::string::npos ? "" : par.substr(igual + 1);
      args_.push_back(std::make_pair(String(decodificar(nome).c_str()),
                                     String(decodificar(valor).c_str())));
    }
    i = fim + 1;
  }
}

ESP8266WebServer::Resposta ESP8266WebServer::simular(HTTPMethod metodo,
                                                     const char* caminho,
                                                     const char* corpo) {
  std::string c = caminho != NULL ? caminho : "/";
  size_t q = c.find('?');
  std::string rota = c.substr(0, q);

  uri_ = String(rota.c_str());
  metodo_ = metodo;
  args_.clear();
  cabecalhos_pendentes_.clear();
  resposta_ = Resposta();
  resposta_.status = 0;
  resposta_.envios = 0;

  if (q != std::string::npos) lerParametros(c.substr(q + 1));
  if (corpo != NULL && corpo[0] != '\0') lerParametros(corpo);

  for (size_t i = 0; i < rotas_.size(); ++i) {
    const Rota& r = rotas_[i];
    if (r.uri == rota && (r.metodo == HTTP_ANY || r.metodo == metodo)) {
      r.fn();
      return resposta_;
    }
  }
  if (nao_encontrado_) {
    nao_encontrado_();
  } else {
    send(404, "text/plain", String(("Not found: " + rota).c_str()));
  }
  return resposta_;
}
