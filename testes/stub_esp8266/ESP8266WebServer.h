/*
 * ESP8266WebServer.h - Stub do servidor HTTP do ESP8266, para os testes no PC.
 *
 * Guarda as rotas registradas e deixa o teste "abrir" uma URL com
 * requisitar(), como se o celular tivesse feito a requisicao.
 */
#ifndef ESP8266WEBSERVER_H_STUB
#define ESP8266WEBSERVER_H_STUB

#include "ESP8266WiFi.h"

#include <map>
#include <string>

class ESP8266WebServer {
 public:
  typedef void (*Rota)();

  explicit ESP8266WebServer(int) {}
  void on(const char* caminho, Rota r) { rotas_[caminho] = r; }
  void onNotFound(Rota r) { nao_encontrada_ = r; }
  void begin() {}
  void handleClient() {}

  String arg(const char* nome) {
    std::map<std::string, std::string>::iterator it = args_.find(nome);
    return it == args_.end() ? String("") : String(it->second.c_str());
  }
  void sendHeader(const char*, const char*) {}
  void send(int codigo, const char* tipo, const String& corpo) {
    send(codigo, tipo, corpo.c_str());
  }
  void send(int codigo, const char* tipo, const char* corpo) {
    codigo_ = codigo;
    tipo_ = tipo;
    corpo_ = corpo;
  }
  void send_P(int codigo, const char* tipo, const char* corpo) {
    send(codigo, tipo, corpo);
  }

  // --- Usado pelos testes ---------------------------------------------------
  int requisitar(const char* caminho, const char* nome_arg = 0,
                 const char* valor_arg = 0) {
    args_.clear();
    if (nome_arg) args_[nome_arg] = valor_arg;
    codigo_ = 0;
    corpo_.clear();
    std::map<std::string, Rota>::iterator it = rotas_.find(caminho);
    if (it != rotas_.end()) it->second();
    else if (nao_encontrada_) nao_encontrada_();
    return codigo_;
  }
  const std::string& corpo() const { return corpo_; }
  const std::string& tipo() const { return tipo_; }

 private:
  std::map<std::string, Rota> rotas_;
  std::map<std::string, std::string> args_;
  Rota nao_encontrada_ = 0;
  int codigo_ = 0;
  std::string tipo_;
  std::string corpo_;
};

#endif  // ESP8266WEBSERVER_H_STUB
