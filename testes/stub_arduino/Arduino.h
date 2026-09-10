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
  virtual int available() { return 0; }
  virtual int read() { return -1; }
  virtual size_t write(uint8_t) { return 1; }
  virtual void flush() {}

  void print(const char* s) { if (registrar_) printf("%s", s); }
  void print(char c) { if (registrar_) printf("%c", c); }
  void print(int v) { if (registrar_) printf("%d", v); }
  void print(unsigned int v) { if (registrar_) printf("%u", v); }
  void print(long v) { if (registrar_) printf("%ld", v); }
  void print(unsigned long v) { if (registrar_) printf("%lu", v); }
  void println() { if (registrar_) printf("\n"); }
  void println(const char* s) { if (registrar_) printf("%s\n", s); }
  void println(char c) { if (registrar_) printf("%c\n", c); }
  void println(int v) { if (registrar_) printf("%d\n", v); }
  void println(unsigned long v) { if (registrar_) printf("%lu\n", v); }

  // Versoes com base numerica, como Serial.print(valor, HEX).
  void print(int v, int base) {
    if (!registrar_) return;
    if (base == HEX) printf("%X", v); else printf("%d", v);
  }
  void println(int v, int base) { print(v, base); println(); }

  void silenciar(bool s) { registrar_ = !s; }

 protected:
  bool registrar_ = true;
};

class HardwareSerial : public Stream {
 public:
  void begin(long) {}
};

extern HardwareSerial Serial;
extern HardwareSerial Serial1;
extern HardwareSerial Serial3;

#endif  // ARDUINO_H_STUB
