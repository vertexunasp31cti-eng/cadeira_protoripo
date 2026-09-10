/*
 * compila_treinador.cpp - Verificacao de compilacao da ferramenta de treino.
 */
#include "stub_arduino/Arduino.h"

#include "../firmware/treinar_voz/treinar_voz.ino"

int main() {
  Serial.silenciar(true);
  setup();
  for (int i = 0; i < 20; ++i) loop();
  printf("Ferramenta de treinamento compila e executa sem travar.\n");
  return 0;
}
