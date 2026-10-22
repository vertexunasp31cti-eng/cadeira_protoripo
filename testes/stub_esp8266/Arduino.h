/*
 * Arduino.h - Stub minimo do core ESP8266 para Arduino (3.x).
 *
 * NAO faz parte do firmware. Serve para compilar o firmware/esp_wifi no PC,
 * pegar erro de sintaxe, de tipo e de chamada errada, e exercitar as rotas
 * HTTP com requisicoes simuladas. Veja testes/compila_esp.cpp.
 *
 * Independente do stub_arduino, que imita o Mega: a Serial do ESP tem
 * setRxBufferSize, o servidor web precisa de String e o relogio precisa ser
 * controlado pelo teste, sem avancar sozinho a cada millis().
 *
 * As assinaturas copiam as do core real (mesmos tipos e valores padrao):
 * codigo que compila aqui deve compilar na placa. So existe o que o
 * esp_wifi.ino usa, mais ajudantes de teste no fim de cada classe.
 */
#ifndef ARDUINO_H_STUB_ESP8266
#define ARDUINO_H_STUB_ESP8266

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <string>

// --- Memoria de programa ----------------------------------------------------
// No ESP8266 o PROGMEM poe o dado na flash; no PC nao faz nada.
#define PROGMEM
typedef const char* PGM_P;
#define PSTR(s) (s)
inline size_t strlen_P(PGM_P s) { return strlen(s); }
inline void* memcpy_P(void* destino, const void* origem, size_t n) {
  return memcpy(destino, origem, n);
}

// --- Tempo ------------------------------------------------------------------
// O relogio so anda quando o teste manda (relogio_simulado += ...), para os
// testes de tempo limite serem exatos.
extern unsigned long relogio_simulado;
extern unsigned long chamadas_delay;  // o esp_wifi.ino nao pode usar delay()
extern unsigned long chamadas_yield;

unsigned long millis();
void delay(unsigned long ms);
void yield();

// --- String -----------------------------------------------------------------
// So o pedaco da String do Arduino que o sketch usa. Construtor implicito a
// partir de const char*, como no core.
class String {
 public:
  String(const char* cstr = nullptr) : s_(cstr != nullptr ? cstr : "") {}
  String(const String& outra) = default;
  String& operator=(const String& outra) = default;

  const char* c_str() const { return s_.c_str(); }
  unsigned int length() const { return (unsigned int)s_.size(); }
  long toInt() const { return atol(s_.c_str()); }

  bool equals(const String& outra) const { return s_ == outra.s_; }
  bool equals(const char* cstr) const { return s_ == (cstr ? cstr : ""); }
  bool operator==(const String& outra) const { return equals(outra); }
  bool operator==(const char* cstr) const { return equals(cstr); }
  bool operator!=(const String& outra) const { return !equals(outra); }
  bool operator!=(const char* cstr) const { return !equals(cstr); }

  String& operator+=(const String& outra) {
    s_ += outra.s_;
    return *this;
  }
  String& operator+=(const char* cstr) {
    if (cstr != nullptr) s_ += cstr;
    return *this;
  }
  String& operator+=(char c) {
    s_ += c;
    return *this;
  }

 private:
  std::string s_;
};

extern const String emptyString;

// --- Serial -----------------------------------------------------------------
// UART0 do ESP, ligada direto na Serial3 do Mega. O teste injeta o que o Mega
// "mandou" e confere byte a byte o que o ESP escreveu.
class HardwareSerial {
 public:
  explicit HardwareSerial(int uart_nr);

  size_t setRxBufferSize(size_t size);
  void begin(unsigned long baud);
  void end();
  void setDebugOutput(bool en);

  int available();
  int peek();
  int read();
  void flush();

  size_t write(uint8_t c);
  size_t write(const uint8_t* buffer, size_t size);
  size_t write(const char* buffer, size_t size) {
    return write((const uint8_t*)buffer, size);
  }
  size_t print(const char* s) { return write(s, strlen(s)); }
  size_t println(const char* s) { return print(s) + print("\r\n"); }

  // --- Ajudantes de teste ---------------------------------------------------
  // Bytes chegando do Mega. Como na UART real, o que nao cabe no buffer de
  // recepcao se perde.
  void injetar(const char* s);
  void injetar(const char* s, size_t n);
  const std::string& enviado() const { return tx_; }
  void limparEnviado() { tx_.clear(); }

  bool iniciada() const { return iniciada_; }
  unsigned long baud() const { return baud_; }
  size_t tamanhoBufferRx() const { return rx_cap_; }
  bool bufferMudadoAntesDoBegin() const { return rx_antes_do_begin_; }
  bool depuracaoLigada() const { return depuracao_; }
  unsigned long bytesPerdidos() const { return perdidos_; }
  unsigned long escritasSemBegin() const { return escritas_sem_begin_; }

 private:
  bool iniciada_;
  unsigned long baud_;
  size_t rx_cap_;
  bool rx_antes_do_begin_;
  bool depuracao_;
  std::string rx_;
  std::string tx_;
  unsigned long perdidos_;
  unsigned long escritas_sem_begin_;
};

extern HardwareSerial Serial;

#endif  // ARDUINO_H_STUB_ESP8266
