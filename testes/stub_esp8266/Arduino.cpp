#include "Arduino.h"

unsigned long relogio_simulado = 0;
unsigned long chamadas_delay = 0;
unsigned long chamadas_yield = 0;

const String emptyString;

// UART0, a unica ligada ao Mega na placa.
HardwareSerial Serial(0);

unsigned long millis() { return relogio_simulado; }

void delay(unsigned long ms) {
  ++chamadas_delay;
  relogio_simulado += ms;
}

void yield() { ++chamadas_yield; }

// ---------------------------------------------------------------------------
// O core reserva 256 bytes de recepcao se ninguem chamar setRxBufferSize.
HardwareSerial::HardwareSerial(int)
    : iniciada_(false),
      baud_(0),
      rx_cap_(256),
      rx_antes_do_begin_(false),
      depuracao_(false),
      perdidos_(0),
      escritas_sem_begin_(0) {}

size_t HardwareSerial::setRxBufferSize(size_t size) {
  rx_cap_ = size;
  rx_antes_do_begin_ = !iniciada_;
  return size;
}

void HardwareSerial::begin(unsigned long baud) {
  iniciada_ = true;
  baud_ = baud;
}

void HardwareSerial::end() { iniciada_ = false; }

void HardwareSerial::setDebugOutput(bool en) { depuracao_ = en; }

int HardwareSerial::available() { return iniciada_ ? (int)rx_.size() : 0; }

int HardwareSerial::peek() {
  if (!iniciada_ || rx_.empty()) return -1;
  return (uint8_t)rx_[0];
}

int HardwareSerial::read() {
  if (!iniciada_ || rx_.empty()) return -1;
  int c = (uint8_t)rx_[0];
  rx_.erase(0, 1);
  return c;
}

void HardwareSerial::flush() {}

size_t HardwareSerial::write(uint8_t c) { return write(&c, 1); }

size_t HardwareSerial::write(const uint8_t* buffer, size_t size) {
  // Sem begin() a UART real nao manda nada; aqui fica registrado para o teste
  // pegar o erro.
  if (!iniciada_) {
    ++escritas_sem_begin_;
    return 0;
  }
  tx_.append((const char*)buffer, size);
  return size;
}

void HardwareSerial::injetar(const char* s) { injetar(s, strlen(s)); }

void HardwareSerial::injetar(const char* s, size_t n) {
  for (size_t i = 0; i < n; ++i) {
    if (rx_.size() < rx_cap_) {
      rx_ += s[i];
    } else {
      ++perdidos_;
    }
  }
}
