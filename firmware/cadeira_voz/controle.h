/*
 * controle.h - Maquina de estados da cadeira de rodas por comando de voz.
 *
 * Este arquivo NAO depende da biblioteca Arduino de proposito: toda a logica
 * de decisao (seguranca, rampa de aceleracao, tempo limite, bloqueio por
 * obstaculo) fica aqui e pode ser compilada e testada no PC.
 * Veja testes/test_controle.cpp.
 *
 * TCC de Robotica - prototipo de cadeira de rodas com comando de voz.
 */
#ifndef CONTROLE_H
#define CONTROLE_H

#include <stdint.h>

namespace cfg {

// --- Velocidade -------------------------------------------------------------
// PWM minimo para o motor DC 3-6V vencer o atrito estatico com carga.
// Abaixo disso o motor apenas "zumbe" sem girar. Ajuste no bancada.
const int PWM_MINIMO = 90;

// Tres niveis de velocidade selecionaveis por voz ("devagar" / "rapido").
const int NIVEIS_PWM[3] = {130, 175, 220};
const int TOTAL_NIVEIS = 3;
const int NIVEL_INICIAL = 0;

// Giro no proprio eixo usa velocidade reduzida (percentual do nivel atual).
const int FATOR_GIRO_PCT = 70;

// Compensacao de desalinhamento: se a cadeira puxa para um lado, reduza o
// percentual da roda mais rapida (100 = sem correcao).
const int AJUSTE_ESQ_PCT = 100;
const int AJUSTE_DIR_PCT = 100;

// --- Rampa de aceleracao ----------------------------------------------------
// Evita o "tranco" na partida, que e desconfortavel e perigoso para o usuario.
const int RAMPA_PASSO = 8;                    // incremento de PWM por ciclo
const unsigned long RAMPA_INTERVALO_MS = 20;  // periodo do ciclo de rampa

// --- Seguranca --------------------------------------------------------------
// Homem-morto: sem novo comando dentro desse tempo, a cadeira para sozinha.
// E a protecao principal contra falha de reconhecimento ou perda do usuario.
const unsigned long TEMPO_LIMITE_COMANDO_MS = 3000;

// Giro e sempre um pulso curto e temporizado (aprox. 45 a 90 graus).
const unsigned long TEMPO_GIRO_MS = 700;

}  // namespace cfg

// Comandos reconhecidos. A ordem deve casar com os registros gravados no
// modulo de voz V3 (registro 0 = FRENTE, registro 1 = RE, ...).
enum Comando {
  CMD_NENHUM = -1,
  CMD_FRENTE = 0,
  CMD_RE = 1,
  CMD_ESQUERDA = 2,
  CMD_DIREITA = 3,
  CMD_PARAR = 4,
  CMD_RAPIDO = 5,
  CMD_DEVAGAR = 6,
  CMD_TOTAL = 7
};

enum Estado {
  EST_PARADA,
  EST_FRENTE,
  EST_RE,
  EST_GIRO_ESQ,
  EST_GIRO_DIR
};

// Motivo da ultima parada, usado na telemetria e no LED/buzzer.
enum MotivoParada {
  PARADA_NENHUMA,
  PARADA_COMANDO,
  PARADA_OBSTACULO,
  PARADA_TEMPO_LIMITE,
  PARADA_FIM_DO_GIRO,
  PARADA_EMERGENCIA
};

class ControleCadeira {
 public:
  ControleCadeira();

  // Zera a maquina de estados. Chame uma vez no setup().
  void iniciar(unsigned long agora_ms);

  // Processa um comando de voz. Retorna false se o comando foi recusado
  // (por exemplo, seguir em frente com obstaculo detectado).
  bool aplicarComando(Comando c, unsigned long agora_ms);

  // Deve ser chamada continuamente no loop principal.
  // obstaculo = true quando o sensor infravermelho ve algo a frente.
  void atualizar(unsigned long agora_ms, bool obstaculo);

  // Parada imediata, sem rampa. Botao de emergencia / falha critica.
  void pararEmergencia(unsigned long agora_ms);

  // Saidas para a ponte H, faixa -255 a 255 (negativo = para tras).
  int pwmEsquerdo() const { return atual_esq_; }
  int pwmDireito() const { return atual_dir_; }

  Estado estado() const { return estado_; }
  int nivelVelocidade() const { return nivel_; }
  bool emMovimento() const { return atual_esq_ != 0 || atual_dir_ != 0; }
  bool obstaculoDetectado() const { return obstaculo_; }
  MotivoParada motivoUltimaParada() const { return motivo_; }
  bool ultimoComandoRecusado() const { return recusado_; }

 private:
  void definirAlvo(int esq, int dir);
  void pararSuave(unsigned long agora_ms, MotivoParada motivo);
  void pararImediato(unsigned long agora_ms, MotivoParada motivo);
  void aplicarRampa(unsigned long agora_ms);
  int velocidadeBase() const;
  int velocidadeGiro() const;
  static int aproximar(int atual, int alvo, int passo);
  static int aproximarPorZero(int atual, int alvo, int passo);
  static int limitar(int v, int minimo, int maximo);

  Estado estado_;
  int nivel_;
  int alvo_esq_;
  int alvo_dir_;
  int atual_esq_;
  int atual_dir_;
  bool obstaculo_;
  bool recusado_;
  MotivoParada motivo_;
  unsigned long t_ultimo_comando_;
  unsigned long t_inicio_giro_;
  unsigned long t_ultima_rampa_;
};

// Nome legivel do comando, para depuracao pela porta serial.
const char* nomeComando(Comando c);
const char* nomeEstado(Estado e);
const char* nomeMotivo(MotivoParada m);

#endif  // CONTROLE_H
