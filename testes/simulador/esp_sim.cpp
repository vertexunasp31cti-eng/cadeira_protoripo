/*
 * esp_sim.cpp - Firmware do ESP8266 rodando no PC, para o simulador.
 *
 * E o esp_wifi.ino de verdade (com a ponte.cpp e o app do pagina.h),
 * compilado com os substitutos das bibliotecas do ESP8266 e com relogio
 * real. O servidor.py recebe as requisicoes HTTP do navegador e repassa para
 * ca; o servidor web do stub chama as mesmas rotas que rodam na placa.
 *
 * Conversa com o servidor.py pela entrada e saida padrao, uma mensagem por
 * linha:
 *
 *   entrada  S <hex>                         bytes que chegam do Mega
 *            H <id> <metodo> <hex caminho> <hex corpo>
 *                                            requisicao HTTP (caminho com a
 *                                            query string; corpo do POST)
 *   saida    S <hex>                         bytes que o ESP escreveu para o
 *                                            Mega
 *            R <id> <status> <hex tipo> <hex cabecalhos> <hex corpo>
 *                                            resposta; cabecalhos no formato
 *                                            "Nome: valor\n"
 *
 * Compilar: make -C testes simulador
 */
#include "../stub_esp8266/Arduino.h"

#include "../../firmware/esp_wifi/esp_wifi.ino"

#include <poll.h>
#include <time.h>
#include <unistd.h>

#include <sstream>
#include <string>

static unsigned long agoraReal() {
  struct timespec t;
  clock_gettime(CLOCK_MONOTONIC, &t);
  return (unsigned long)t.tv_sec * 1000UL + (unsigned long)(t.tv_nsec / 1000000L);
}

static std::string paraHex(const std::string& s) {
  static const char D[] = "0123456789abcdef";
  std::string h;
  for (size_t i = 0; i < s.size(); ++i) {
    unsigned char b = (unsigned char)s[i];
    h += D[b >> 4];
    h += D[b & 15];
  }
  return h;
}

static std::string deHex(const std::string& h) {
  std::string s;
  for (size_t i = 0; i + 1 < h.size(); i += 2) {
    s += (char)strtol(h.substr(i, 2).c_str(), NULL, 16);
  }
  return s;
}

static HTTPMethod metodo(const std::string& m) {
  if (m == "GET") return HTTP_GET;
  if (m == "HEAD") return HTTP_HEAD;
  if (m == "POST") return HTTP_POST;
  if (m == "PUT") return HTTP_PUT;
  if (m == "PATCH") return HTTP_PATCH;
  if (m == "DELETE") return HTTP_DELETE;
  if (m == "OPTIONS") return HTTP_OPTIONS;
  return HTTP_ANY;
}

static void tratarMensagem(const std::string& m) {
  if (m.size() < 3 || m[1] != ' ') return;
  if (m[0] == 'S') {
    std::string bytes = deHex(m.substr(2));
    Serial.injetar(bytes.data(), bytes.size());
    return;
  }
  if (m[0] != 'H') return;

  std::istringstream campos(m.substr(2));
  std::string id, nome_metodo, caminho_hex, corpo_hex;
  campos >> id >> nome_metodo >> caminho_hex >> corpo_hex;
  std::string caminho = deHex(caminho_hex);
  std::string corpo = deHex(corpo_hex);

  ESP8266WebServer::Resposta r =
      servidor.simular(metodo(nome_metodo), caminho.c_str(), corpo.c_str());
  std::string cabecalhos;
  for (size_t i = 0; i < r.cabecalhos.size(); ++i) {
    cabecalhos += r.cabecalhos[i].first + ": " + r.cabecalhos[i].second + "\n";
  }
  // Hex vazio vira "-" para a linha ter sempre o mesmo numero de campos.
  std::string tipo = paraHex(r.tipo);
  std::string cab = paraHex(cabecalhos);
  std::string cor = paraHex(r.corpo);
  printf("R %s %d %s %s %s\n", id.c_str(), r.status,
         tipo.empty() ? "-" : tipo.c_str(), cab.empty() ? "-" : cab.c_str(),
         cor.empty() ? "-" : cor.c_str());
}

int main() {
  setvbuf(stdout, NULL, _IOLBF, 0);
  const unsigned long base = agoraReal();
  relogio_simulado = 0;
  setup();

  std::string entrada;
  for (;;) {
    struct pollfd p;
    p.fd = 0;
    p.events = POLLIN;
    if (poll(&p, 1, 1) > 0) {
      char buf[8192];
      ssize_t n = read(0, buf, sizeof(buf));
      if (n <= 0) break;  // o servidor.py fechou: acabou a simulacao
      entrada.append(buf, (size_t)n);
      size_t fim;
      while ((fim = entrada.find('\n')) != std::string::npos) {
        relogio_simulado = agoraReal() - base;
        tratarMensagem(entrada.substr(0, fim));
        entrada.erase(0, fim + 1);
      }
    }

    relogio_simulado = agoraReal() - base;
    loop();

    if (!Serial.enviado().empty()) {
      printf("S %s\n", paraHex(Serial.enviado()).c_str());
      Serial.limparEnviado();
    }
  }
  return 0;
}
