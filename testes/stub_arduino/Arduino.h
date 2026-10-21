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
#include <string.h>

#include <string>

#define HIGH 1
#define LOW 0
#define INPUT 0
#define OUTPUT 1
#define INPUT_PULLUP 2

#define F(x) (x)
#define PROGMEM

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

void pinMode(uint8_t pino, uint8_t modo);
int digitalRead(uint8_t pino);
void digitalWrite(uint8_t pino, uint8_t valor);
void analogWrite(uint8_t pino, int valor);
unsigned long millis();
void delay(unsigned long ms);

// --- Serial ----------------------------------------------------------------
class Stream {
 public:
  virtual ~Stream() {}
  virtual int available() { return (int)entrada_.size(); }
  virtual int read() {
    if (entrada_.empty()) return -1;
    int c = (unsigned char)entrada_[0];
    entrada_.erase(0, 1);
    return c;
  }
  virtual size_t write(uint8_t c) { saida_ += (char)c; return 1; }
  virtual void flush() {}

  void print(const char* s) { emitir(s); }
  void print(char c) { char b[2] = {c, 0}; emitir(b); }
  void print(int v) { formatar("%d", v); }
  void print(unsigned int v) { formatar("%u", v); }
  void print(long v) { formatar("%ld", v); }
  void print(unsigned long v) { formatar("%lu", v); }
  void println() { emitir("\n"); }
  void println(const char* s) { print(s); println(); }
  void println(char c) { print(c); println(); }
  void println(int v) { print(v); println(); }
  void println(unsigned long v) { print(v); println(); }

  // Versoes com base numerica, como Serial.print(valor, HEX).
  void print(int v, int base) {
    if (base == HEX) formatar("%X", v); else formatar("%d", v);
  }
  void println(int v, int base) { print(v, base); println(); }

  void silenciar(bool s) { registrar_ = !s; }

  // Usado pelos testes: simula bytes chegando e le o que o sketch enviou.
  void injetar(const char* s) { entrada_ += s; }
  std::string retirarSaida() { std::string s = saida_; saida_.clear(); return s; }

 protected:
  template <typename T>
  void formatar(const char* fmt, T v) {
    char b[24];
    snprintf(b, sizeof(b), fmt, v);
    emitir(b);
  }
  void emitir(const char* s) {
    saida_ += s;
    if (registrar_) printf("%s", s);
  }

  bool registrar_ = true;
  std::string entrada_;
  std::string saida_;
};

class HardwareSerial : public Stream {
 public:
  void begin(long) {}
};

extern HardwareSerial Serial;
extern HardwareSerial Serial1;
extern HardwareSerial Serial3;

#endif  // ARDUINO_H_STUB
