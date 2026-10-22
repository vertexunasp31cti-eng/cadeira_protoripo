/*
 * ESP8266WebServer.h - Stub do servidor HTTP do core ESP8266.
 *
 * Funcional o bastante para testar as rotas: guarda os handlers na ordem em
 * que foram registrados e, com simular(), faz uma requisicao de mentira e
 * devolve status, tipo, cabecalhos e corpo da resposta. Como no core, os
 * parametros vem da query string e do corpo form-urlencoded, nessa ordem, e
 * rota sem handler cai no onNotFound (ou 404 se nao houver).
 */
#ifndef ESP8266WEBSERVER_H_STUB_ESP8266
#define ESP8266WEBSERVER_H_STUB_ESP8266

#include <functional>
#include <string>
#include <utility>
#include <vector>

#include "Arduino.h"
#include "IPAddress.h"

enum HTTPMethod {
  HTTP_ANY,
  HTTP_GET,
  HTTP_HEAD,
  HTTP_POST,
  HTTP_PUT,
  HTTP_PATCH,
  HTTP_DELETE,
  HTTP_OPTIONS
};

class Uri {
 public:
  Uri(const char* uri) : uri_(uri != nullptr ? uri : "") {}
  Uri(const String& uri) : uri_(uri.c_str()) {}
  const std::string& texto() const { return uri_; }

 private:
  std::string uri_;
};

class ESP8266WebServer {
 public:
  typedef std::function<void(void)> THandlerFunction;

  ESP8266WebServer(int port = 80);
  ESP8266WebServer(IPAddress addr, int port = 80);

  void begin();
  void begin(uint16_t port);
  void handleClient();
  void close();
  void stop();

  void on(const Uri& uri, THandlerFunction handler);
  void on(const Uri& uri, HTTPMethod method, THandlerFunction fn);
  void on(const Uri& uri, HTTPMethod method, THandlerFunction fn,
          THandlerFunction ufn);
  void onNotFound(THandlerFunction fn);

  const String& uri() const { return uri_; }
  HTTPMethod method() const { return metodo_; }
  const String& arg(const String& name) const;
  const String& arg(int i) const;
  const String& argName(int i) const;
  int args() const { return (int)args_.size(); }
  bool hasArg(const String& name) const;

  void send(int code, const char* content_type = NULL,
            const String& content = emptyString);
  void send(int code, char* content_type, const String& content);
  void send(int code, const String& content_type, const String& content);
  void send(int code, const char* content_type, const char* content);
  void send(int code, const char* content_type, const char* content,
            size_t content_length);
  void send_P(int code, PGM_P content_type, PGM_P content);
  void send_P(int code, PGM_P content_type, PGM_P content,
              size_t contentLength);
  void sendHeader(const String& name, const String& value, bool first = false);

  // --- Ajudantes de teste ---------------------------------------------------
  struct Resposta {
    int status;
    std::string tipo;
    std::vector<std::pair<std::string, std::string> > cabecalhos;
    std::string corpo;
    int envios;  // quantas vezes o handler chamou send(); o certo e 1

    // Valor do cabecalho (nome sem diferenciar maiuscula), "" se nao houver.
    std::string cabecalho(const char* nome) const;
  };

  // Faz uma requisicao. caminho pode ter query string ("/x?a=1&b=2"); corpo
  // e form-urlencoded, como o app manda no POST.
  Resposta simular(HTTPMethod metodo, const char* caminho,
                   const char* corpo = "");

  bool iniciado() const { return iniciado_; }
  int porta() const { return porta_; }
  unsigned long voltasAtendidas() const { return voltas_; }

 private:
  struct Rota {
    std::string uri;
    HTTPMethod metodo;
    THandlerFunction fn;
  };

  void registrar(int code, const std::string& tipo, const std::string& corpo);
  void lerParametros(const std::string& texto);

  int porta_;
  bool iniciado_;
  unsigned long voltas_;
  std::vector<Rota> rotas_;
  THandlerFunction nao_encontrado_;

  String uri_;
  HTTPMethod metodo_;
  std::vector<std::pair<String, String> > args_;
  std::vector<std::pair<std::string, std::string> > cabecalhos_pendentes_;
  Resposta resposta_;
};

#endif  // ESP8266WEBSERVER_H_STUB_ESP8266
