#include "sensor_obstaculo.h"

#include "config.h"

void SensorObstaculo::iniciar(unsigned long agora_ms) {
  pinMode(PINO_SENSOR_IR, INPUT);
  bool leitura = (digitalRead(PINO_SENSOR_IR) == NIVEL_IR_DETECTADO);
  estavel_ = leitura;
  candidato_ = leitura;
  t_mudanca_ = agora_ms;
}

bool SensorObstaculo::atualizar(unsigned long agora_ms) {
  bool leitura = (digitalRead(PINO_SENSOR_IR) == NIVEL_IR_DETECTADO);

  if (leitura != candidato_) {
    candidato_ = leitura;
    t_mudanca_ = agora_ms;
  } else if (leitura != estavel_ &&
             (agora_ms - t_mudanca_) >= DEBOUNCE_IR_MS) {
    estavel_ = leitura;
  }
  return estavel_;
}
