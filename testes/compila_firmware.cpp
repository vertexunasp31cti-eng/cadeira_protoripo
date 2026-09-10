/*
 * compila_firmware.cpp - Verificacao de compilacao do firmware no PC.
 *
 * Inclui o sketch inteiro usando o stub da API do Arduino. Nao executa a
 * cadeira: serve para garantir que o codigo embarcado compila sem erro antes
 * de abrir o Arduino IDE.
 */
#include "stub_arduino/Arduino.h"

#include "../firmware/cadeira_voz/cadeira_voz.ino"

int main() {
  Serial.silenciar(true);
  setup();
  for (int i = 0; i < 50; ++i) {
    relogio_simulado += 10;
    loop();
  }
  printf("Firmware compila e o loop principal executa sem travar.\n");
  return 0;
}
