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
#include <cstring>

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

// Tudo o que renovarComando() nao pode mudar.
struct Retrato {
  Estado estado;
  int nivel;
  int esq;
  int dir;
  MotivoParada motivo;
  bool recusado;
  bool obstaculo;
};

static Retrato retratar(const ControleCadeira& c) {
  Retrato r;
  r.estado = c.estado();
  r.nivel = c.nivelVelocidade();
  r.esq = c.pwmEsquerdo();
  r.dir = c.pwmDireito();
  r.motivo = c.motivoUltimaParada();
  r.recusado = c.ultimoComandoRecusado();
  r.obstaculo = c.obstaculoDetectado();
  return r;
}

static bool iguais(const Retrato& a, const Retrato& b) {
  return a.estado == b.estado && a.nivel == b.nivel && a.esq == b.esq &&
         a.dir == b.dir && a.motivo == b.motivo && a.recusado == b.recusado &&
         a.obstaculo == b.obstaculo;
}

// Depois de uma parada, insiste na renovacao por 5 s, como o app faria com o
// botao segurado (a cada 400 ms), e confere que a cadeira nunca volta a andar:
// o estado fica em PARADA, o PWM so diminui e o motivo da parada nao muda.
static bool continuaParada(ControleCadeira& c, unsigned long& t, Comando mov,
                           bool obstaculo) {
  bool ok = true;
  MotivoParada motivo = c.motivoUltimaParada();
  int esq = std::abs(c.pwmEsquerdo());
  int dir = std::abs(c.pwmDireito());
  for (int i = 0; i < 5000; ++i) {
    ++t;
    if (i % 400 == 0 && c.renovarComando(mov, t)) ok = false;
    c.atualizar(t, obstaculo);
    if (c.estado() != EST_PARADA) ok = false;
    if (std::abs(c.pwmEsquerdo()) > esq || std::abs(c.pwmDireito()) > dir) {
      ok = false;
    }
    esq = std::abs(c.pwmEsquerdo());
    dir = std::abs(c.pwmDireito());
  }
  if (c.emMovimento() || c.motivoUltimaParada() != motivo) ok = false;
  return ok;
}

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
  titulo("11. Renovacao do app so prolonga o mesmo movimento");
  {
    ControleCadeira c;
    unsigned long t = 0;
    c.iniciar(t);
    bool renovou = c.renovarComando(CMD_FRENTE, t);
    renovou = c.renovarComando(CMD_RE, t) || renovou;
    avancar(c, t, 500, false);
    verificar(!renovou && c.estado() == EST_PARADA && !c.emMovimento(),
              "com a cadeira parada, renovar nao faz nada (nunca inicia movimento)");

    c.aplicarComando(CMD_FRENTE, t);
    avancar(c, t, 100, false);
    verificar(c.renovarComando(CMD_FRENTE, t),
              "andando para frente, renovar FRENTE vale");
    verificar(!c.renovarComando(CMD_RE, t) && c.estado() == EST_FRENTE,
              "andando para frente, renovar RE nao vale nem muda o sentido");

    c.aplicarComando(CMD_RE, t);
    avancar(c, t, 100, false);
    verificar(c.renovarComando(CMD_RE, t), "andando de re, renovar RE vale");
    verificar(!c.renovarComando(CMD_FRENTE, t) && c.estado() == EST_RE,
              "andando de re, renovar FRENTE nao vale nem muda o sentido");
  }
  {
    // Cada comando nao renovavel, tentado em cada estado possivel.
    const Comando naoRenovaveis[] = {CMD_ESQUERDA, CMD_DIREITA, CMD_PARAR,
                                     CMD_RAPIDO,   CMD_DEVAGAR, CMD_NENHUM};
    const Comando movimentos[] = {CMD_NENHUM, CMD_FRENTE, CMD_RE, CMD_ESQUERDA,
                                  CMD_DIREITA};
    bool algumRenovou = false;
    bool algumMudou = false;
    for (int m = 0; m < 5; ++m) {
      for (int r = 0; r < 6; ++r) {
        ControleCadeira c;
        unsigned long t = 0;
        c.iniciar(t);
        c.aplicarComando(CMD_RAPIDO, t);  // nivel do meio: sobe e desce
        if (movimentos[m] != CMD_NENHUM) c.aplicarComando(movimentos[m], t);
        avancar(c, t, 100, false);
        Retrato antes = retratar(c);
        if (c.renovarComando(naoRenovaveis[r], t)) algumRenovou = true;
        if (!iguais(antes, retratar(c))) algumMudou = true;
      }
    }
    verificar(!algumRenovou,
              "giro, parar, rapido e devagar nunca sao renovaveis, em nenhum estado");
    verificar(!algumMudou,
              "a tentativa nao muda estado, nivel, PWM, motivo nem recusa");
  }
  {
    ControleCadeira c;
    unsigned long t = 0;
    c.iniciar(t);
    c.aplicarComando(CMD_DIREITA, t);
    for (unsigned long i = 0; i < cfg::TEMPO_GIRO_MS; ++i) {
      ++t;
      c.renovarComando(CMD_DIREITA, t);
      c.renovarComando(CMD_FRENTE, t);
      c.renovarComando(CMD_RE, t);
      c.atualizar(t, false);
    }
    verificar(c.estado() == EST_PARADA &&
                  c.motivoUltimaParada() == PARADA_FIM_DO_GIRO,
              "renovar durante o giro nao estica o pulso: termina no tempo de sempre");
  }

  // ------------------------------------------------------------------
  titulo("12. Renovacao nunca retoma o movimento depois de uma parada");
  {
    const Comando sentidos[] = {CMD_FRENTE, CMD_RE};
    const char* nomes[] = {"FRENTE", "RE"};
    char texto[120];
    for (int s = 0; s < 2; ++s) {
      Comando mov = sentidos[s];
      {
        ControleCadeira c;
        unsigned long t = 0;
        c.iniciar(t);
        c.aplicarComando(mov, t);
        avancar(c, t, 500, false);
        c.aplicarComando(CMD_PARAR, t);
        snprintf(texto, sizeof(texto),
                 "%s: depois de PARAR, renovar nao volta a andar", nomes[s]);
        verificar(continuaParada(c, t, mov, false), texto);
      }
      {
        ControleCadeira c;
        unsigned long t = 0;
        c.iniciar(t);
        c.aplicarComando(mov, t);
        avancar(c, t, cfg::TEMPO_LIMITE_COMANDO_MS + 10, false);
        bool parouPorTempo =
            c.motivoUltimaParada() == PARADA_TEMPO_LIMITE;
        snprintf(texto, sizeof(texto),
                 "%s: depois do homem-morto, renovar nao volta a andar",
                 nomes[s]);
        verificar(parouPorTempo && continuaParada(c, t, mov, false), texto);
      }
      {
        ControleCadeira c;
        unsigned long t = 0;
        c.iniciar(t);
        c.aplicarComando(mov, t);
        avancar(c, t, 500, false);
        c.pararEmergencia(t);
        snprintf(texto, sizeof(texto),
                 "%s: depois da emergencia, renovar nao volta a andar",
                 nomes[s]);
        verificar(continuaParada(c, t, mov, false), texto);
      }
    }
  }
  {
    ControleCadeira c;
    unsigned long t = 0;
    c.iniciar(t);
    c.aplicarComando(CMD_FRENTE, t);
    avancar(c, t, 500, false);
    c.atualizar(++t, true);  // obstaculo aparece: para na hora
    verificar(!c.renovarComando(CMD_FRENTE, t),
              "com obstaculo a frente, renovar FRENTE e ignorado");
    verificar(continuaParada(c, t, CMD_FRENTE, true),
              "insistir na renovacao com o obstaculo presente nunca move a cadeira");
    verificar(continuaParada(c, t, CMD_FRENTE, false),
              "nem depois que o caminho fica livre: voltar exige comando novo");
  }
  {
    ControleCadeira c;
    unsigned long t = 0;
    c.iniciar(t);
    c.aplicarComando(CMD_ESQUERDA, t);
    avancar(c, t, cfg::TEMPO_GIRO_MS + 10, false);
    bool fimDoGiro = c.motivoUltimaParada() == PARADA_FIM_DO_GIRO;
    bool ok = continuaParada(c, t, CMD_ESQUERDA, false) &&
              continuaParada(c, t, CMD_FRENTE, false) &&
              continuaParada(c, t, CMD_RE, false);
    verificar(fimDoGiro && ok,
              "depois do fim do giro, nenhuma renovacao volta a mover a cadeira");
  }
  {
    // Re continua renovavel com obstaculo a frente, como o comando de re.
    ControleCadeira c;
    unsigned long t = 0;
    c.iniciar(t);
    c.atualizar(++t, true);
    c.aplicarComando(CMD_RE, t);
    bool andando = true;
    for (unsigned long i = 1; i <= 2 * cfg::TEMPO_LIMITE_COMANDO_MS; ++i) {
      ++t;
      if (i % 400 == 0) c.renovarComando(CMD_RE, t);
      c.atualizar(t, true);
      if (c.estado() != EST_RE) andando = false;
    }
    verificar(andando && c.pwmEsquerdo() < 0,
              "com obstaculo a frente, a re continua renovavel");
  }

  // ------------------------------------------------------------------
  titulo("13. Segurar o botao do app mantem o movimento; soltar para");
  {
    const Comando sentidos[] = {CMD_FRENTE, CMD_RE};
    const Estado estados[] = {EST_FRENTE, EST_RE};
    const char* nomes[] = {"FRENTE", "RE"};
    char texto[120];
    for (int s = 0; s < 2; ++s) {
      ControleCadeira c;
      unsigned long t = 0;
      c.iniciar(t);
      c.aplicarComando(sentidos[s], t);
      unsigned long ultimaRenovacao = t;
      bool sempreAndando = true;
      for (unsigned long i = 1; i <= 4 * cfg::TEMPO_LIMITE_COMANDO_MS; ++i) {
        ++t;
        if (i % 400 == 0 && c.renovarComando(sentidos[s], t)) {
          ultimaRenovacao = t;
        }
        c.atualizar(t, false);
        if (c.estado() != estados[s]) sempreAndando = false;
      }
      int velocidade = cfg::NIVEIS_PWM[cfg::NIVEL_INICIAL];
      snprintf(texto, sizeof(texto),
               "%s renovado a cada 400 ms passa de 4x o tempo limite sem parar",
               nomes[s]);
      verificar(sempreAndando && std::abs(c.pwmEsquerdo()) == velocidade,
                texto);

      // O app parou de renovar (botao solto, Wi-Fi caiu, celular travou).
      avancar(c, t,
              ultimaRenovacao + cfg::TEMPO_LIMITE_COMANDO_MS - 1 - t, false);
      bool andavaAntes = c.estado() == estados[s];
      avancar(c, t, 1, false);
      snprintf(texto, sizeof(texto),
               "%s sem renovacao para pelo homem-morto, 3 s apos a ultima",
               nomes[s]);
      verificar(andavaAntes && c.estado() == EST_PARADA &&
                    c.motivoUltimaParada() == PARADA_TEMPO_LIMITE,
                texto);
      avancar(c, t, 1000, false);
      verificar(!c.emMovimento(), "o PWM chega a zero pela rampa");
    }
  }
  {
    // Duas cadeiras com os mesmos comandos; so a primeira recebe renovacoes.
    // Ate o homem-morto da segunda vencer, as duas tem de ser identicas.
    const Comando sentidos[] = {CMD_FRENTE, CMD_RE};
    bool identicas = true;
    bool renovouSempre = true;
    bool recusaMantida = true;
    bool soASemRenovacaoParou = true;
    for (int s = 0; s < 2; ++s) {
      ControleCadeira a;
      ControleCadeira b;
      unsigned long t = 0;
      a.iniciar(t);
      b.iniciar(t);
      a.aplicarComando(sentidos[s], t);
      b.aplicarComando(sentidos[s], t);
      // Ja no nivel maximo, RAPIDO fica marcado como recusado.
      for (int i = 0; i < cfg::TOTAL_NIVEIS; ++i) {
        a.aplicarComando(CMD_RAPIDO, t);
        b.aplicarComando(CMD_RAPIDO, t);
      }
      for (unsigned long i = 1; i < cfg::TEMPO_LIMITE_COMANDO_MS; ++i) {
        ++t;
        if (i % 400 == 0 && !a.renovarComando(sentidos[s], t)) {
          renovouSempre = false;
        }
        a.atualizar(t, false);
        b.atualizar(t, false);
        if (!iguais(retratar(a), retratar(b))) identicas = false;
      }
      if (!a.ultimoComandoRecusado()) recusaMantida = false;
      ++t;
      a.atualizar(t, false);
      b.atualizar(t, false);
      if (b.estado() != EST_PARADA || a.estado() == EST_PARADA) {
        soASemRenovacaoParou = false;
      }
    }
    verificar(renovouSempre,
              "renovacoes durante o movimento, inclusive na rampa, sao aceitas");
    verificar(identicas,
              "renovar nao muda alvo, rampa, nivel, motivo, estado nem recusa");
    verificar(recusaMantida,
              "o aviso de comando recusado continua depois das renovacoes");
    verificar(soASemRenovacaoParou,
              "so muda o tempo do homem-morto: a cadeira sem renovacao para");
  }
  {
    ControleCadeira c;
    unsigned long t = 0;
    c.iniciar(t);
    c.aplicarComando(CMD_FRENTE, t);
    avancar(c, t, cfg::TEMPO_LIMITE_COMANDO_MS - 1, false);
    verificar(c.renovarComando(CMD_FRENTE, t),
              "renovacao 1 ms antes do tempo limite vale");
    // Volta do laco atrasada: o prazo venceu antes de o atualizar() rodar.
    t += cfg::TEMPO_LIMITE_COMANDO_MS;
    verificar(!c.renovarComando(CMD_FRENTE, t),
              "renovacao que chega com o homem-morto ja vencido e recusada");
    c.atualizar(t, false);
    verificar(c.estado() == EST_PARADA &&
                  c.motivoUltimaParada() == PARADA_TEMPO_LIMITE,
              "e a cadeira para por tempo limite");
  }

  // ------------------------------------------------------------------
  titulo("14. Codigo do motivo de parada para o app");
  {
    const MotivoParada motivos[] = {PARADA_NENHUMA,      PARADA_COMANDO,
                                    PARADA_OBSTACULO,    PARADA_TEMPO_LIMITE,
                                    PARADA_FIM_DO_GIRO,  PARADA_EMERGENCIA};
    const char* codigos[] = {"NENHUMA",      "COMANDO",     "OBSTACULO",
                             "TEMPO_LIMITE", "FIM_DO_GIRO", "EMERGENCIA"};
    char texto[120];
    for (int i = 0; i < 6; ++i) {
      snprintf(texto, sizeof(texto), "codigoMotivo -> %s", codigos[i]);
      verificar(strcmp(codigoMotivo(motivos[i]), codigos[i]) == 0, texto);
    }
    verificar(strcmp(nomeMotivo(PARADA_TEMPO_LIMITE),
                     "tempo limite sem comando") == 0 &&
                  strcmp(nomeMotivo(PARADA_NENHUMA), "-") == 0,
              "nomeMotivo, o texto da depuracao, continua o mesmo");
  }

  // ------------------------------------------------------------------
  printf("\n----------------------------------------\n");
  printf("Total: %d   Falhas: %d\n", total, falhas);
  if (falhas == 0) printf("Todos os testes passaram.\n");
  return falhas == 0 ? 0 : 1;
}
