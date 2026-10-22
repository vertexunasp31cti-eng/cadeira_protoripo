/*
 * Arduino.h - Stub minimo da API do Arduino.
 *
 * NAO faz parte do firmware embarcado. Serve so para compilar o codigo no PC
 * e pegar erro de sintaxe, de tipo e de chamada errada sem depender da placa
 * nem do Arduino IDE. Veja testes/Makefile, alvo "verificar".
 */
#ifndef ARDUINO_H_STUB
#define ARDUINO_H_STUB

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <deque>
#include <string>
#include <utility>

#define HIGH 1
#define LOW 0
#define INPUT 0
#define OUTPUT 1
#define INPUT_PULLUP 2

#define F(x) (x)

#define DEC 10
#define HEX 16
#define BIN 2

inline int constrain(int v, int lo, int hi) {
  return v < lo ? lo : (v > hi ? hi : v);
}

using std::abs;

// --- Estado simulado dos pinos ---------------------------------------------
extern int pinos_simulados[70];
extern unsigned long relogio_simulado;

// Com true (o padrao), cada millis() avanca 1 ms: esperas com tempo limite,
// como as do modulo de voz no setup(), sempre terminam. Os ensaios e o
// simulador desligam e controlam o relogio eles mesmos.
extern bool relogio_automatico;

void pinMode(uint8_t pino, uint8_t modo);
int digitalRead(uint8_t pino);
void digitalWrite(uint8_t pino, uint8_t valor);
void analogWrite(uint8_t pino, int valor);
unsigned long millis();
void delay(unsigned long ms);

// --- Serial ----------------------------------------------------------------
// Alem de compilar, a serial do stub serve aos ensaios do sketch inteiro
// (testes/ensaio_firmware.cpp) e ao simulador (testes/simulador/):
//   - injetar() poe bytes "no fio", com o instante em que chegam. Como na
//     UART do Mega, so entram depois do begin() e o buffer de recepcao guarda
//     63 bytes: o que chega com ele cheio se perde.
//   - tudo que o sketch escreve fica em saida, alem de ir para a tela quando
//     a serial nao esta silenciada.
//   - livre_tx imita o espaco no buffer de transmissao (availableForWrite).
class Stream {
 public:
  virtual ~Stream() {}
  virtual int available() {
    receber();
    return (int)rx.size();
  }
  virtual int read() {
    receber();
    if (rx.empty()) return -1;
    int b = rx.front();
    rx.pop_front();
    ++lidos;
    return b;
  }
  virtual size_t write(uint8_t c) {
    saida += (char)c;
    return 1;
  }
  virtual void flush() {}
  // Espaco livre no buffer de transmissao. No Mega sao no maximo 63 bytes.
  virtual int availableForWrite() { return livre_tx; }

  void print(const char* s) { escrever(s); }
  void print(char c) { char t[2] = {c, '\0'}; escrever(t); }
  void print(int v) { formatar("%d", v); }
  void print(unsigned int v) { formatar("%u", v); }
  void print(long v) { formatar("%ld", v); }
  void print(unsigned long v) { formatar("%lu", v); }
  void println() { escrever("\n"); }
  void println(const char* s) { print(s); println(); }
  void println(char c) { print(c); println(); }
  void println(int v) { print(v); println(); }
  void println(unsigned long v) { print(v); println(); }

  // Versoes com base numerica, como Serial.print(valor, HEX).
  void print(int v, int base) { formatar(base == HEX ? "%X" : "%d", v); }
  void println(int v, int base) { print(v, base); println(); }

  void silenciar(bool s) { registrar_ = !s; }

  // --- Ajudantes de ensaio e simulacao --------------------------------------
  // Bytes que chegam agora, ou no instante t (relogio_simulado).
  void injetar(const std::string& s) { injetar(s, relogio_simulado); }
  void injetar(const std::string& s, unsigned long t) {
    for (size_t i = 0; i < s.size(); ++i) {
      no_fio.push_back(std::make_pair(t, (uint8_t)s[i]));
    }
  }
  // Esquece o que estava no fio, no buffer e na saida.
  void zerar() {
    no_fio.clear();
    rx.clear();
    saida.clear();
    lidos = 0;
    perdidos = 0;
  }

  std::deque<std::pair<unsigned long, uint8_t> > no_fio;
  std::deque<uint8_t> rx;
  std::string saida;
  bool iniciada = false;
  int livre_tx = 63;
  unsigned long lidos = 0;
  unsigned long perdidos = 0;

 protected:
  bool registrar_ = true;

 private:
  void receber() {
    while (!no_fio.empty() && no_fio.front().first <= relogio_simulado) {
      if (iniciada && rx.size() < 63) {
        rx.push_back(no_fio.front().second);
      } else {
        ++perdidos;
      }
      no_fio.pop_front();
    }
  }
  void escrever(const char* s) {
    saida += s;
    if (registrar_) printf("%s", s);
  }
  template <typename T>
  void formatar(const char* formato, T v) {
    char t[24];
    snprintf(t, sizeof(t), formato, v);
    escrever(t);
  }
};

class HardwareSerial : public Stream {
 public:
  void begin(long) { iniciada = true; }
};

extern HardwareSerial Serial;
extern HardwareSerial Serial1;
extern HardwareSerial Serial3;

#endif  // ARDUINO_H_STUB
