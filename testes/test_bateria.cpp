/*
 * test_bateria.cpp - Testes da estimativa de bateria, executados no PC.
 *
 * Compilar e rodar:  make -C testes
 */
#include "../firmware/cadeira_voz/bateria.h"

#include <cstdio>

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

static void titulo(const char* s) { printf("\n%s\n", s); }

// Leitura do A0 que corresponde a uma tensao na bateria.
static int adcPara(int mv_bateria) {
  long mv_pino = (long)mv_bateria * cfg_bat::DIVISOR_DEN / cfg_bat::DIVISOR_NUM;
  return (int)((mv_pino * 1023 + cfg_bat::REFERENCIA_MV / 2) /
               cfg_bat::REFERENCIA_MV);
}

static void rodar(EstimadorBateria& b, unsigned long& t, unsigned long ms,
                  int mv, bool movendo) {
  for (unsigned long i = 0; i < ms; i += 10) {
    t += 10;
    b.amostrar(adcPara(mv), movendo, t);
  }
}

static bool perto(int a, int b, int tol) { return a >= b - tol && a <= b + tol; }

int main() {
  printf("Testes da estimativa de bateria\n");

  titulo("1. Conversao da leitura do pino A0");
  {
    verificar(milivoltsDaLeitura(0) == 0, "leitura 0 e 0 mV");
    verificar(perto(milivoltsDaLeitura(adcPara(8400)), 8400, 20),
              "8,4 V na bateria sao lidos como 8,4 V");
    verificar(milivoltsDaLeitura(1023) == 15000,
              "fundo de escala do divisor 3x e 15 V");
    verificar(adcPara(8400) < 600,
              "bateria cheia fica bem abaixo do limite do pino (2,8 V)");
  }

  titulo("2. Curva de descarga 2S");
  {
    verificar(percentualLitio2S(8400) == 100, "8,4 V = 100%");
    verificar(percentualLitio2S(9000) == 100, "acima de cheia satura em 100%");
    verificar(percentualLitio2S(7640) == 50, "7,64 V = 50%");
    verificar(percentualLitio2S(6600) == 0, "6,6 V = 0%");
    verificar(percentualLitio2S(5000) == 0, "abaixo de vazia satura em 0%");
    bool desce = true;
    for (int mv = 8400; mv > 6600; mv -= 10) {
      if (percentualLitio2S(mv - 10) > percentualLitio2S(mv)) desce = false;
    }
    verificar(desce, "a porcentagem nunca sobe quando a tensao cai");
  }

  titulo("3. Bateria ausente");
  {
    EstimadorBateria b;
    unsigned long t = 0;
    rodar(b, t, 1000, 0, false);
    verificar(!b.ligada() && b.percentual() == -1,
              "sem divisor ligado, a bateria aparece como nao medida");
  }

  titulo("4. Filtro com a cadeira parada e em movimento");
  {
    EstimadorBateria b;
    unsigned long t = 0;
    rodar(b, t, 3000, 7900, false);
    verificar(b.ligada() && perto(b.milivolts(), 7900, 30),
              "parada, a leitura acompanha a tensao da bateria");
    int antes = b.percentual();

    // Partida dos motores: a tensao afunda 0,6 V por 2 s.
    rodar(b, t, 2000, 7300, true);
    verificar(antes - b.percentual() <= 10,
              "o afundamento da partida quase nao mexe na porcentagem");

    // Parou: a tensao volta e a estimativa tambem.
    rodar(b, t, 5000, 7900, false);
    verificar(perto(b.percentual(), antes, 2),
              "depois de parar, a porcentagem volta ao valor real");

    // Descarga de verdade, medida com a cadeira parada.
    rodar(b, t, 5000, 7400, false);
    verificar(perto(b.milivolts(), 7400, 30),
              "descarga real aparece na leitura parada");
  }

  printf("\n----------------------------------------\n");
  printf("Total: %d   Falhas: %d\n", total, falhas);
  if (falhas == 0) printf("Todos os testes passaram.\n");
  return falhas == 0 ? 0 : 1;
}
