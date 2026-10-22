/*
 * test_protocolo.cpp - Testes do protocolo serial entre o Mega e o ESP8266,
 * do lado do Mega. Executados no PC.
 *
 * Compilar e rodar:  make -C testes
 *
 * Por este protocolo chegam comandos de movimento. Ruido na serial (o ESP8266
 * imprime lixo quando liga) nunca pode virar comando, e toda linha que o Mega
 * escreve precisa sair inteira ou nao sair.
 *
 * Os valores esperados de checksum foram calculados a parte, em Python, para
 * nao conferir o codigo com ele mesmo.
 */
#include "../firmware/cadeira_voz/protocolo_app.h"

#include <cctype>
#include <cstdio>
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

static void titulo(const char* s) { printf("\n%s\n", s); }

// Gerador pseudoaleatorio com semente fixa (xorshift32): o mesmo "ruido" em
// toda execucao, para uma falha poder ser repetida.
static uint32_t semente = 2463534242u;
static uint32_t aleatorio() {
  semente ^= semente << 13;
  semente ^= semente >> 17;
  semente ^= semente << 5;
  return semente;
}

// As 9 linhas validas do ESP para o Mega, com checksum calculado em Python.
struct Valida {
  const char* linha;
  TipoLinhaApp tipo;
  Comando cmd;
};
static const Valida VALIDAS[] = {
    {"CMD:frente*7E", LINHA_COMANDO, CMD_FRENTE},
    {"CMD:tras*64", LINHA_COMANDO, CMD_RE},
    {"CMD:esquerda*70", LINHA_COMANDO, CMD_ESQUERDA},
    {"CMD:direita*16", LINHA_COMANDO, CMD_DIREITA},
    {"CMD:parar*00", LINHA_COMANDO, CMD_PARAR},
    {"CMD:rapido*71", LINHA_COMANDO, CMD_RAPIDO},
    {"CMD:devagar*12", LINHA_COMANDO, CMD_DEVAGAR},
    {"MNT:frente*63", LINHA_RENOVACAO, CMD_FRENTE},
    {"MNT:tras*79", LINHA_RENOVACAO, CMD_RE},
};
static const int TOTAL_VALIDAS = 9;

// Oraculo independente do codigo testado: a linha e uma das 9 validas, com o
// hexadecimal em maiusculo ou minusculo. Retorna o indice ou -1.
static int linhaCanonica(const char* s) {
  for (int i = 0; i < TOTAL_VALIDAS; ++i) {
    const char* v = VALIDAS[i].linha;
    size_t n = strlen(v);
    if (strlen(s) != n || strncmp(s, v, n - 2) != 0) continue;
    if (toupper((unsigned char)s[n - 2]) == v[n - 2] &&
        toupper((unsigned char)s[n - 1]) == v[n - 1]) {
      return i;
    }
  }
  return -1;
}

// Monta "<conteudo>*HH" com o checksum certo, sem o '\n'.
static void montar(char* destino, size_t cap, const char* conteudo,
                   bool minusculo) {
  unsigned int x = checksumLinha(conteudo, (unsigned int)strlen(conteudo));
  snprintf(destino, cap, minusculo ? "%s*%02x" : "%s*%02X", conteudo, x);
}

static bool invalida(const char* s) {
  return interpretarLinhaApp(s).tipo == LINHA_INVALIDA;
}

// Passa n bytes pelo leitor, interpretando cada linha completa, como o laco
// do Mega faz. Conta linhas completas e linhas validas.
struct Contagem {
  long linhas;
  long validas;
  bool terminador_ok;  // linha() sempre terminou dentro do buffer
};
static Contagem alimentar(LeitorLinhas& l, const char* s, size_t n) {
  Contagem c = {0, 0, true};
  for (size_t i = 0; i < n; ++i) {
    if (!l.receber(s[i])) continue;
    ++c.linhas;
    if (memchr(l.linha(), '\0', TAM_LINHA_ENTRADA_APP) == NULL) {
      c.terminador_ok = false;
      continue;
    }
    if (interpretarLinhaApp(l.linha()).tipo != LINHA_INVALIDA) ++c.validas;
  }
  return c;
}

static Contagem alimentarTexto(LeitorLinhas& l, const char* s) {
  return alimentar(l, s, strlen(s));
}

// Confere uma linha que o Mega escreve: termina em '\n', o conteudo e ASCII
// imprimivel, comeca como o ESP espera, fecha a chave e o checksum bate. E a
// mesma validacao que a ponte do ESP faz.
static bool linhaDeSaidaValida(const char* s, unsigned int n) {
  if (n < 6 || strlen(s) != n || s[n - 1] != '\n' || s[n - 4] != '*') {
    return false;
  }
  unsigned int conteudo = n - 4;
  for (unsigned int i = 0; i < conteudo; ++i) {
    unsigned char b = (unsigned char)s[i];
    if (b < 0x20 || b > 0x7E) return false;
  }
  if (strncmp(s, "{\"t\":\"tel\",", 11) != 0 &&
      strncmp(s, "{\"t\":\"evt\",", 11) != 0) {
    return false;
  }
  if (s[conteudo - 1] != '}') return false;
  unsigned int x = 0;
  for (unsigned int i = 0; i < conteudo; ++i) x ^= (unsigned char)s[i];
  char hex[3];
  snprintf(hex, sizeof(hex), "%02X", x);
  return s[n - 3] == hex[0] && s[n - 2] == hex[1];
}

static DadosTelemetria telemetriaMaxima() {
  DadosTelemetria d;
  d.ms = 4294967295UL;
  d.estado = EST_GIRO_ESQ;
  d.pwm_esq = -255;
  d.pwm_dir = -255;
  d.nivel = 3;
  d.obstaculo = true;
  d.emergencia = true;
  d.motivo = PARADA_TEMPO_LIMITE;
  d.voz = true;
  d.app = true;
  return d;
}

// Os seis formatadores, cada um com valores fixos, para os testes de tamanho.
static const int TOTAL_FORMATADORES = 6;
static unsigned int formatarCaso(int i, char* buf, unsigned int cap) {
  switch (i) {
    case 0: return formatarTelemetria(buf, cap, telemetriaMaxima());
    case 1: return formatarEventoInicio(buf, cap, 4294967295UL, true);
    case 2:
      return formatarEventoComando(buf, cap, 4294967295UL, CMD_ESQUERDA,
                                   "teclado", true);
    case 3:
      return formatarEventoParada(buf, cap, 4294967295UL, PARADA_FIM_DO_GIRO);
    case 4: return formatarEventoObstaculo(buf, cap, 4294967295UL, true);
    default: return formatarEventoEmergencia(buf, cap, 4294967295UL, true);
  }
}

// Mensagem de boot do ESP8266 (74880 bps) como a UART do Mega, a 115200, a
// recebe: simulada bit a bit por um script Python, a partir do texto real do
// boot ("ets Jan  8 2013,rst cause:2..."). Nenhum '\n' e o fim fora do ASCII
// imprimivel: o leitor termina descartando, o pior caso para ele.
static const char LIXO_BOOT_ESP[] =
    "\x72\x6c\x00\x6c\x9c\x9f\x7c\x00\x8c\x6c\xe0\x7c\x03\x0c\x0c\x0c"
    "\x8c\x0c\x6c\xec\x0c\x62\x7c\x8f\x82\x03\xec\x12\x92\x72\x92\x63"
    "\x8c\x0c\x62\x8c\xf2\x6f\x6f\x9e\x6c\x6e\x6e\x9c\xe3\xec\x0c\x62"
    "\x1c\x70\x8c\x8f\x6c\x73\x6c\x72\x6c\x70\xf2\x6f\xe0\x10\x03\x0c"
    "\x0c\x82\x0c\x6c\x0c\x8c\x9c\x0c\x0c\x0c\x63\x0c\x6e\xe2\x7c\x02"
    "\xec\x0c\x8e\x8e\x0c\x62\x8c\xf2\x6f\x6f\xee\x00\x6c\x8c\x8f\x6c"
    "\x60\x03\x90\x12\x12\x6e\x6f\x0c\x8e\x0e\x6c\x0e\x02\x6e\x72\x8f"
    "\x93\x93\x6e\x0c\x0c\x72\x73\x93\x60\x02\x70\xf2\x6e\xe0\x10\x03"
    "\x0c\x0c\x72\x8c\x9c\x9c\x9c\x1c\x80\x8c\x02\x0c\x63\x0c\x6e\xe2"
    "\x7c\x02\x0c\x8e\x0c\x62\x8c\xf2\x6e\x6f\xef\x00\x0c\x8e\x6c\x60"
    "\x02\x90\x12\x13\x6e\x6e\x0c\x8f\x0f\x6c\x0f\x03\x6e\x72\x8e\x92"
    "\x92\x6f\x0c\x0c\x72\x12\x6c\x60\x02\x0f\x73\x93\x92\x6e\x0c\x0c"
    "\x73\x13\x6c\x60\x02\x9c\x02\x0c\x0c\x0c\x8f\x1c\x1c\x92\x0c\x6c"
    "\x60\x03\xfc\x82\x6e\x9c\x0f\x6c";
static const size_t TAM_LIXO_BOOT = sizeof(LIXO_BOOT_ESP) - 1;  // tem 0x00

// Como alimentar(), mas com o relogio parado em agora: uma volta do laco que
// le varios bytes de uma vez.
static Contagem alimentarNoTempo(LeitorLinhas& l, const char* s, size_t n,
                                 unsigned long agora) {
  Contagem c = {0, 0, true};
  for (size_t i = 0; i < n; ++i) {
    if (!l.receber(s[i], agora)) continue;
    ++c.linhas;
    if (interpretarLinhaApp(l.linha()).tipo != LINHA_INVALIDA) ++c.validas;
  }
  return c;
}

static Contagem alimentarTextoNoTempo(LeitorLinhas& l, const char* s,
                                      unsigned long agora) {
  return alimentarNoTempo(l, s, strlen(s), agora);
}

static const char* nomeAcao(AcaoApp a) {
  switch (a) {
    case ACAO_TRATAR: return "trata";
    case ACAO_RECUSAR: return "recusa";
    case ACAO_RENOVAR: return "renova";
    default: return "ignora";
  }
}

int main() {
  printf("Testes do protocolo serial do app (lado do Mega)\n");
  char texto[200];

  // ------------------------------------------------------------------
  titulo("1. Checksum");
  {
    verificar(checksumLinha("", 0) == 0, "linha vazia tem checksum 0");
    verificar(checksumLinha("A", 1) == 0x41, "um byte: o proprio byte");
    verificar(checksumLinha("CMD:frente", 10) == 0x7E &&
                  checksumLinha("MNT:frente", 10) == 0x63 &&
                  checksumLinha("CMD:parar", 9) == 0x00,
              "CMD:frente=7E, MNT:frente=63, CMD:parar=00 (calculados em Python)");
    verificar(checksumLinha("CMD:frenteXYZ", 10) == 0x7E,
              "usa so os n primeiros bytes");
    verificar(checksumLinha("\xff\x80", 2) == 0x7F,
              "bytes acima de 0x7F entram sem perder bits");
    const char* exemplo =
        "{\"t\":\"tel\",\"ms\":15230,\"estado\":\"FRENTE\",\"pwmEsq\":130,"
        "\"pwmDir\":130,\"nivel\":1,\"obstaculo\":0,\"emergencia\":0,"
        "\"motivo\":\"NENHUMA\",\"voz\":1,\"app\":1}";
    verificar(checksumLinha(exemplo, (unsigned int)strlen(exemplo)) == 0x7F,
              "telemetria de exemplo: 7F (calculado em Python)");
  }

  // ------------------------------------------------------------------
  titulo("2. Leitor de linhas");
  {
    LeitorLinhas l;
    const char* s = "CMD:frente*7E\n";
    bool cedo = false;
    int completas = 0;
    for (size_t i = 0; s[i] != '\0'; ++i) {
      bool r = l.receber(s[i]);
      if (r) ++completas;
      if (r && s[i] != '\n') cedo = true;
    }
    verificar(completas == 1 && !cedo,
              "linha normal fica completa so no '\\n', uma unica vez");
    verificar(strcmp(l.linha(), "CMD:frente*7E") == 0,
              "linha() devolve o texto sem o '\\n'");

    Contagem c = alimentarTexto(l, "CMD:tras*64\nCMD:parar*00\n");
    verificar(c.linhas == 2 && strcmp(l.linha(), "CMD:parar*00") == 0,
              "duas linhas seguidas saem separadas");

    alimentarTexto(l, "CMD:parar*00\r\n");
    bool semCr = strcmp(l.linha(), "CMD:parar*00") == 0;
    alimentarTexto(l, "CMD:pa\rrar*00\r\n");
    verificar(semCr && strcmp(l.linha(), "CMD:parar*00") == 0,
              "'\\r' e ignorado, no fim ou no meio da linha");

    c = alimentarTexto(l, "\n\r\n\n");
    verificar(c.linhas == 0, "linha vazia nao conta como linha");
  }
  {
    LeitorLinhas l;
    char longa[64];
    memset(longa, 'A', sizeof(longa));
    longa[TAM_LINHA_ENTRADA_APP - 1] = '\n';  // 31 bytes + '\n'
    Contagem c = alimentar(l, longa, TAM_LINHA_ENTRADA_APP);
    verificar(c.linhas == 1 && strlen(l.linha()) == TAM_LINHA_ENTRADA_APP - 1,
              "linha de 31 bytes, o maximo do buffer, e aceita");

    longa[TAM_LINHA_ENTRADA_APP - 1] = 'A';
    longa[TAM_LINHA_ENTRADA_APP] = '\n';  // 32 bytes + '\n'
    c = alimentar(l, longa, TAM_LINHA_ENTRADA_APP + 1);
    verificar(c.linhas == 0, "linha de 32 bytes estoura e e descartada");
    c = alimentarTexto(l, "CMD:frente*7E\n");
    verificar(c.linhas == 1 && c.validas == 1,
              "depois do estouro, a proxima linha e lida normalmente");

    // Estouro com um comando valido escondido no fim da linha comprida.
    char lixo[1100];
    memset(lixo, 'x', 1000);
    strcpy(lixo + 1000, "CMD:frente*7E\nCMD:tras*64\n");
    c = alimentarTexto(l, lixo);
    verificar(c.linhas == 1 && c.validas == 1 &&
                  strcmp(l.linha(), "CMD:tras*64") == 0,
              "lixo comprido descarta ate o '\\n', inclusive o comando colado nele");
  }
  {
    const unsigned char ruins[] = {0x00, 0x01, 0x1F, 0x7F, 0x80, 0xC3, 0xFF};
    bool descartou = true;
    bool recuperou = true;
    for (size_t i = 0; i < sizeof(ruins); ++i) {
      LeitorLinhas l;
      char s[32];
      memcpy(s, "CMD:fr", 6);
      s[6] = (char)ruins[i];
      memcpy(s + 7, "ente*7E\n", 8);
      if (alimentar(l, s, 15).linhas != 0) descartou = false;
      // Byte ruim colado no inicio de uma linha boa: ela e perdida inteira.
      s[0] = (char)ruins[i];
      memcpy(s + 1, "CMD:frente*7E\n", 14);
      if (alimentar(l, s, 15).linhas != 0) descartou = false;
      if (alimentarTexto(l, "CMD:frente*7E\n").validas != 1) recuperou = false;
    }
    verificar(descartou,
              "byte fora do ASCII imprimivel (0x00, 0x7F, 0x80, 0xFF...) "
              "descarta a linha inteira");
    verificar(recuperou, "e a linha seguinte e lida normalmente");
  }
  {
    LeitorLinhas l;
    alimentarTexto(l, "CMD:fre");
    l.descartarLinhaAtual();
    Contagem c = alimentarTexto(l, "nte*7E\n");
    verificar(c.linhas == 0,
              "descartarLinhaAtual joga fora a linha que estava pela metade");
    c = alimentarTexto(l, "CMD:frente*7E\n");
    verificar(c.validas == 1, "e a proxima linha inteira vale");

    l.descartarLinhaAtual();  // no inicio de linha: nada a descartar
    l.descartarLinhaAtual();
    c = alimentarTexto(l, "CMD:parar*00\n");
    verificar(c.validas == 1,
              "no inicio de linha nao perde a proxima (que pode ser um parar)");
  }

  // ------------------------------------------------------------------
  titulo("3. Ruido na serial nunca vira comando");
  {
    // Bytes de 0x00 a 0xFF, como o boot do ESP8266 a 74880 bps visto a
    // 115200, ou um fio solto.
    const size_t TOTAL_RUIDO = 2000000;
    static char ruido[TOTAL_RUIDO];
    for (size_t i = 0; i < TOTAL_RUIDO; ++i) ruido[i] = (char)(aleatorio() >> 24);
    LeitorLinhas l;
    Contagem c = alimentar(l, ruido, TOTAL_RUIDO);
    snprintf(texto, sizeof(texto),
             "2 milhoes de bytes aleatorios: %ld linhas completas, %ld comandos",
             c.linhas, c.validas);
    verificar(c.validas == 0 && c.terminador_ok, texto);

    // Ruido so com os caracteres do protocolo, para muitas linhas chegarem
    // ate o interpretador.
    const char alfabeto[] = "CMDNT:frentasquoidplvg*0123456789ABCDEFabcdef\n\n";
    for (size_t i = 0; i < TOTAL_RUIDO; ++i) {
      ruido[i] = alfabeto[aleatorio() % (sizeof(alfabeto) - 1)];
    }
    c = alimentar(l, ruido, TOTAL_RUIDO);
    snprintf(texto, sizeof(texto),
             "2 milhoes de caracteres do protocolo: %ld linhas, %ld comandos",
             c.linhas, c.validas);
    verificar(c.linhas > 10000 && c.validas == 0 && c.terminador_ok, texto);

    const char* boot =
        "\n ets Jan  8 2013,rst cause:2, boot mode:(3,6)\n\n"
        "load 0x4010f000, len 3460, room 16 \ntail 4\nchksum 0xcc\n"
        "load 0x3fff20b8, len 40, room 4 \ntail 4\nchksum 0xc9\n"
        "csum 0xc9\nv00044b70\n~ld\n";
    c = alimentarTexto(l, boot);
    verificar(c.linhas > 0 && c.validas == 0,
              "mensagem de boot do ESP8266 lida na velocidade certa: nenhum comando");
  }
  {
    // Cada byte de cada linha valida trocado por cada um dos 256 valores.
    // O XOR pega qualquer erro em um byte: so a troca de caixa do
    // hexadecimal (7E -> 7e) continua valida, e com o mesmo comando.
    long trocas = 0;
    long aceitasErradas = 0;
    for (int i = 0; i < TOTAL_VALIDAS; ++i) {
      const char* v = VALIDAS[i].linha;
      size_t n = strlen(v);
      for (size_t p = 0; p < n; ++p) {
        for (int b = 0; b < 256; ++b) {
          if ((char)b == v[p]) continue;
          char s[32];
          memcpy(s, v, n + 1);
          s[p] = (char)b;
          ++trocas;
          LinhaApp r = interpretarLinhaApp(s);
          bool soCaixa = p >= n - 2 && b != 0 &&
                         toupper(b) == (unsigned char)v[p];
          bool esperado = soCaixa;
          bool aceita = r.tipo != LINHA_INVALIDA;
          if (aceita != esperado ||
              (aceita && (r.tipo != VALIDAS[i].tipo ||
                          r.cmd != VALIDAS[i].cmd))) {
            ++aceitasErradas;
          }
        }
      }
    }
    snprintf(texto, sizeof(texto),
             "%ld linhas com um byte trocado: nenhuma vira outro comando",
             trocas);
    verificar(aceitasErradas == 0, texto);
  }
  {
    // Linhas montadas ao acaso a partir de pedacos do protocolo, metade com
    // checksum certo. O interpretador tem de concordar com o oraculo.
    const char* prefixos[] = {"CMD:", "MNT:", "cmd:", "CMD;", "MNT", "CMD:CMD:",
                              "", " CMD:"};
    const char* nomes[] = {"frente", "tras",    "esquerda", "direita",
                           "parar",  "rapido",  "devagar",  "re",
                           "frent",  "frentex", "FRENTE",   "tras ",
                           "",       "xfrente"};
    long sorteadas = 0;
    long aceitas = 0;
    long divergencias = 0;
    for (int k = 0; k < 300000; ++k) {
      char conteudo[40];
      snprintf(conteudo, sizeof(conteudo), "%s%s", prefixos[aleatorio() % 8],
               nomes[aleatorio() % 14]);
      char s[48];
      if (aleatorio() % 2) {
        montar(s, sizeof(s), conteudo, aleatorio() % 2 != 0);
      } else {
        snprintf(s, sizeof(s), "%s*%02X", conteudo,
                 (unsigned int)(aleatorio() & 0xFF));
      }
      if (aleatorio() % 10 == 0) {  // as vezes um byte qualquer vira lixo
        size_t n = strlen(s);
        if (n > 0) s[aleatorio() % n] = (char)(1 + aleatorio() % 255);
      }
      ++sorteadas;
      int esperado = linhaCanonica(s);
      LinhaApp r = interpretarLinhaApp(s);
      if (r.tipo != LINHA_INVALIDA) ++aceitas;
      bool ok = esperado < 0
                    ? r.tipo == LINHA_INVALIDA
                    : (r.tipo == VALIDAS[esperado].tipo &&
                       r.cmd == VALIDAS[esperado].cmd);
      if (!ok) {
        ++divergencias;
        if (divergencias <= 5) printf("    divergencia: \"%s\"\n", s);
      }
    }
    snprintf(texto, sizeof(texto),
             "%ld linhas sorteadas (%ld validas): interpretacao igual ao oraculo",
             sorteadas, aceitas);
    verificar(divergencias == 0 && aceitas > 1000, texto);
  }

  // ------------------------------------------------------------------
  titulo("4. Interpretacao das linhas de comando");
  {
    for (int i = 0; i < TOTAL_VALIDAS; ++i) {
      LinhaApp r = interpretarLinhaApp(VALIDAS[i].linha);
      snprintf(texto, sizeof(texto), "%s -> %s %s", VALIDAS[i].linha,
               VALIDAS[i].tipo == LINHA_COMANDO ? "comando" : "renovacao",
               nomeComando(VALIDAS[i].cmd));
      verificar(r.tipo == VALIDAS[i].tipo && r.cmd == VALIDAS[i].cmd, texto);
    }

    const char* naoRenovaveis[] = {"esquerda", "direita", "parar", "rapido",
                                   "devagar"};
    bool todas = true;
    for (int i = 0; i < 5; ++i) {
      char conteudo[24];
      char s[32];
      snprintf(conteudo, sizeof(conteudo), "MNT:%s", naoRenovaveis[i]);
      montar(s, sizeof(s), conteudo, false);
      LinhaApp r = interpretarLinhaApp(s);
      if (r.tipo != LINHA_INVALIDA || r.cmd != CMD_NENHUM) todas = false;
    }
    verificar(todas,
              "MNT com esquerda, direita, parar, rapido ou devagar e invalido");

    bool errados = true;
    for (int i = 0; i < TOTAL_VALIDAS; ++i) {
      const char* v = VALIDAS[i].linha;
      size_t n = strlen(v);
      char conteudo[24];
      char s[32];
      memcpy(conteudo, v, n - 3);
      conteudo[n - 3] = '\0';
      unsigned int x = checksumLinha(conteudo, (unsigned int)(n - 3));
      snprintf(s, sizeof(s), "%s*%02X", conteudo, (x + 1) & 0xFF);
      if (!invalida(s)) errados = false;
    }
    verificar(errados, "checksum errado invalida qualquer linha");

    bool minusculo = true;
    for (int i = 0; i < TOTAL_VALIDAS; ++i) {
      char s[32];
      strcpy(s, VALIDAS[i].linha);
      size_t n = strlen(s);
      s[n - 1] = (char)tolower((unsigned char)s[n - 1]);
      s[n - 2] = (char)tolower((unsigned char)s[n - 2]);
      LinhaApp r = interpretarLinhaApp(s);
      if (r.tipo != VALIDAS[i].tipo || r.cmd != VALIDAS[i].cmd) {
        minusculo = false;
      }
    }
    LinhaApp r = interpretarLinhaApp("CMD:frente*7e");
    verificar(minusculo && r.tipo == LINHA_COMANDO && r.cmd == CMD_FRENTE,
              "hexadecimal minusculo e aceito (CMD:frente*7e)");

    verificar(invalida("CMD:frente") && invalida("CMD:frente7E") &&
                  invalida("CMD:frente 7E") && invalida("CMD:frente#7E"),
              "sem o '*' antes do checksum: invalida");
    verificar(invalida("CMD:frente*7") && invalida("CMD:frente*07E") &&
                  invalida("CMD:frente*"),
              "checksum com 1 ou 3 digitos, ou sem digitos: invalida");
    verificar(invalida("CMD:frente*7G") && invalida("CMD:frente*G7") &&
                  invalida("CMD:frente*  ") && invalida("CMD:frente*-1"),
              "checksum com caractere que nao e hexadecimal: invalida");
    verificar(invalida("CMD:frente*7E ") && invalida("CMD:frente*7EX") &&
                  invalida("CMD:frente*7E\r") && invalida("CMD:frente*7E\n") &&
                  invalida("CMD:frente*7E*7E"),
              "qualquer coisa depois do checksum: invalida");
    verificar(invalida("") && invalida("*00") && invalida(NULL),
              "linha vazia, so checksum ou ponteiro nulo: invalida");

    const char* conteudosRuins[] = {
        "cmd:frente", "Cmd:frente", "mnt:frente", "Mnt:tras",  "CMD;frente",
        "CMDfrente",  " CMD:frente", "xCMD:frente", "CMD :frente", "CMD:",
        "MNT:",       "CMD:frentex", "CMD:frent", "CMD:xfrente", "CMD: frente",
        "CMD:frente ", "CMD:FRENTE", "CMD:Frente", "CMD:re",     "CMD:tras ",
        "CMD:fre*nte", "MNT:frentes", "CMD:frente\t", "CMD:tr\xc3\xa1s",
        "CMD:parar,frente", "CMD:frente\x01"};
    const int TOTAL_RUINS = sizeof(conteudosRuins) / sizeof(conteudosRuins[0]);
    int aceitasRuins = 0;
    for (int i = 0; i < TOTAL_RUINS; ++i) {
      char s[48];
      montar(s, sizeof(s), conteudosRuins[i], false);
      if (!invalida(s)) {
        ++aceitasRuins;
        printf("    aceitou: \"%s\"\n", s);
      }
    }
    snprintf(texto, sizeof(texto),
             "%d variacoes com checksum CERTO (cmd:, CMD;, frentex, frent, "
             "FRENTE, re, tras...): todas invalidas",
             TOTAL_RUINS);
    verificar(aceitasRuins == 0, texto);

    char longa[128];
    memset(longa, 'A', 100);
    strcpy(longa + 100, "*00");
    verificar(invalida(longa), "linha de 100 bytes: invalida, sem estourar nada");
  }

  // ------------------------------------------------------------------
  titulo("5. Telemetria e eventos: texto exato");
  {
    char buf[TAM_LINHA_APP];
    DadosTelemetria d;
    d.ms = 15230;
    d.estado = EST_FRENTE;
    d.pwm_esq = 130;
    d.pwm_dir = 130;
    d.nivel = 1;
    d.obstaculo = false;
    d.emergencia = false;
    d.motivo = PARADA_NENHUMA;
    d.voz = true;
    d.app = true;
    unsigned int n = formatarTelemetria(buf, sizeof(buf), d);
    const char* esperado =
        "{\"t\":\"tel\",\"ms\":15230,\"estado\":\"FRENTE\",\"pwmEsq\":130,"
        "\"pwmDir\":130,\"nivel\":1,\"obstaculo\":0,\"emergencia\":0,"
        "\"motivo\":\"NENHUMA\",\"voz\":1,\"app\":1}*7F\n";
    verificar(strcmp(buf, esperado) == 0 && n == strlen(esperado) && n == 144,
              "telemetria: campos na ordem fixa, checksum e '\\n' (144 bytes)");

    d.ms = 4294967295UL;
    d.estado = EST_RE;
    d.pwm_esq = -255;
    d.pwm_dir = -130;
    d.nivel = 3;
    d.obstaculo = true;
    d.emergencia = true;
    d.motivo = PARADA_TEMPO_LIMITE;
    d.voz = false;
    d.app = false;
    n = formatarTelemetria(buf, sizeof(buf), d);
    esperado =
        "{\"t\":\"tel\",\"ms\":4294967295,\"estado\":\"RE\",\"pwmEsq\":-255,"
        "\"pwmDir\":-130,\"nivel\":3,\"obstaculo\":1,\"emergencia\":1,"
        "\"motivo\":\"TEMPO_LIMITE\",\"voz\":0,\"app\":0}*04\n";
    verificar(strcmp(buf, esperado) == 0 && n == strlen(esperado),
              "telemetria com PWM negativo e ms maximo (4294967295)");

    d.ms = 0;
    d.estado = EST_GIRO_ESQ;
    d.pwm_esq = -91;
    d.pwm_dir = 91;
    d.nivel = 2;
    d.obstaculo = false;
    d.emergencia = false;
    d.motivo = PARADA_FIM_DO_GIRO;
    d.voz = true;
    d.app = true;
    formatarTelemetria(buf, sizeof(buf), d);
    esperado =
        "{\"t\":\"tel\",\"ms\":0,\"estado\":\"GIRO_ESQ\",\"pwmEsq\":-91,"
        "\"pwmDir\":91,\"nivel\":2,\"obstaculo\":0,\"emergencia\":0,"
        "\"motivo\":\"FIM_DO_GIRO\",\"voz\":1,\"app\":1}*5F\n";
    verificar(strcmp(buf, esperado) == 0,
              "telemetria de giro: rodas em sentidos opostos, ms 0");

    const Estado estados[] = {EST_PARADA, EST_FRENTE, EST_RE, EST_GIRO_ESQ,
                              EST_GIRO_DIR};
    const char* nomesEstado[] = {"PARADA", "FRENTE", "RE", "GIRO_ESQ",
                                 "GIRO_DIR"};
    bool todosEstados = true;
    for (int i = 0; i < 5; ++i) {
      d.estado = estados[i];
      n = formatarTelemetria(buf, sizeof(buf), d);
      char campo[32];
      snprintf(campo, sizeof(campo), "\"estado\":\"%s\"", nomesEstado[i]);
      if (strstr(buf, campo) == NULL || !linhaDeSaidaValida(buf, n)) {
        todosEstados = false;
      }
    }
    verificar(todosEstados, "os cinco estados saem com o nome de nomeEstado()");

    n = formatarTelemetria(buf, sizeof(buf), telemetriaMaxima());
    snprintf(texto, sizeof(texto),
             "a maior telemetria possivel tem %u bytes e cabe no buffer de %u",
             n, TAM_LINHA_APP);
    verificar(n == 158 && linhaDeSaidaValida(buf, n), texto);
  }
  {
    char buf[TAM_LINHA_APP];
    // Cada chamada e conferida logo em seguida, porque todas usam buf.
    unsigned int n = formatarEventoInicio(buf, sizeof(buf), 120, true);
    verificar(strcmp(buf, "{\"t\":\"evt\",\"ms\":120,\"tipo\":\"inicio\","
                          "\"voz\":1}*4F\n") == 0 &&
                  n == 48,
              "evento inicio com modulo de voz");
    formatarEventoInicio(buf, sizeof(buf), 120, false);
    verificar(strcmp(buf, "{\"t\":\"evt\",\"ms\":120,\"tipo\":\"inicio\","
                          "\"voz\":0}*4E\n") == 0,
              "evento inicio sem modulo de voz");

    n = formatarEventoComando(buf, sizeof(buf), 15100, CMD_FRENTE, "voz", true);
    verificar(strcmp(buf, "{\"t\":\"evt\",\"ms\":15100,\"tipo\":\"comando\","
                          "\"cmd\":\"FRENTE\",\"origem\":\"voz\","
                          "\"aceito\":1}*4D\n") == 0 &&
                  n == 84,
              "evento comando aceito pela voz");
    formatarEventoComando(buf, sizeof(buf), 4294967295UL, CMD_RE, "app", false);
    verificar(strcmp(buf, "{\"t\":\"evt\",\"ms\":4294967295,\"tipo\":\"comando\","
                          "\"cmd\":\"RE\",\"origem\":\"app\",\"aceito\":0}*6F\n") == 0,
              "evento comando recusado do app, ms maximo");

    formatarEventoParada(buf, sizeof(buf), 18100, PARADA_TEMPO_LIMITE);
    verificar(strcmp(buf, "{\"t\":\"evt\",\"ms\":18100,\"tipo\":\"parada\","
                          "\"motivo\":\"TEMPO_LIMITE\"}*10\n") == 0,
              "evento parada por tempo limite");
    formatarEventoParada(buf, sizeof(buf), 4294967295UL, PARADA_EMERGENCIA);
    verificar(strcmp(buf, "{\"t\":\"evt\",\"ms\":4294967295,\"tipo\":\"parada\","
                          "\"motivo\":\"EMERGENCIA\"}*31\n") == 0,
              "evento parada por emergencia, ms maximo");

    formatarEventoObstaculo(buf, sizeof(buf), 19000, true);
    bool ap = strcmp(buf, "{\"t\":\"evt\",\"ms\":19000,\"tipo\":\"obstaculo\","
                          "\"ativo\":1}*37\n") == 0;
    formatarEventoObstaculo(buf, sizeof(buf), 19000, false);
    verificar(ap && strcmp(buf, "{\"t\":\"evt\",\"ms\":19000,\"tipo\":\"obstaculo\","
                                "\"ativo\":0}*36\n") == 0,
              "evento obstaculo: apareceu e sumiu");

    formatarEventoEmergencia(buf, sizeof(buf), 20000, true);
    bool ac = strcmp(buf, "{\"t\":\"evt\",\"ms\":20000,\"tipo\":\"emergencia\","
                          "\"ativa\":1}*55\n") == 0;
    formatarEventoEmergencia(buf, sizeof(buf), 20000, false);
    verificar(ac && strcmp(buf, "{\"t\":\"evt\",\"ms\":20000,\"tipo\":\"emergencia\","
                                "\"ativa\":0}*54\n") == 0,
              "evento emergencia: apertado e solto");

    const MotivoParada motivos[] = {PARADA_NENHUMA,     PARADA_COMANDO,
                                    PARADA_OBSTACULO,   PARADA_TEMPO_LIMITE,
                                    PARADA_FIM_DO_GIRO, PARADA_EMERGENCIA};
    const char* codigos[] = {"NENHUMA",      "COMANDO",     "OBSTACULO",
                             "TEMPO_LIMITE", "FIM_DO_GIRO", "EMERGENCIA"};
    bool todosMotivos = true;
    for (int i = 0; i < 6; ++i) {
      n = formatarEventoParada(buf, sizeof(buf), 1, motivos[i]);
      char campo[32];
      snprintf(campo, sizeof(campo), "\"motivo\":\"%s\"}", codigos[i]);
      if (strstr(buf, campo) == NULL || !linhaDeSaidaValida(buf, n)) {
        todosMotivos = false;
      }
    }
    verificar(todosMotivos, "parada sai com o codigo de cada motivo");

    const char* nomesCmd[] = {"FRENTE", "RE",     "ESQUERDA", "DIREITA",
                              "PARAR",  "RAPIDO", "DEVAGAR"};
    const char* origens[] = {"voz", "teclado", "app"};
    bool todosCmd = true;
    for (int i = 0; i < CMD_TOTAL; ++i) {
      for (int o = 0; o < 3; ++o) {
        n = formatarEventoComando(buf, sizeof(buf), 1, (Comando)i, origens[o],
                                  o == 1);
        char campo[64];
        snprintf(campo, sizeof(campo),
                 "\"cmd\":\"%s\",\"origem\":\"%s\",\"aceito\":%d}", nomesCmd[i],
                 origens[o], o == 1 ? 1 : 0);
        if (strstr(buf, campo) == NULL || !linhaDeSaidaValida(buf, n)) {
          todosCmd = false;
        }
      }
    }
    verificar(todosCmd, "comando sai com nomeComando() e a origem, para os 7 comandos");
  }

  // ------------------------------------------------------------------
  titulo("6. Linha que nao cabe nao sai pela metade");
  {
    const char* origensRuins[] = {NULL, "", "a\"b", "a\\b", "voz\n",
                                  "tr\xc3\xa1s"};
    bool recusou = true;
    for (int i = 0; i < 6; ++i) {
      char buf[TAM_LINHA_APP];
      memset(buf, 'Z', sizeof(buf));
      unsigned int n = formatarEventoComando(buf, sizeof(buf), 1, CMD_FRENTE,
                                             origensRuins[i], true);
      if (n != 0 || buf[0] != '\0') recusou = false;
    }
    verificar(recusou,
              "origem com aspas, barra, quebra de linha, acento ou vazia: retorna 0");

    bool todosOk = true;
    for (int f = 0; f < TOTAL_FORMATADORES; ++f) {
      char cheia[TAM_LINHA_APP];
      unsigned int n = formatarCaso(f, cheia, sizeof(cheia));
      if (n == 0 || !linhaDeSaidaValida(cheia, n)) {
        todosOk = false;
        continue;
      }
      // Toda capacidade de 0 ate o tamanho exato (n + 1, com o '\0').
      for (unsigned int cap = 0; cap <= n + 1; ++cap) {
        char buf[TAM_LINHA_APP + 16];
        memset(buf, 0x5A, sizeof(buf));
        unsigned int r = formatarCaso(f, buf, cap);
        bool alemIntacto = true;
        for (size_t k = cap; k < sizeof(buf); ++k) {
          if (buf[k] != 0x5A) alemIntacto = false;
        }
        if (!alemIntacto) todosOk = false;
        if (cap < n + 1) {
          if (r != 0) todosOk = false;
          if (cap > 0 && buf[0] != '\0') todosOk = false;
        } else if (r != n || strcmp(buf, cheia) != 0) {
          todosOk = false;
        }
      }
    }
    verificar(todosOk,
              "capacidade pequena: retorna 0, deixa buf vazio e nao escreve "
              "alem de cap");
    verificar(formatarCaso(0, NULL, 0) == 0 && formatarCaso(2, NULL, 0) == 0,
              "capacidade 0 com ponteiro nulo nao escreve nada");
  }

  // ------------------------------------------------------------------
  titulo("7. Fila de saida");
  {
    FilaSaida f;
    verificar(f.pendentes() == 0 && f.livres() == TAM_FILA_APP &&
                  f.tirar() == '\0' && f.pendentes() == 0,
              "comeca vazia; tirar da fila vazia nao faz nada");

    f.colocar("abc", 3);
    char a = f.tirar();
    char b = f.tirar();
    char c = f.tirar();
    verificar(a == 'a' && b == 'b' && c == 'c' && f.pendentes() == 0,
              "sai na mesma ordem em que entrou");

    char bloco[TAM_FILA_APP];
    memset(bloco, 'x', sizeof(bloco));
    f.colocar(bloco, TAM_FILA_APP - 2);
    bool recusou = !f.colocar("123", 3) && f.pendentes() == TAM_FILA_APP - 2;
    bool coube = f.colocar("12", 2) && f.livres() == 0;
    verificar(recusou && coube && !f.colocar("1", 1),
              "linha que nao cabe inteira nao entra nem pela metade");
    verificar(!f.colocar(NULL, 3) && !f.colocar("x", 0),
              "ponteiro nulo ou tamanho zero nao entra");
  }
  {
    // O laco do Mega: linhas entram, e a cada volta sai o que cabe no buffer
    // da serial (0 a 63 bytes). O que saiu tem de ser exatamente a sequencia
    // das linhas aceitas, inteiras, mesmo dando muitas voltas no buffer.
    FilaSaida f;
    static char esperado[400000];
    static char saida[400000];
    size_t tamEsperado = 0;
    size_t tamSaida = 0;
    long aceitas = 0;
    long recusadas = 0;
    bool estourou = false;  // so com a fila errada: mais saida que entrada
    for (int volta = 0; volta < 5000 && !estourou; ++volta) {
      int quantas = (int)(aleatorio() % 3);
      for (int k = 0; k < quantas; ++k) {
        char linha[TAM_LINHA_APP];
        unsigned int n;
        if (aleatorio() % 2) {
          DadosTelemetria d = telemetriaMaxima();
          d.ms = aleatorio();
          d.pwm_esq = (int)(aleatorio() % 511) - 255;
          n = formatarTelemetria(linha, sizeof(linha), d);
        } else {
          n = formatarEventoComando(linha, sizeof(linha), aleatorio(),
                                    (Comando)(aleatorio() % CMD_TOTAL), "app",
                                    aleatorio() % 2 != 0);
        }
        if (f.colocar(linha, n)) {
          if (tamEsperado + n > sizeof(esperado)) {
            estourou = true;
            break;
          }
          memcpy(esperado + tamEsperado, linha, n);
          tamEsperado += n;
          ++aceitas;
        } else {
          ++recusadas;
        }
      }
      int espaco = (int)(aleatorio() % 64);
      while (espaco-- > 0 && f.pendentes() > 0 && tamSaida < sizeof(saida)) {
        saida[tamSaida++] = f.tirar();
      }
    }
    while (f.pendentes() > 0 && tamSaida < sizeof(saida)) {
      saida[tamSaida++] = f.tirar();
    }

    bool igual = !estourou && tamSaida == tamEsperado &&
                 memcmp(saida, esperado, tamEsperado) == 0;
    bool linhasInteiras = true;
    size_t inicio = 0;
    for (size_t i = 0; i < tamSaida; ++i) {
      if (saida[i] != '\n') continue;
      char linha[TAM_LINHA_APP + 1];
      size_t n = i + 1 - inicio;
      if (n > TAM_LINHA_APP) {
        linhasInteiras = false;
      } else {
        memcpy(linha, saida + inicio, n);
        linha[n] = '\0';
        if (!linhaDeSaidaValida(linha, (unsigned int)n)) linhasInteiras = false;
      }
      inicio = i + 1;
    }
    snprintf(texto, sizeof(texto),
             "%ld linhas aceitas e %ld recusadas com a fila cheia: saem "
             "inteiras e em ordem",
             aceitas, recusadas);
    verificar(igual && linhasInteiras && inicio == tamSaida && recusadas > 0,
              texto);
  }
  {
    char buf[TAM_LINHA_APP];
    unsigned int tel = formatarTelemetria(buf, sizeof(buf), telemetriaMaxima());
    unsigned int cmd = formatarCaso(2, buf, sizeof(buf));
    unsigned int par = formatarCaso(3, buf, sizeof(buf));
    unsigned int obs = formatarCaso(4, buf, sizeof(buf));
    snprintf(texto, sizeof(texto),
             "fila de %u bytes: maior telemetria (%u) + 2 comandos (%u) + "
             "parada (%u) + obstaculo (%u) cabem juntos",
             TAM_FILA_APP, tel, cmd, par, obs);
    verificar(tel + 2 * cmd + par + obs <= TAM_FILA_APP, texto);
  }

  // ------------------------------------------------------------------
  titulo("8. Leitor volta a sincronizar sozinho (lixo do boot do ESP)");
  {
    LeitorLinhas l;
    alimentarTexto(l, "CMD:fre");
    l.reiniciar();
    Contagem c = alimentarTexto(l, "CMD:parar*00\n");
    verificar(c.validas == 1 && strcmp(l.linha(), "CMD:parar*00") == 0,
              "reiniciar() esquece a linha pela metade: a seguinte vale inteira");

    alimentarTexto(l, "lixo\x01sem fim");  // fica descartando
    l.reiniciar();
    c = alimentarTexto(l, "CMD:parar*00\n");
    verificar(c.validas == 1, "reiniciar() tambem encerra um descarte em andamento");

    l.reiniciar();
    l.reiniciar();
    c = alimentarTexto(l, "CMD:frente*7E\n");
    verificar(c.validas == 1, "reiniciar() no inicio de linha nao perde nada");
  }
  {
    // Cada pedaco do lixo do boot (de 0 a 216 bytes, porque o Mega pode ter
    // pego so o fim dele), seguido do primeiro comando do app: um parar.
    bool comBarraN = true;
    bool comSilencio = true;
    bool lixoQuieto = true;
    for (size_t k = 0; k <= TAM_LIXO_BOOT; ++k) {
      const char* inicio = LIXO_BOOT_ESP + (TAM_LIXO_BOOT - k);
      // Como o ESP manda (ESPEC 2.3): '\n' antes da linha, sem pausa.
      LeitorLinhas a;
      if (alimentarNoTempo(a, inicio, k, 1000).linhas != 0) lixoQuieto = false;
      Contagem c = alimentarTextoNoTempo(a, "\nCMD:parar*00\n", 1000);
      if (c.validas != 1 || strcmp(a.linha(), "CMD:parar*00") != 0) {
        comBarraN = false;
      }
      // Sem o '\n' antes, mas depois do silencio: o Mega sincroniza sozinho.
      LeitorLinhas b;
      alimentarNoTempo(b, inicio, k, 1000);
      c = alimentarTextoNoTempo(b, "CMD:parar*00\n",
                                1000 + SILENCIO_LEITOR_APP_MS);
      if (c.validas != 1 || strcmp(b.linha(), "CMD:parar*00") != 0) {
        comSilencio = false;
      }
    }
    verificar(lixoQuieto, "lixo do boot sem '\\n' nao forma nenhuma linha");
    verificar(comBarraN,
              "com o '\\n' que o ESP manda antes, o parar logo depois do lixo "
              "e lido");
    snprintf(texto, sizeof(texto),
             "sem esse '\\n', o parar que chega %lu ms depois do lixo tambem "
             "e lido",
             SILENCIO_LEITOR_APP_MS);
    verificar(comSilencio, texto);

    // O caso do revisor: lixo chegando a 115200 (uns 11 bytes por ms), o laco
    // lendo a cada 1 ms, e o parar SILENCIO ms depois do ultimo byte.
    LeitorLinhas l;
    unsigned long t = 2000;
    for (size_t i = 0; i < TAM_LIXO_BOOT; i += 11, ++t) {
      size_t n = TAM_LIXO_BOOT - i < 11 ? TAM_LIXO_BOOT - i : 11;
      alimentarNoTempo(l, LIXO_BOOT_ESP + i, n, t);
    }
    Contagem c = alimentarTextoNoTempo(l, "CMD:parar*00\n",
                                       t - 1 + SILENCIO_LEITOR_APP_MS);
    verificar(c.validas == 1 && strcmp(l.linha(), "CMD:parar*00") == 0,
              "lixo do boot chegando aos poucos e parar depois: parar e lido");
  }
  {
    LeitorLinhas l;
    unsigned long t = 5000;
    alimentarTextoNoTempo(l, "CMD:fre", t);
    Contagem c =
        alimentarTextoNoTempo(l, "nte*7E\n", t + SILENCIO_LEITOR_APP_MS - 1);
    verificar(c.validas == 1 && strcmp(l.linha(), "CMD:frente*7E") == 0,
              "linha que chega em duas voltas do laco, com pausa menor que o "
              "silencio, nao e cortada");

    t += 1000;
    alimentarTextoNoTempo(l, "CMD:fre", t);
    c = alimentarTextoNoTempo(l, "nte*7E\n", t + SILENCIO_LEITOR_APP_MS);
    verificar(c.linhas == 1 && c.validas == 0,
              "com o silencio no meio, a metade velha sai e o resto nao vira "
              "comando");

    t += 1000;
    alimentarTextoNoTempo(l, "CMD:fre", t);
    l.descartarLinhaAtual();  // emergencia apertada no meio da linha
    c = alimentarTextoNoTempo(l, "nte*7E\n", t + SILENCIO_LEITOR_APP_MS);
    Contagem depois = alimentarTextoNoTempo(l, "CMD:parar*00\n", t + 2000);
    verificar(c.validas == 0 && depois.validas == 1,
              "linha descartada na emergencia continua sem virar comando, "
              "mesmo apos o silencio");

    // millis() da a volta (49,7 dias no Mega). Aqui a volta e a do unsigned
    // long do PC: a subtracao sem sinal e a mesma nas duas larguras.
    unsigned long antes = ~0UL - 9;  // 10 ms antes da volta
    alimentarTextoNoTempo(l, "CMD:fre", antes);
    c = alimentarTextoNoTempo(l, "nte*7E\n", antes + 30);
    verificar(c.validas == 1,
              "na volta do millis(), 30 ms continua sendo pausa curta");
    alimentarTextoNoTempo(l, "lixo\x01", antes);
    c = alimentarTextoNoTempo(l, "CMD:parar*00\n",
                              antes + SILENCIO_LEITOR_APP_MS);
    verificar(c.validas == 1,
              "e o silencio atravessando a volta tambem sincroniza");
  }
  {
    // Rajadas de lixo (0x00 a 0xFF, as vezes com '\n'), pausas sorteadas e
    // comandos no meio. Comando depois do silencio, ou com o '\n' antes como
    // o ESP manda, tem de ser lido. Comando colado no lixo pode se perder,
    // mas nunca vira outro. E o lixo sozinho nunca vira comando.
    LeitorLinhas l;
    unsigned long t = 1000;
    long obrigatorios = 0;
    long lidosObrig = 0;
    long colados = 0;
    long lidosColados = 0;
    long errados = 0;
    for (int k = 0; k < 200000; ++k) {
      char lixo[80];
      size_t n = aleatorio() % sizeof(lixo);
      for (size_t i = 0; i < n; ++i) lixo[i] = (char)(aleatorio() >> 24);
      t += aleatorio() % (2 * SILENCIO_LEITOR_APP_MS);
      if (alimentarNoTempo(l, lixo, n, t).validas != 0) ++errados;

      const Valida& v = VALIDAS[aleatorio() % TOTAL_VALIDAS];
      char s[24];
      int modo = (int)(aleatorio() % 3);
      if (modo == 0) {
        t += SILENCIO_LEITOR_APP_MS + aleatorio() % 100;
        snprintf(s, sizeof(s), "%s\n", v.linha);
      } else if (modo == 1) {
        t += aleatorio() % SILENCIO_LEITOR_APP_MS;
        snprintf(s, sizeof(s), "\n%s\n", v.linha);
      } else {
        t += aleatorio() % SILENCIO_LEITOR_APP_MS;
        snprintf(s, sizeof(s), "%s\n", v.linha);
      }
      Contagem c = alimentarTextoNoTempo(l, s, t);
      bool igual = c.validas == 1 && strcmp(l.linha(), v.linha) == 0;
      if (c.validas > 1 || (c.validas == 1 && !igual)) ++errados;
      if (modo < 2) {
        ++obrigatorios;
        if (igual) ++lidosObrig;
      } else {
        ++colados;
        if (igual) ++lidosColados;
      }
    }
    snprintf(texto, sizeof(texto),
             "lixo e pausas sorteados: %ld de %ld comandos depois de silencio "
             "ou '\\n' lidos",
             lidosObrig, obrigatorios);
    verificar(lidosObrig == obrigatorios && obrigatorios > 100000, texto);
    snprintf(texto, sizeof(texto),
             "%ld comandos colados no lixo (%ld lidos): nenhum lido errado, "
             "nenhum comando vindo do lixo",
             colados, lidosColados);
    verificar(errados == 0, texto);
  }

  // ------------------------------------------------------------------
  titulo("9. O que o Mega faz com cada linha do app");
  {
    // Tabela escrita a mao, da linha crua ate a acao, nos quatro casos de
    // PERMITIR_COMANDO_APP (1 e 0) com e sem emergencia.
    struct Caso {
      const char* linha;
      AcaoApp permitido;
      AcaoApp soAcompanha;
      AcaoApp emergPermitido;
      AcaoApp emergSoAcompanha;
    };
    const AcaoApp T = ACAO_TRATAR;
    const AcaoApp R = ACAO_RECUSAR;
    const AcaoApp N = ACAO_RENOVAR;
    const AcaoApp I = ACAO_IGNORAR;
    const Caso casos[] = {
        {"CMD:frente*7E", T, R, I, I},   {"CMD:tras*64", T, R, I, I},
        {"CMD:esquerda*70", T, R, I, I}, {"CMD:direita*16", T, R, I, I},
        {"CMD:parar*00", T, T, I, I},    {"CMD:rapido*71", T, R, I, I},
        {"CMD:devagar*12", T, R, I, I},  {"MNT:frente*63", N, I, I, I},
        {"MNT:tras*79", N, I, I, I},     {"MNT:parar*1D", I, I, I, I},
        {"MNT:esquerda*6D", I, I, I, I}, {"MNT:rapido*6C", I, I, I, I},
        {"CMD:FRENTE*7E", I, I, I, I},   {"CMD:re*67", I, I, I, I},
        {"CMD:frente*7F", I, I, I, I},   {"CMD:parar*01", I, I, I, I},
        {"", I, I, I, I},
    };
    const int TOTAL_CASOS = sizeof(casos) / sizeof(casos[0]);
    for (int i = 0; i < TOTAL_CASOS; ++i) {
      LinhaApp l = interpretarLinhaApp(casos[i].linha);
      AcaoApp a = decidirAcaoApp(l, true, false);
      AcaoApp b = decidirAcaoApp(l, false, false);
      AcaoApp c = decidirAcaoApp(l, true, true);
      AcaoApp d = decidirAcaoApp(l, false, true);
      snprintf(texto, sizeof(texto),
               "\"%s\": %s | so acompanha: %s | emergencia: %s / %s",
               casos[i].linha, nomeAcao(casos[i].permitido),
               nomeAcao(casos[i].soAcompanha), nomeAcao(casos[i].emergPermitido),
               nomeAcao(casos[i].emergSoAcompanha));
      verificar(a == casos[i].permitido && b == casos[i].soAcompanha &&
                    c == casos[i].emergPermitido &&
                    d == casos[i].emergSoAcompanha,
                texto);
    }
  }
  {
    // Todas as combinacoes de tipo x comando x permitir x emergencia, ate as
    // que interpretarLinhaApp nunca gera (CMD_NENHUM, MNT de giro): as travas
    // valem para qualquer entrada.
    const TipoLinhaApp tipos[] = {LINHA_INVALIDA, LINHA_COMANDO,
                                  LINHA_RENOVACAO};
    int combinacoes = 0;
    bool emergencia = true;
    bool invalidaIgnorada = true;
    bool parar = true;
    bool permitido = true;
    bool soAcompanha = true;
    bool renovacao = true;
    bool foraDaLista = true;
    for (int t = 0; t < 3; ++t) {
      for (int k = CMD_NENHUM; k < CMD_TOTAL; ++k) {
        for (int p = 0; p < 2; ++p) {
          for (int e = 0; e < 2; ++e) {
            LinhaApp l;
            l.tipo = tipos[t];
            l.cmd = (Comando)k;
            AcaoApp a = decidirAcaoApp(l, p != 0, e != 0);
            ++combinacoes;
            bool cmd = l.tipo == LINHA_COMANDO;
            bool mnt = l.tipo == LINHA_RENOVACAO;
            bool naLista = k >= CMD_FRENTE && k < CMD_TOTAL;
            bool renovavel = k == CMD_FRENTE || k == CMD_RE;

            if (e && a != ACAO_IGNORAR) emergencia = false;
            if (l.tipo == LINHA_INVALIDA && a != ACAO_IGNORAR) {
              invalidaIgnorada = false;
            }
            if (!naLista && a != ACAO_IGNORAR) foraDaLista = false;
            if (e || !naLista) continue;

            if (cmd && k == CMD_PARAR && a != ACAO_TRATAR) parar = false;
            if (p && cmd && a != ACAO_TRATAR) permitido = false;
            if (p && mnt && a != (renovavel ? ACAO_RENOVAR : ACAO_IGNORAR)) {
              renovacao = false;
            }
            if (!p && cmd && k != CMD_PARAR && a != ACAO_RECUSAR) {
              soAcompanha = false;
            }
            if (!p && mnt && a != ACAO_IGNORAR) soAcompanha = false;
          }
        }
      }
    }
    snprintf(texto, sizeof(texto),
             "%d combinacoes de tipo, comando, permitir e emergencia conferidas",
             combinacoes);
    verificar(combinacoes == 3 * 8 * 2 * 2, texto);
    verificar(emergencia,
              "na emergencia tudo e ignorado, ate parar: nada fica guardado "
              "para quando o botao for solto");
    verificar(invalidaIgnorada, "linha invalida e sempre ignorada");
    verificar(foraDaLista,
              "comando fora da lista e ignorado: nunca chega a tratarComando");
    verificar(parar,
              "CMD:parar e sempre tratado, com o app comandando ou so "
              "acompanhando");
    verificar(permitido,
              "com PERMITIR_COMANDO_APP 1, todo CMD passa pela maquina de "
              "estados (que tem as proprias travas)");
    verificar(renovacao,
              "MNT so renova frente e tras; nunca vira comando, bip ou evento");
    verificar(soAcompanha,
              "com PERMITIR_COMANDO_APP 0, todo CMD que nao e parar e "
              "recusado e todo MNT e ignorado");
  }

  // ------------------------------------------------------------------
  printf("\n----------------------------------------\n");
  printf("Total: %d   Falhas: %d\n", total, falhas);
  if (falhas == 0) printf("Todos os testes passaram.\n");
  return falhas == 0 ? 0 : 1;
}
