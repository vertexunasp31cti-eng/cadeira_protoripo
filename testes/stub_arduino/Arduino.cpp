#include "Arduino.h"

int pinos_simulados[70] = {0};
unsigned long relogio_simulado = 0;

HardwareSerial Serial;
HardwareSerial Serial1;
HardwareSerial Serial3;

void pinMode(uint8_t pino, uint8_t modo) {
  // INPUT_PULLUP deixa o pino em HIGH, igual ao hardware real. E o que faz o
  // botao de emergencia ausente nunca disparar.
  if (modo == INPUT_PULLUP && pino < 70) pinos_simulados[pino] = HIGH;
}

int digitalRead(uint8_t pino) {
  return pino < 70 ? pinos_simulados[pino] : LOW;
}

void digitalWrite(uint8_t pino, uint8_t valor) {
  if (pino < 70) pinos_simulados[pino] = valor;
}

void analogWrite(uint8_t pino, int valor) {
  if (pino < 70) pinos_simulados[pino] = valor;
}

// No PC nao existe interrupcao de timer para avancar o relogio. Cada consulta
// avanca 1 ms, o que garante que esperas com tempo limite sempre terminem
// durante a verificacao.
unsigned long millis() { return ++relogio_simulado; }

void delay(unsigned long ms) { relogio_simulado += ms; }
