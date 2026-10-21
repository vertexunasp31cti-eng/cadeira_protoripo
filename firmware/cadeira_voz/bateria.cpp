#include "bateria.h"

EstimadorBateria::EstimadorBateria()
    : iniciado_(false),
      ligada_(false),
      filtrado_mv_(0),
      t_amostra_(0),
      t_parada_desde_(0),
      estava_em_movimento_(false) {}

long milivoltsDaLeitura(int leitura_adc) {
  if (leitura_adc < 0) leitura_adc = 0;
  if (leitura_adc > 1023) leitura_adc = 1023;
  long mv_pino = (long)leitura_adc * cfg_bat::REFERENCIA_MV / 1023;
  return mv_pino * cfg_bat::DIVISOR_NUM / cfg_bat::DIVISOR_DEN;
}

// Tensao em repouso de duas celulas de litio em serie, de cheia a vazia.
// Valores tipicos de celula 18650 (4,20 V a 3,30 V por celula), multiplicados
// por dois. Entre dois pontos a porcentagem e interpolada em linha reta.
namespace {
struct PontoCurva {
  int mv;
  int pct;
};
const PontoCurva CURVA_2S[] = {
  {8400, 100}, {8200, 90}, {8000, 80}, {7860, 70}, {7740, 60}, {7640, 50},
  {7560, 40}, {7480, 30}, {7380, 20}, {7200, 10}, {6900, 5},  {6600, 0},
};
const int PONTOS_CURVA = sizeof(CURVA_2S) / sizeof(CURVA_2S[0]);
}  // namespace

int percentualLitio2S(int mv) {
  if (mv >= CURVA_2S[0].mv) return 100;
  if (mv <= CURVA_2S[PONTOS_CURVA - 1].mv) return 0;
  for (int i = 1; i < PONTOS_CURVA; ++i) {
    if (mv >= CURVA_2S[i].mv) {
      const PontoCurva& alto = CURVA_2S[i - 1];
      const PontoCurva& baixo = CURVA_2S[i];
      return baixo.pct + (long)(mv - baixo.mv) * (alto.pct - baixo.pct) /
                             (alto.mv - baixo.mv);
    }
  }
  return 0;
}

void EstimadorBateria::amostrar(int leitura_adc, bool em_movimento,
                                unsigned long agora_ms) {
  if (em_movimento) {
    estava_em_movimento_ = true;
  } else if (estava_em_movimento_) {
    estava_em_movimento_ = false;
    t_parada_desde_ = agora_ms;  // acabou de parar: a tensao ainda vai subir
  }

  if (iniciado_ && (agora_ms - t_amostra_) < cfg_bat::AMOSTRA_MS) return;
  t_amostra_ = agora_ms;

  long mv = milivoltsDaLeitura(leitura_adc);
  if (mv < cfg_bat::SEM_BATERIA_MV) {
    ligada_ = false;
    iniciado_ = false;  // quando religar, comeca do zero
    return;
  }

  if (!iniciado_) {
    // Primeira leitura valida vale inteira, para nao subir de 0 devagar.
    filtrado_mv_ = mv;
    iniciado_ = true;
    ligada_ = true;
    return;
  }

  bool estavel = !em_movimento &&
                 (agora_ms - t_parada_desde_) >= cfg_bat::ESTABILIZAR_MS;
  int peso = estavel ? cfg_bat::PESO_PARADA : cfg_bat::PESO_MOVIMENTO;
  filtrado_mv_ += (mv - filtrado_mv_) / peso;
}

int EstimadorBateria::percentual() const {
  if (!ligada_) return -1;
  return percentualLitio2S((int)filtrado_mv_);
}
