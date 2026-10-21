/*
 * EEPROM.h - Stub da EEPROM emulada do ESP8266, para os testes no PC.
 *
 * Guarda os bytes num vetor em memoria. reiniciarPlaca() nos testes simula
 * desligar e ligar: o conteudo do vetor sobrevive, como na flash de verdade.
 */
#ifndef EEPROM_H_STUB
#define EEPROM_H_STUB

#include <string.h>

class EEPROMClass {
 public:
  EEPROMClass() { memset(dados_, 0xFF, sizeof(dados_)); }  // flash nova
  void begin(unsigned int) {}
  template <typename T>
  T& get(int endereco, T& valor) {
    memcpy(&valor, dados_ + endereco, sizeof(T));
    return valor;
  }
  template <typename T>
  const T& put(int endereco, const T& valor) {
    memcpy(dados_ + endereco, &valor, sizeof(T));
    return valor;
  }
  bool commit() { ++gravacoes; return true; }
  int gravacoes = 0;

 private:
  unsigned char dados_[512];
};

extern EEPROMClass EEPROM;

#endif  // EEPROM_H_STUB
