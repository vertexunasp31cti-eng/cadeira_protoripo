/*
 * test_controle.cpp - Testes da maquina de estados, executados no PC.
 *
 * Compilar e rodar:  make -C testes
 *
 * Estes testes existem porque errar a logica de seguranca em uma cadeira de
 * rodas real machuca alguem. Toda regra de seguranca do prototipo tem um teste
 * correspondente aqui.
 */
#include "../firmware/cadeira_voz/controle.h"

#include <cstdio>
#include <cstdlib>
#include <cmath>

static int total = 0;
static int falhas = 0;

static void verificar(bool ok, const char* descricao) {
  ++total;
  if (ok) {
    printf("  [ok]    %s\n", descricao);
  } else {
    ++falhas;
    printf("  [FALHA] %s\n", descricao);
  }
}

// Avanca o tempo simulado chamando atualizar() a cada milissegundo,
// exatamente como o loop() faria no Arduino.
static void avancar(ControleCadeira& c, unsigned long& t, unsigned long ms,
                    bool obstaculo) {
  for (unsigned long i = 0; i < ms; ++i) {
    ++t;
    c.atualizar(t, obstaculo);
  }
}

static void titulo(const char* s) { printf("\n%s\n", s); }

int main() {
  printf("Testes da logica de controle da cadeira de rodas por voz\n");

  // ------------------------------------------------------------------
  titulo("1. Partida e rampa de aceleracao");
  {
    ControleCadeira c;
    unsigned long t = 1000;
    c.iniciar(t);

    verificar(c.pwmEsquerdo() == 0 && c.pwmDireito() == 0,
              "cadeira inicia parada");

    c.aplicarComando(CMD_FRENTE, t);
    verificar(c.estado() == EST_FRENTE, "comando FRENTE muda o estado");
    verificar(c.pwmEsquerdo() == 0,
              "PWM ainda e zero no instante do comando (sem tranco)");

    // Um unico intervalo de rampa nao pode saltar mais que o passo.
    avancar(c, t, cfg::RAMPA_INTERVALO_MS, false);
    verificar(c.pwmEsquerdo() == cfg::RAMPA_PASSO,
              "primeiro ciclo sobe exatamente um passo de rampa");

    // Tempo suficiente para atingir o alvo, mas menor que o homem-morto.
    avancar(c, t, 1000, false);
    verificar(c.pwmEsquerdo() == cfg::NIVEIS_PWM[cfg::NIVEL_INICIAL],
              "atinge a velocidade do nivel inicial");
    verificar(c.pwmEsquerdo() == c.pwmDireito(),
              "as duas rodas recebem o mesmo PWM em linha reta");
  }

  // ------------------------------------------------------------------
  titulo("2. A rampa nunca salta mais que RAMPA_PASSO por ciclo");
  {
    ControleCadeira c;
    unsigned long t = 0;
    c.iniciar(t);
    c.aplicarComando(CMD_FRENTE, t);

    int anterior = c.pwmEsquerdo();
    int maiorSalto = 0;
    for (int i = 0; i < 2000; ++i) {
      ++t;
      c.atualizar(t, false);
      // Renova o comando para nao cair no homem-morto durante o teste.
      if (i % 500 == 0) c.aplicarComando(CMD_FRENTE, t);
      int salto = std::abs(c.pwmEsquerdo() - anterior);
      if (salto > maiorSalto) maiorSalto = salto;
      anterior = c.pwmEsquerdo();
    }
    verificar(maiorSalto <= cfg::RAMPA_PASSO,
              "nenhum degrau de PWM maior que o passo da rampa");
  }

  // ------------------------------------------------------------------
  titulo("3. Parada por comando de voz e imediata");
  {
    ControleCadeira c;
    unsigned long t = 0;
    c.iniciar(t);
    c.aplicarComando(CMD_FRENTE, t);
    avancar(c, t, 1000, false);
    verificar(c.emMovimento(), "cadeira em movimento antes do PARAR");

    c.aplicarComando(CMD_PARAR, t);
    verificar(c.pwmEsquerdo() == 0 && c.pwmDireito() == 0,
              "PARAR zera o PWM no mesmo instante, sem esperar a rampa");
    verificar(c.estado() == EST_PARADA, "estado volta para PARADA");
    verificar(c.motivoUltimaParada() == PARADA_COMANDO,
              "motivo registrado: comando do usuario");
  }

  // ------------------------------------------------------------------
  titulo("4. Sensor de obstaculo");
  {
    ControleCadeira c;
    unsigned long t = 0;
    c.iniciar(t);
    c.aplicarComando(CMD_FRENTE, t);
    avancar(c, t, 500, false);
    verificar(c.emMovimento(), "andando para frente com caminho livre");

    // O sensor passa a ver um obstaculo.
    c.atualizar(++t, true);
    verificar(c.pwmEsquerdo() == 0 && c.pwmDireito() == 0,
              "obstaculo para a cadeira imediatamente");
    verificar(c.motivoUltimaParada() == PARADA_OBSTACULO,
              "motivo registrado: obstaculo a frente");

    // Novo comando de frente com o obstaculo ainda presente: recusado.
    bool aceito = c.aplicarComando(CMD_FRENTE, t);
    verificar(!aceito, "comando FRENTE e recusado enquanto ha obstaculo");
    avancar(c, t, 500, true);
    verificar(c.pwmEsquerdo() == 0,
              "cadeira permanece parada mesmo insistindo no comando");

    // Re continua permitida: o sensor olha so para a frente.
    bool aceitoRe = c.aplicarComando(CMD_RE, t);
    verificar(aceitoRe, "comando RE e aceito mesmo com obstaculo a frente");
    avancar(c, t, 500, true);
    verificar(c.pwmEsquerdo() < 0 && c.pwmDireito() < 0,
              "rodas giram no sentido reverso");

    // Removido o obstaculo, frente volta a ser permitida.
    c.atualizar(++t, false);
    verificar(c.aplicarComando(CMD_FRENTE, t),
              "FRENTE volta a ser aceito quando o caminho fica livre");
  }

  // ------------------------------------------------------------------
  titulo("5. Homem-morto (parada por tempo limite)");
  {
    ControleCadeira c;
    unsigned long t = 0;
    c.iniciar(t);
    c.aplicarComando(CMD_FRENTE, t);
    avancar(c, t, cfg::TEMPO_LIMITE_COMANDO_MS - 100, false);
    verificar(c.emMovimento(), "ainda andando antes de esgotar o tempo");

    avancar(c, t, 200, false);
    verificar(c.estado() == EST_PARADA,
              "estado vai para PARADA ao esgotar o tempo sem comando");
    verificar(c.motivoUltimaParada() == PARADA_TEMPO_LIMITE,
              "motivo registrado: tempo limite sem comando");

    avancar(c, t, 1000, false);
    verificar(c.pwmEsquerdo() == 0,
              "PWM chega a zero pela rampa apos o tempo limite");
  }

  // ------------------------------------------------------------------
  titulo("6. Giro e um pulso curto e temporizado");
  {
    ControleCadeira c;
    unsigned long t = 0;
    c.iniciar(t);
    c.aplicarComando(CMD_DIREITA, t);
    avancar(c, t, 200, false);
    verificar(c.pwmEsquerdo() > 0 && c.pwmDireito() < 0,
              "giro a direita inverte o sentido das rodas");
    verificar(c.pwmEsquerdo() < cfg::NIVEIS_PWM[cfg::NIVEL_INICIAL],
              "giro usa velocidade reduzida");

    avancar(c, t, cfg::TEMPO_GIRO_MS, false);
    verificar(c.estado() == EST_PARADA, "giro termina sozinho");
    verificar(c.motivoUltimaParada() == PARADA_FIM_DO_GIRO,
              "motivo registrado: fim do giro");

    c.aplicarComando(CMD_ESQUERDA, t);
    avancar(c, t, 200, false);
    verificar(c.pwmEsquerdo() < 0 && c.pwmDireito() > 0,
              "giro a esquerda inverte o sentido das rodas");
  }

  // ------------------------------------------------------------------
  titulo("7. Niveis de velocidade");
  {
    ControleCadeira c;
    unsigned long t = 0;
    c.iniciar(t);
    verificar(c.nivelVelocidade() == cfg::NIVEL_INICIAL, "comeca no nivel inicial");

    c.aplicarComando(CMD_DEVAGAR, t);
    verificar(c.nivelVelocidade() == 0, "nao desce abaixo do nivel 0");

    for (int i = 0; i < 10; ++i) c.aplicarComando(CMD_RAPIDO, t);
    verificar(c.nivelVelocidade() == cfg::TOTAL_NIVEIS - 1,
              "nao sobe acima do ultimo nivel");

    c.aplicarComando(CMD_FRENTE, t);
    avancar(c, t, 1500, false);
    verificar(c.pwmEsquerdo() == cfg::NIVEIS_PWM[cfg::TOTAL_NIVEIS - 1],
              "anda na velocidade do nivel maximo");

    // Mudar de nivel durante o movimento vale na hora.
    c.aplicarComando(CMD_DEVAGAR, t);
    avancar(c, t, 1500, false);
    verificar(c.pwmEsquerdo() == cfg::NIVEIS_PWM[cfg::TOTAL_NIVEIS - 2],
              "reduzir a velocidade em movimento tem efeito imediato");
  }

  // ------------------------------------------------------------------
  titulo("8. Inversao de sentido passa por zero");
  {
    ControleCadeira c;
    unsigned long t = 0;
    c.iniciar(t);
    c.aplicarComando(CMD_FRENTE, t);
    avancar(c, t, 1000, false);
    verificar(c.pwmEsquerdo() > 0, "andando para frente");

    c.aplicarComando(CMD_RE, t);
    bool passouPorZero = false;
    int anterior = c.pwmEsquerdo();
    bool inverteuSemZerar = false;
    for (int i = 0; i < 2000; ++i) {
      ++t;
      c.atualizar(t, false);
      if (i % 500 == 0) c.aplicarComando(CMD_RE, t);
      int v = c.pwmEsquerdo();
      if (v == 0) passouPorZero = true;
      if ((anterior > 0 && v < 0) || (anterior < 0 && v > 0)) {
        inverteuSemZerar = true;
      }
      anterior = v;
    }
    verificar(passouPorZero, "o PWM passa por zero ao inverter o sentido");
    verificar(!inverteuSemZerar,
              "nunca salta direto de positivo para negativo");
    verificar(c.pwmEsquerdo() < 0, "termina andando de re");
  }

  // ------------------------------------------------------------------
  titulo("9. Parada de emergencia");
  {
    ControleCadeira c;
    unsigned long t = 0;
    c.iniciar(t);
    c.aplicarComando(CMD_FRENTE, t);
    avancar(c, t, 1000, false);
    c.pararEmergencia(t);
    verificar(c.pwmEsquerdo() == 0 && c.pwmDireito() == 0,
              "emergencia corta o PWM na hora");
    verificar(c.motivoUltimaParada() == PARADA_EMERGENCIA,
              "motivo registrado: parada de emergencia");
  }

  // ------------------------------------------------------------------
  titulo("10. Faixa de PWM sempre valida");
  {
    ControleCadeira c;
    unsigned long t = 0;
    c.iniciar(t);
    const Comando sequencia[] = {CMD_FRENTE, CMD_RAPIDO,   CMD_ESQUERDA,
                                 CMD_RE,     CMD_DIREITA,  CMD_DEVAGAR,
                                 CMD_FRENTE, CMD_PARAR};
    bool dentroDaFaixa = true;
    for (int volta = 0; volta < 20; ++volta) {
      for (int i = 0; i < 8; ++i) {
        c.aplicarComando(sequencia[i], t);
        avancar(c, t, 120, (volta % 3) == 0);
        if (c.pwmEsquerdo() < -255 || c.pwmEsquerdo() > 255 ||
            c.pwmDireito() < -255 || c.pwmDireito() > 255) {
          dentroDaFaixa = false;
        }
      }
    }
    verificar(dentroDaFaixa, "PWM permanece entre -255 e 255 em qualquer sequencia");
  }

  // ------------------------------------------------------------------
  printf("\n----------------------------------------\n");
  printf("Total: %d   Falhas: %d\n", total, falhas);
  if (falhas == 0) printf("Todos os testes passaram.\n");
  return falhas == 0 ? 0 : 1;
}
