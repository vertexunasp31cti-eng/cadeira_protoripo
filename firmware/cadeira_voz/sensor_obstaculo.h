/*
 * sensor_obstaculo.h - Sensor infravermelho reflexivo com filtro de ruido.
 *
 * A leitura crua do modulo IR oscila com iluminacao ambiente e vibracao do
 * carrinho. Uma unica leitura falsa pode frear a cadeira sem motivo, entao a
 * mudanca de estado so e aceita depois de se manter estavel.
 */
#ifndef SENSOR_OBSTACULO_H
#define SENSOR_OBSTACULO_H

#include <Arduino.h>

class SensorObstaculo {
 public:
  void iniciar(unsigned long agora_ms);

  // Chame a cada volta do loop. Retorna o estado ja filtrado.
  bool atualizar(unsigned long agora_ms);

  bool detectado() const { return estavel_; }

 private:
  bool estavel_;
  bool candidato_;
  unsigned long t_mudanca_;
};

#endif  // SENSOR_OBSTACULO_H
