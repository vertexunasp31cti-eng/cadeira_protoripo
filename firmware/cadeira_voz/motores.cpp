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
    // IN1 = IN2 = LOW. Com o ENA/ENB vindo do Arduino (EN em LOW) o motor fica
    // em roda livre; com o EN preso no 5V o L298 freia o motor.
    digitalWrite(pino_a, LOW);
    digitalWrite(pino_b, LOW);
    analogWrite(pino_en, 0);
    return;
  }

  if (ENA_ENB_NO_5V) {
    // O EN esta sempre ligado, entao a velocidade vai no pino IN do sentido
    // escolhido, e o outro IN fica em LOW. Na parte baixa do PWM os dois IN
    // ficam em LOW e o L298 freia; na media, a velocidade fica proporcional.
    // O outro IN vai para LOW antes, para os dois nunca ficarem em HIGH.
    if (valor > 0) {
      digitalWrite(pino_b, LOW);
      analogWrite(pino_a, abs(valor));
    } else {
      digitalWrite(pino_a, LOW);
      analogWrite(pino_b, abs(valor));
    }
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
