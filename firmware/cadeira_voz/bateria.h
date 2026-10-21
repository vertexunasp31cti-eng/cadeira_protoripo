/*
 * bateria.h - Estimativa da carga da bateria dos motores.
 *
 * O Mega mede a tensao da bateria por um divisor resistivo no pino A0 (veja
 * docs/ligacoes.md). A carga em porcentagem sai de uma curva de descarga, por
 * isso e uma ESTIMATIVA: a tensao de uma bateria de litio cai de forma pouco
 * linear e muda com a temperatura e com a idade das celulas.
 *
 * A tensao tambem afunda enquanto os motores puxam corrente. Para a
 * porcentagem nao despencar a cada partida e voltar quando a cadeira para, o
 * filtro confia muito mais nas leituras feitas com a cadeira parada.
 *
 * Igual a controle.h, este arquivo nao depende da biblioteca Arduino e e
 * testado no PC (testes/test_bateria.cpp).
 */
#ifndef BATERIA_H
#define BATERIA_H

#include <stdint.h>

namespace cfg_bat {

// Divisor: R1 = 20k entre o + da bateria e A0, R2 = 10k entre A0 e GND.
// Tensao na bateria = tensao em A0 * (R1 + R2) / R2 = 3x.
// 8,4 V (bateria cheia) chegam como 2,8 V, longe do limite de 5 V do pino.
const long DIVISOR_NUM = 3;
const long DIVISOR_DEN = 1;

// Referencia do conversor analogico do Mega (5 V pela USB ou regulador).
// Se a leitura sair sempre um pouco diferente do multimetro, ajuste aqui.
const long REFERENCIA_MV = 5000;

// Abaixo disso nao ha bateria ligada no divisor (ou o fio soltou).
const int SEM_BATERIA_MV = 3000;

// Intervalo entre amostras e quanto tempo parada para a leitura valer mais.
const unsigned long AMOSTRA_MS = 100;
const unsigned long ESTABILIZAR_MS = 1500;

// Pesos do filtro: 1/8 da nova leitura parada, 1/512 em movimento. Em
// movimento a leitura quase nao conta, porque mede o afundamento causado
// pelos motores e nao a carga; ela so pesa em trajetos longos sem parar.
const int PESO_PARADA = 8;
const int PESO_MOVIMENTO = 512;

}  // namespace cfg_bat

class EstimadorBateria {
 public:
  EstimadorBateria();

  // Leitura crua do analogRead (0 a 1023). Chame a cada volta do loop: a
  // propria classe respeita o intervalo de amostragem.
  void amostrar(int leitura_adc, bool em_movimento, unsigned long agora_ms);

  bool ligada() const { return ligada_; }
  int milivolts() const { return ligada_ ? (int)filtrado_mv_ : 0; }

  // 0 a 100, ou -1 se nao ha bateria ligada no divisor.
  int percentual() const;

 private:
  bool iniciado_;
  bool ligada_;
  long filtrado_mv_;
  unsigned long t_amostra_;
  unsigned long t_parada_desde_;
  bool estava_em_movimento_;
};

// Converte a leitura do pino A0 em milivolts na bateria.
long milivoltsDaLeitura(int leitura_adc);

// Curva de descarga de bateria de litio 2S (duas celulas, 7,4 V nominal).
int percentualLitio2S(int milivolts);

#endif  // BATERIA_H
