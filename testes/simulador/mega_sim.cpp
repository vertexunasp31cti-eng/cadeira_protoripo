/*
 * mega_sim.cpp - Firmware do Mega rodando no PC, para o simulador.
 *
 * E o cadeira_voz.ino de verdade, compilado com o stub da API do Arduino e
 * com relogio real. O servidor.py liga este programa ao esp_sim, que roda o
 * firmware do ESP8266 do mesmo jeito: o app no navegador conversa com o
 * codigo real dos dois lados.
 *
 * Conversa com o servidor.py pela entrada e saida padrao, uma mensagem por
 * linha:
 *
 *   entrada  S <hex>   bytes que chegam na Serial3 (vindos do ESP)
 *            O <0|1>   sensor de obstaculo ve algo a frente
 *            E <0|1>   botao de emergencia apertado
 *   saida    S <hex>   bytes que o Mega escreveu na Serial3
 *
 * Os bytes vao em hexadecimal porque o lixo de boot do ESP, que o simulador
 * tambem reproduz, nao e texto. A Serial de depuracao sai na saida de erro.
 *
 * Compilar: make -C testes simulador
 */
#include "../stub_arduino/Arduino.h"

#include "../../firmware/cadeira_voz/cadeira_voz.ino"

#include <poll.h>
#include <time.h>
#include <unistd.h>

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

static void tratarMensagem(const std::string& m) {
  if (m.size() < 3 || m[1] != ' ') return;
  const std::string resto = m.substr(2);
  switch (m[0]) {
    case 'S':
      Serial3.injetar(deHex(resto));
      break;
    case 'O':
      pinos_simulados[PINO_SENSOR_IR] =
          resto == "1" ? NIVEL_IR_DETECTADO : !NIVEL_IR_DETECTADO;
      break;
    case 'E':
      pinos_simulados[PINO_BOTAO_EMERG] =
          resto == "1" ? NIVEL_BOTAO_ACIONADO : !NIVEL_BOTAO_ACIONADO;
      break;
  }
}

int main() {
  setvbuf(stdout, NULL, _IOLBF, 0);
  Serial.silenciar(true);
  Serial1.silenciar(true);
  Serial3.silenciar(true);
  pinos_simulados[PINO_SENSOR_IR] = !NIVEL_IR_DETECTADO;  // caminho livre

  // O setup() roda com o relogio automatico do stub: sem o modulo de voz ele
  // passaria uns 4 s esperando resposta, e aqui isso leva um instante.
  setup();

  // Dai em diante, relogio real, continuando de onde o setup() parou e sem
  // nunca voltar: o firmware subtrai instantes sem sinal e um relogio que
  // volta viraria um intervalo enorme.
  relogio_automatico = false;
  const unsigned long base = agoraReal() - relogio_simulado;

  std::string entrada;
  for (;;) {
    struct pollfd p;
    p.fd = 0;
    p.events = POLLIN;
    if (poll(&p, 1, 1) > 0) {
      char buf[4096];
      ssize_t n = read(0, buf, sizeof(buf));
      if (n <= 0) break;  // o servidor.py fechou: acabou a simulacao
      entrada.append(buf, (size_t)n);
      size_t fim;
      while ((fim = entrada.find('\n')) != std::string::npos) {
        tratarMensagem(entrada.substr(0, fim));
        entrada.erase(0, fim + 1);
      }
    }

    unsigned long t = agoraReal() - base;
    if (t > relogio_simulado) relogio_simulado = t;
    loop();

    if (!Serial3.saida.empty()) {
      printf("S %s\n", paraHex(Serial3.saida).c_str());
      Serial3.saida.clear();
    }
    if (!Serial.saida.empty()) {
      fputs(Serial.saida.c_str(), stderr);
      Serial.saida.clear();
    }
    Serial1.saida.clear();
  }
  return 0;
}
