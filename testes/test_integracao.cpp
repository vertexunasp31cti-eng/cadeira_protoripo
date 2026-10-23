/*
 * test_integracao.cpp - Os dois lados do protocolo juntos, no PC.
 *
 * Compilar e rodar:  make -C testes
 *
 * test_protocolo testa o Mega e test_ponte testa o ESP8266, cada um contra a
 * propria leitura do contrato. Aqui o codigo real dos dois lados conversa:
 * o que o Mega escreve, a ponte do ESP aceita e repassa ao app sem mudar, e
 * o que a ponte prepara a partir de um pedido do app, o Mega entende como o
 * comando certo. Um erro de digitacao num nome de campo ou de comando so
 * aparece neste teste.
 */
#include "../firmware/cadeira_voz/controle.h"
#include "../firmware/cadeira_voz/protocolo_app.h"
#include "../firmware/esp_wifi/ponte.h"

#include <cstdio>
#include <cstring>
#include <string>

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

// Entrega uma linha do Mega ("<json>*HH\n") a ponte, byte a byte.
static void megaParaPonte(ponte::PonteCadeira& p, const char* linha,
                          unsigned long agora) {
  for (const char* c = linha; *c; ++c) p.receberByte(*c, agora);
}

// Conteudo da linha do Mega, sem "*HH\n": e o que deve aparecer no JSON.
static std::string conteudo(const char* linha) {
  std::string s(linha);
  return s.substr(0, s.rfind('*'));
}

static std::string estado(const ponte::PonteCadeira& p, unsigned long agora,
                          unsigned long desde) {
  static char buf[ponte::TAM_ESTADO];
  unsigned int n = p.montarEstado(buf, sizeof(buf), agora, desde, 1);
  return std::string(buf, n);
}

static bool contem(const std::string& s, const std::string& trecho) {
  return s.find(trecho) != std::string::npos;
}

// Entrega a linha da ponte ao leitor do Mega, byte a byte, e interpreta a
// primeira linha completa que sair.
static LinhaApp ponteParaMega(LeitorLinhas& leitor, const char* linha,
                              unsigned long agora) {
  LinhaApp r = {LINHA_INVALIDA, CMD_NENHUM};
  bool achou = false;
  for (const char* c = linha; *c; ++c) {
    if (leitor.receber(*c, agora) && leitor.linha()[0] != '\0' && !achou) {
      r = interpretarLinhaApp(leitor.linha());
      achou = true;
    }
  }
  return r;
}

int main() {
  printf("Testes de integracao: Mega <-> ponte do ESP8266\n");

  char linha[TAM_LINHA_APP];

  // ------------------------------------------------------------------
  titulo("1. Telemetria do Mega chega intacta ao JSON do app");
  {
    ponte::PonteCadeira p;
    p.iniciar();
    unsigned long t = 1000;

    DadosTelemetria d;
    d.ms = 15230;
    d.estado = EST_FRENTE;
    d.pwm_esq = 130;
    d.pwm_dir = -130;
    d.nivel = 2;
    d.obstaculo = false;
    d.emergencia = false;
    d.motivo = PARADA_NENHUMA;
    d.voz = true;
    d.app = true;
    verificar(formatarTelemetria(linha, sizeof(linha), d) > 0,
              "Mega formata a telemetria");
    megaParaPonte(p, linha, t);

    std::string e = estado(p, t + 10, 0);
    verificar(contem(e, "\"conectado\":true"),
              "uma telemetria valida deixa a ponte conectada");
    verificar(contem(e, "\"tel\":" + conteudo(linha)),
              "o JSON do app traz a telemetria exatamente como o Mega mandou");
    verificar(p.linhasErro() == 0, "nenhuma linha recusada pela ponte");

    // Pior caso de cada campo: ainda precisa caber e ser aceito.
    d.ms = 4294967295UL;
    d.estado = EST_GIRO_ESQ;
    d.pwm_esq = -255;
    d.pwm_dir = -255;
    d.nivel = 3;
    d.obstaculo = true;
    d.emergencia = true;
    d.motivo = PARADA_TEMPO_LIMITE;
    verificar(formatarTelemetria(linha, sizeof(linha), d) > 0,
              "telemetria no pior caso cabe na linha do Mega");
    megaParaPonte(p, linha, t + 20);
    e = estado(p, t + 30, 0);
    verificar(contem(e, "\"tel\":" + conteudo(linha)),
              "telemetria no pior caso tambem passa pela ponte sem mudar");
  }

  // ------------------------------------------------------------------
  titulo("1b. Estatisticas do Mega chegam intactas ao JSON do app");
  {
    ponte::PonteCadeira p;
    p.iniciar();
    Estatisticas est;
    zerarEstatisticas(est);
    contarComando(est, "voz", true);
    contarObstaculo(est);
    verificar(formatarEstatisticas(linha, sizeof(linha), 61000UL, est) > 0,
              "Mega formata as estatisticas");
    megaParaPonte(p, linha, 1000);
    std::string e = estado(p, 1010, 0);
    verificar(contem(e, "\"est\":" + conteudo(linha) + ","),
              "o JSON do app traz as estatisticas como o Mega mandou");

    // Pior caso: todo contador no limite e o maior millis().
    for (long i = 0; i < 70000; ++i) {
      contarComando(est, "voz", false);
      contarComando(est, "app", true);
      contarComando(est, "teclado", true);
      contarObstaculo(est);
      contarEmergencia(est);
      contarParada(est, PARADA_COMANDO);
      contarParada(est, PARADA_OBSTACULO);
      contarParada(est, PARADA_TEMPO_LIMITE);
      contarParada(est, PARADA_FIM_DO_GIRO);
      contarParada(est, PARADA_EMERGENCIA);
    }
    est.ms_movimento = 4294967295UL;
    verificar(formatarEstatisticas(linha, sizeof(linha), 4294967295UL, est) > 0,
              "estatisticas no pior caso cabem na linha do Mega");
    megaParaPonte(p, linha, 2000);
    e = estado(p, 2010, 0);
    verificar(contem(e, "\"est\":" + conteudo(linha) + ",") &&
                  p.linhasErro() == 0,
              "e no pior caso tambem passam pela ponte sem mudar");
  }

  // ------------------------------------------------------------------
  titulo("2. Todo evento do Mega entra no historico do app, em ordem");
  {
    ponte::PonteCadeira p;
    p.iniciar();
    unsigned long t = 5000;

    std::string esperado[16];
    int qtd = 0;

    // Os maiores valores possiveis de cada evento, para garantir que cabem
    // no espaco que a ponte reserva por evento.
    const unsigned long MS_MAX = 4294967295UL;
    if (formatarEventoInicio(linha, sizeof(linha), MS_MAX, true)) {
      megaParaPonte(p, linha, t);
      esperado[qtd++] = conteudo(linha);
    }
    const Comando comandos[] = {CMD_FRENTE,  CMD_RE,     CMD_ESQUERDA,
                                CMD_DIREITA, CMD_PARAR,  CMD_RAPIDO,
                                CMD_DEVAGAR};
    const char* origens[] = {"voz", "teclado", "app"};
    for (int i = 0; i < 7; ++i) {
      if (formatarEventoComando(linha, sizeof(linha), MS_MAX, comandos[i],
                                origens[i % 3], i % 2 == 0)) {
        megaParaPonte(p, linha, t);
        esperado[qtd++] = conteudo(linha);
      }
    }
    if (formatarEventoParada(linha, sizeof(linha), MS_MAX,
                             PARADA_TEMPO_LIMITE)) {
      megaParaPonte(p, linha, t);
      esperado[qtd++] = conteudo(linha);
    }
    if (formatarEventoObstaculo(linha, sizeof(linha), MS_MAX, true)) {
      megaParaPonte(p, linha, t);
      esperado[qtd++] = conteudo(linha);
    }
    if (formatarEventoEmergencia(linha, sizeof(linha), MS_MAX, true)) {
      megaParaPonte(p, linha, t);
      esperado[qtd++] = conteudo(linha);
    }

    verificar(qtd == 11, "o Mega formatou os 11 eventos");
    verificar(p.linhasErro() == 0 && p.seq() == (unsigned long)qtd,
              "a ponte aceitou todos, com seq 1 a 11");

    std::string e = estado(p, t + 10, 0);
    size_t pos = 0;
    bool em_ordem = true;
    for (int i = 0; i < qtd; ++i) {
      char trecho[32];
      snprintf(trecho, sizeof(trecho), "{\"seq\":%d,", i + 1);
      size_t a = e.find(trecho, pos);
      size_t b = e.find("\"dados\":" + esperado[i], pos);
      if (a == std::string::npos || b == std::string::npos || b < a) {
        em_ordem = false;
        break;
      }
      pos = b;
    }
    verificar(em_ordem,
              "cada evento aparece com o seq certo e os dados sem mudanca");

    e = estado(p, t + 10, 9);
    verificar(!contem(e, "\"seq\":9,") && contem(e, "\"seq\":10,") &&
                  contem(e, "\"seq\":11,"),
              "desde=9 devolve so os eventos 10 e 11");
  }

  // ------------------------------------------------------------------
  titulo("3. Todo comando do app chega ao Mega como o comando certo");
  {
    ponte::PonteCadeira p;
    p.iniciar();
    unsigned long t = 1000;
    // A ponte so repassa movimento com o Mega falando.
    formatarEventoInicio(linha, sizeof(linha), 10, true);
    megaParaPonte(p, linha, t);

    struct Caso {
      const char* nome;
      Comando cmd;
    };
    const Caso casos[] = {{"frente", CMD_FRENTE},   {"tras", CMD_RE},
                          {"esquerda", CMD_ESQUERDA}, {"direita", CMD_DIREITA},
                          {"parar", CMD_PARAR},     {"rapido", CMD_RAPIDO},
                          {"devagar", CMD_DEVAGAR}};
    char saida[ponte::TAM_COMANDO];
    char texto[96];
    for (unsigned int i = 0; i < sizeof(casos) / sizeof(casos[0]); ++i) {
      LeitorLinhas leitor;
      ponte::ResultadoComando r = p.prepararComando(
          casos[i].nome, false, "", -1, t, saida, sizeof(saida));
      LinhaApp l = ponteParaMega(leitor, saida, t);
      snprintf(texto, sizeof(texto), "\"%s\" vira CMD %s no Mega",
               casos[i].nome, nomeComando(casos[i].cmd));
      verificar(r == ponte::CMD_OK && l.tipo == LINHA_COMANDO &&
                    l.cmd == casos[i].cmd,
                texto);
    }

    const Caso renovaveis[] = {{"frente", CMD_FRENTE}, {"tras", CMD_RE}};
    for (unsigned int i = 0; i < 2; ++i) {
      LeitorLinhas leitor;
      ponte::ResultadoComando r = p.prepararComando(
          renovaveis[i].nome, true, "", -1, t, saida, sizeof(saida));
      LinhaApp l = ponteParaMega(leitor, saida, t);
      snprintf(texto, sizeof(texto), "\"%s\" com manter=1 vira MNT no Mega",
               renovaveis[i].nome);
      verificar(r == ponte::CMD_OK && l.tipo == LINHA_RENOVACAO &&
                    l.cmd == renovaveis[i].cmd,
                texto);
    }
  }

  // ------------------------------------------------------------------
  titulo("4. Depois do lixo de boot do ESP, o primeiro comando nao se perde");
  {
    ponte::PonteCadeira p;
    p.iniciar();
    char saida[ponte::TAM_COMANDO];
    verificar(p.prepararComando("parar", false, "", -1, 0, saida,
                                sizeof(saida)) == ponte::CMD_OK,
              "parar e preparado mesmo sem o Mega ter falado ainda");

    // Lixo sem '\n' no fim, como a mensagem de boot do ESP lida a 115200, e
    // o comando colado nele, sem pausa nenhuma.
    LeitorLinhas leitor;
    unsigned long t = 100;
    const unsigned char lixo[] = {0x82, 0x6e, 0x9c, 0x0f, 0x6c, 0xfe,
                                  0x10, 'C',  'M',  'D',  0x00, 0xff};
    for (unsigned int i = 0; i < sizeof(lixo); ++i) {
      leitor.receber((char)lixo[i], t);
    }
    LinhaApp l = ponteParaMega(leitor, saida, t);
    verificar(l.tipo == LINHA_COMANDO && l.cmd == CMD_PARAR,
              "o '\\n' que a ponte poe antes da linha fecha o lixo pendente");
  }

  // ------------------------------------------------------------------
  titulo("5. Segure para andar, ida e volta pelos dois lados");
  {
    ponte::PonteCadeira p;
    p.iniciar();
    ControleCadeira c;
    LeitorLinhas leitor;
    unsigned long t = 1000;
    c.iniciar(t);
    formatarEventoInicio(linha, sizeof(linha), t, true);
    megaParaPonte(p, linha, t);

    char saida[ponte::TAM_COMANDO];
    long n = 0;

    // O app aperta Frente: CMD, depois uma renovacao a cada 400 ms por 4 s,
    // mais que o tempo do homem-morto.
    p.prepararComando("frente", false, "abc123", ++n, t, saida, sizeof(saida));
    LinhaApp l = ponteParaMega(leitor, saida, t);
    if (decidirAcaoApp(l, true, false) == ACAO_TRATAR) {
      c.aplicarComando(l.cmd, t);
    }
    bool renovou_sempre = true;
    for (int passo = 0; passo < 10; ++passo) {
      for (int ms = 0; ms < 400; ++ms) c.atualizar(++t, false);
      // O Mega continua mandando telemetria: a ponte segue conectada.
      formatarEventoInicio(linha, sizeof(linha), t, true);
      megaParaPonte(p, linha, t);
      ponte::ResultadoComando r = p.prepararComando(
          "frente", true, "abc123", ++n, t, saida, sizeof(saida));
      l = ponteParaMega(leitor, saida, t);
      if (r != ponte::CMD_OK ||
          decidirAcaoApp(l, true, false) != ACAO_RENOVAR ||
          !c.renovarComando(l.cmd, t)) {
        renovou_sempre = false;
      }
    }
    verificar(renovou_sempre && c.estado() == EST_FRENTE,
              "com as renovacoes, a cadeira segue em frente por 4 s");

    // O celular perde o Wi-Fi: nada mais chega. O homem-morto para a cadeira.
    for (int ms = 0; ms < 3100; ++ms) c.atualizar(++t, false);
    verificar(c.estado() == EST_PARADA &&
                  c.motivoUltimaParada() == PARADA_TEMPO_LIMITE,
              "sem renovacao, para sozinha pelo tempo limite");

    // O evento da parada chega ao app.
    formatarEventoParada(linha, sizeof(linha), t, c.motivoUltimaParada());
    megaParaPonte(p, linha, t);
    std::string e = estado(p, t, 0);
    verificar(contem(e, "\"tipo\":\"parada\",\"motivo\":\"TEMPO_LIMITE\""),
              "o app recebe a parada com o motivo TEMPO_LIMITE");

    // Uma renovacao atrasada que chega agora nao religa a cadeira.
    p.prepararComando("frente", true, "abc123", ++n, t, saida, sizeof(saida));
    l = ponteParaMega(leitor, saida, t);
    bool renovou = decidirAcaoApp(l, true, false) == ACAO_RENOVAR &&
                   c.renovarComando(l.cmd, t);
    verificar(!renovou && c.estado() == EST_PARADA,
              "renovacao depois da parada e ignorada: andar exige CMD novo");

    // Pedido mais antigo que chega fora de ordem nem sai do ESP.
    verificar(p.prepararComando("frente", false, "abc123", n - 3, t, saida,
                                sizeof(saida)) == ponte::CMD_ATRASADO,
              "comando atrasado pela rede e barrado na ponte");
  }

  // ------------------------------------------------------------------
  printf("\n----------------------------------------\n");
  printf("Total: %d   Falhas: %d\n", total, falhas);
  if (falhas == 0) printf("Todos os testes passaram.\n");
  return falhas == 0 ? 0 : 1;
}
