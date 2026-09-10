#include "motores.h"

#include "config.h"

void Motores::iniciar() {
  pinMode(PINO_ENA, OUTPUT);
  pinMode(PINO_IN1, OUTPUT);
  pinMode(PINO_IN2, OUTPUT);
  pinMode(PINO_ENB, OUTPUT);
  pinMode(PINO_IN3, OUTPUT);
  pinMode(PINO_IN4, OUTPUT);
  desligar();
}

void Motores::desligar() {
  analogWrite(PINO_ENA, 0);
  analogWrite(PINO_ENB, 0);
  digitalWrite(PINO_IN1, LOW);
  digitalWrite(PINO_IN2, LOW);
  digitalWrite(PINO_IN3, LOW);
  digitalWrite(PINO_IN4, LOW);
}

void Motores::acionar(uint8_t pino_en, uint8_t pino_a, uint8_t pino_b,
                      int valor, bool inverter) {
  if (inverter) valor = -valor;
  valor = constrain(valor, -255, 255);

  if (valor == 0) {
    // IN1 = IN2 = LOW deixa o motor em roda livre. Nao usamos frenagem
    // eletrica para nao derrapar nem sobrecarregar a ponte.
    digitalWrite(pino_a, LOW);
    digitalWrite(pino_b, LOW);
    analogWrite(pino_en, 0);
    return;
  }

  if (valor > 0) {
    digitalWrite(pino_a, HIGH);
    digitalWrite(pino_b, LOW);
  } else {
    digitalWrite(pino_a, LOW);
    digitalWrite(pino_b, HIGH);
  }
  analogWrite(pino_en, abs(valor));
}

void Motores::aplicar(int esquerdo, int direito) {
  acionar(PINO_ENA, PINO_IN1, PINO_IN2, esquerdo, INVERTER_MOTOR_ESQ);
  acionar(PINO_ENB, PINO_IN3, PINO_IN4, direito, INVERTER_MOTOR_DIR);
}
