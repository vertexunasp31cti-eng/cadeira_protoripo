#include "controle.h"

ControleCadeira::ControleCadeira()
    : estado_(EST_PARADA),
      nivel_(cfg::NIVEL_INICIAL),
      alvo_esq_(0),
      alvo_dir_(0),
      atual_esq_(0),
      atual_dir_(0),
      obstaculo_(false),
      recusado_(false),
      motivo_(PARADA_NENHUMA),
      t_ultimo_comando_(0),
      t_inicio_giro_(0),
      t_ultima_rampa_(0) {}

void ControleCadeira::iniciar(unsigned long agora_ms) {
  estado_ = EST_PARADA;
  nivel_ = cfg::NIVEL_INICIAL;
  alvo_esq_ = alvo_dir_ = 0;
  atual_esq_ = atual_dir_ = 0;
  obstaculo_ = false;
  recusado_ = false;
  motivo_ = PARADA_NENHUMA;
  t_ultimo_comando_ = agora_ms;
  t_inicio_giro_ = agora_ms;
  t_ultima_rampa_ = agora_ms;
}

int ControleCadeira::limitar(int v, int minimo, int maximo) {
  if (v < minimo) return minimo;
  if (v > maximo) return maximo;
  return v;
}

int ControleCadeira::aproximar(int atual, int alvo, int passo) {
  if (atual < alvo) {
    atual += passo;
    if (atual > alvo) atual = alvo;
  } else if (atual > alvo) {
    atual -= passo;
    if (atual < alvo) atual = alvo;
  }
  return atual;
}

// Ao inverter o sentido, a roda obrigatoriamente passa por zero por pelo menos
// um ciclo. Sem isso a rampa pode pular de +2 para -6 e a ponte H inverte a
// polaridade com o motor ainda girando, o que gera pico de corrente e tranco.
int ControleCadeira::aproximarPorZero(int atual, int alvo, int passo) {
  int novo = aproximar(atual, alvo, passo);
  if ((atual > 0 && novo < 0) || (atual < 0 && novo > 0)) novo = 0;
  return novo;
}

int ControleCadeira::velocidadeBase() const {
  int n = limitar(nivel_, 0, cfg::TOTAL_NIVEIS - 1);
  return limitar(cfg::NIVEIS_PWM[n], cfg::PWM_MINIMO, 255);
}

int ControleCadeira::velocidadeGiro() const {
  int v = velocidadeBase() * cfg::FATOR_GIRO_PCT / 100;
  return limitar(v, cfg::PWM_MINIMO, 255);
}

void ControleCadeira::definirAlvo(int esq, int dir) {
  // Correcao de desalinhamento entre as duas rodas.
  esq = esq * cfg::AJUSTE_ESQ_PCT / 100;
  dir = dir * cfg::AJUSTE_DIR_PCT / 100;
  alvo_esq_ = limitar(esq, -255, 255);
  alvo_dir_ = limitar(dir, -255, 255);
}

void ControleCadeira::pararSuave(unsigned long agora_ms, MotivoParada motivo) {
  estado_ = EST_PARADA;
  alvo_esq_ = alvo_dir_ = 0;
  motivo_ = motivo;
  t_ultimo_comando_ = agora_ms;
}

void ControleCadeira::pararImediato(unsigned long agora_ms, MotivoParada motivo) {
  estado_ = EST_PARADA;
  alvo_esq_ = alvo_dir_ = 0;
  atual_esq_ = atual_dir_ = 0;  // corta o PWM no mesmo ciclo, sem rampa
  motivo_ = motivo;
  t_ultimo_comando_ = agora_ms;
}

void ControleCadeira::pararEmergencia(unsigned long agora_ms) {
  pararImediato(agora_ms, PARADA_EMERGENCIA);
}

bool ControleCadeira::aplicarComando(Comando c, unsigned long agora_ms) {
  recusado_ = false;

  switch (c) {
    case CMD_PARAR:
      // Parada e sempre imediata: e o comando de seguranca do usuario.
      pararImediato(agora_ms, PARADA_COMANDO);
      return true;

    case CMD_FRENTE:
      if (obstaculo_) {
        // Recusa o comando e garante que a cadeira esta parada.
        pararImediato(agora_ms, PARADA_OBSTACULO);
        recusado_ = true;
        return false;
      }
      estado_ = EST_FRENTE;
      definirAlvo(velocidadeBase(), velocidadeBase());
      motivo_ = PARADA_NENHUMA;
      t_ultimo_comando_ = agora_ms;
      return true;

    case CMD_RE:
      // Re nao e bloqueada pelo sensor: ele aponta para a frente.
      estado_ = EST_RE;
      definirAlvo(-velocidadeBase(), -velocidadeBase());
      motivo_ = PARADA_NENHUMA;
      t_ultimo_comando_ = agora_ms;
      return true;

    case CMD_ESQUERDA:
      estado_ = EST_GIRO_ESQ;
      definirAlvo(-velocidadeGiro(), velocidadeGiro());
      motivo_ = PARADA_NENHUMA;
      t_inicio_giro_ = agora_ms;
      t_ultimo_comando_ = agora_ms;
      return true;

    case CMD_DIREITA:
      estado_ = EST_GIRO_DIR;
      definirAlvo(velocidadeGiro(), -velocidadeGiro());
      motivo_ = PARADA_NENHUMA;
      t_inicio_giro_ = agora_ms;
      t_ultimo_comando_ = agora_ms;
      return true;

    case CMD_RAPIDO:
    case CMD_DEVAGAR: {
      int anterior = nivel_;
      nivel_ = limitar(nivel_ + (c == CMD_RAPIDO ? 1 : -1), 0,
                       cfg::TOTAL_NIVEIS - 1);
      t_ultimo_comando_ = agora_ms;
      // Se ja esta em movimento, o novo nivel vale imediatamente.
      if (estado_ == EST_FRENTE) {
        definirAlvo(velocidadeBase(), velocidadeBase());
      } else if (estado_ == EST_RE) {
        definirAlvo(-velocidadeBase(), -velocidadeBase());
      } else if (estado_ == EST_GIRO_ESQ) {
        definirAlvo(-velocidadeGiro(), velocidadeGiro());
      } else if (estado_ == EST_GIRO_DIR) {
        definirAlvo(velocidadeGiro(), -velocidadeGiro());
      }
      recusado_ = (nivel_ == anterior);  // ja estava no limite
      return true;
    }

    default:
      return false;
  }
}

void ControleCadeira::atualizar(unsigned long agora_ms, bool obstaculo) {
  obstaculo_ = obstaculo;

  // 1) Obstaculo a frente tem prioridade sobre tudo.
  if (obstaculo_ && estado_ == EST_FRENTE) {
    pararImediato(agora_ms, PARADA_OBSTACULO);
  }

  // 2) Giro e um pulso temporizado.
  if ((estado_ == EST_GIRO_ESQ || estado_ == EST_GIRO_DIR) &&
      (agora_ms - t_inicio_giro_) >= cfg::TEMPO_GIRO_MS) {
    pararSuave(agora_ms, PARADA_FIM_DO_GIRO);
  }

  // 3) Homem-morto: sem comando novo, a cadeira para sozinha.
  if (estado_ != EST_PARADA &&
      (agora_ms - t_ultimo_comando_) >= cfg::TEMPO_LIMITE_COMANDO_MS) {
    pararSuave(agora_ms, PARADA_TEMPO_LIMITE);
  }

  aplicarRampa(agora_ms);
}

void ControleCadeira::aplicarRampa(unsigned long agora_ms) {
  if ((agora_ms - t_ultima_rampa_) < cfg::RAMPA_INTERVALO_MS) return;
  t_ultima_rampa_ = agora_ms;

  atual_esq_ = aproximarPorZero(atual_esq_, alvo_esq_, cfg::RAMPA_PASSO);
  atual_dir_ = aproximarPorZero(atual_dir_, alvo_dir_, cfg::RAMPA_PASSO);
}

const char* nomeComando(Comando c) {
  switch (c) {
    case CMD_FRENTE: return "FRENTE";
    case CMD_RE: return "RE";
    case CMD_ESQUERDA: return "ESQUERDA";
    case CMD_DIREITA: return "DIREITA";
    case CMD_PARAR: return "PARAR";
    case CMD_RAPIDO: return "RAPIDO";
    case CMD_DEVAGAR: return "DEVAGAR";
    default: return "NENHUM";
  }
}

const char* nomeEstado(Estado e) {
  switch (e) {
    case EST_FRENTE: return "FRENTE";
    case EST_RE: return "RE";
    case EST_GIRO_ESQ: return "GIRO_ESQ";
    case EST_GIRO_DIR: return "GIRO_DIR";
    default: return "PARADA";
  }
}

const char* nomeMotivo(MotivoParada m) {
  switch (m) {
    case PARADA_COMANDO: return "comando do usuario";
    case PARADA_OBSTACULO: return "obstaculo a frente";
    case PARADA_TEMPO_LIMITE: return "tempo limite sem comando";
    case PARADA_FIM_DO_GIRO: return "fim do giro";
    case PARADA_EMERGENCIA: return "parada de emergencia";
    default: return "-";
  }
}
