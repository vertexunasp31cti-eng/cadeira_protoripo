/*
 * motores.h - Camada de acesso a ponte H dupla L298N.
 */
#ifndef MOTORES_H
#define MOTORES_H

#include <Arduino.h>

class Motores {
 public:
  void iniciar();

  // Faixa -255 a 255. Negativo gira para tras, zero deixa em roda livre.
  void aplicar(int esquerdo, int direito);

  // Corta as duas saidas imediatamente.
  void desligar();

 private:
  void acionar(uint8_t pino_en, uint8_t pino_a, uint8_t pino_b, int valor,
               bool inverter);
};

#endif  // MOTORES_H
