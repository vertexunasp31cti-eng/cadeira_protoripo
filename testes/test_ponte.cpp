/*
 * test_ponte.cpp - Testes da ponte do ESP8266 (namespace ponte), no PC.
 *
 * Compilar e rodar:  make -C testes
 *
 * A ponte e o unico caminho entre o celular e a cadeira. Estes testes cobrem
 * o que nao pode falhar: ruido na serial nunca vira dado, o JSON do
 * /api/estado e sempre valido, "parar" nunca e bloqueado e comando atrasado
 * pela rede nunca chega ao Mega.
 *
 * Com um caminho de arquivo como argumento, grava cada JSON de estado gerado
 * (um por linha) nesse arquivo, para conferir com um leitor de JSON de
 * verdade:  ./build/test_ponte estados.txt
 */
#include "../firmware/esp_wifi/ponte.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

using ponte::PonteCadeira;

static int total = 0;
static int falhas = 0;
static FILE* arquivo_json = NULL;

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

// Checksum feito de novo aqui, sem usar o da ponte, para o teste nao
// concordar com um erro so porque chama o mesmo codigo.
static std::string comChecksum(const std::string& conteudo) {
  unsigned int x = 0;
  for (size_t i = 0; i < conteudo.size(); ++i) x ^= (unsigned char)conteudo[i];
  char hh[4];
  snprintf(hh, sizeof(hh), "%02X", x);
  return conteudo + "*" + hh;
}

// Linha completa do Mega, como sai da Serial3: "<json>*HH\n".
static std::string linhaMega(const std::string& conteudo) {
  return comChecksum(conteudo) + "\n";
}

static void alimentar(PonteCadeira& p, const std::string& bytes,
                      unsigned long agora) {
  for (size_t i = 0; i < bytes.size(); ++i) p.receberByte(bytes[i], agora);
}

static std::string telemetria(unsigned long ms, const char* estado) {
  char s[220];
  snprintf(s, sizeof(s),
           "{\"t\":\"tel\",\"ms\":%lu,\"estado\":\"%s\",\"pwmEsq\":130,"
           "\"pwmDir\":-130,\"nivel\":1,\"obstaculo\":0,\"emergencia\":0,"
           "\"motivo\":\"NENHUMA\",\"voz\":1,\"app\":1}",
           ms, estado);
  return s;
}

static std::string evento(unsigned long ms) {
  char s[120];
  snprintf(s, sizeof(s),
           "{\"t\":\"evt\",\"ms\":%lu,\"tipo\":\"obstaculo\",\"ativo\":1}", ms);
  return s;
}

static ponte::TipoLinhaMega tipoDe(const std::string& linha_sem_fim) {
  unsigned int tam = 0;
  return ponte::validarLinhaMega(linha_sem_fim.c_str(),
                                 (unsigned int)linha_sem_fim.size(), &tam);
}

static std::string estado(const PonteCadeira& p, unsigned long agora,
                          unsigned long desde, int clientes = 1) {
  static char buf[ponte::TAM_ESTADO];
  unsigned int n = p.montarEstado(buf, sizeof(buf), agora, desde, clientes);
  if (n == 0) return "";
  if (arquivo_json != NULL) fprintf(arquivo_json, "%s\n", buf);
  return std::string(buf, n);
}

static int contar(const std::string& s, const char* trecho) {
  int n = 0;
  size_t pos = 0;
  while ((pos = s.find(trecho, pos)) != std::string::npos) {
    ++n;
    pos += strlen(trecho);
  }
  return n;
}

// seq do primeiro evento listado, ou 0 se a lista estiver vazia.
static unsigned long primeiroSeq(const std::string& json) {
  const char* marca = "\"eventos\":[{\"seq\":";
  size_t pos = json.find(marca);
  if (pos == std::string::npos) return 0;
  return strtoul(json.c_str() + pos + strlen(marca), NULL, 10);
}

// seq dos eventos na ordem em que aparecem; true se forem crescentes de 1 em 1
// a partir de "de" e terminarem em "ate".
static bool seqsEmOrdem(const std::string& json, unsigned long de,
                        unsigned long ate) {
  const char* marca = "{\"seq\":";
  size_t pos = json.find("\"eventos\":[");
  unsigned long esperado = de;
  while ((pos = json.find(marca, pos)) != std::string::npos) {
    pos += strlen(marca);
    if (strtoul(json.c_str() + pos, NULL, 10) != esperado) return false;
    ++esperado;
  }
  return esperado == ate + 1;
}

// Gerador pseudoaleatorio com semente fixa: o teste de ruido da sempre o
// mesmo resultado.
static uint32_t semente = 2463534242u;
static uint32_t aleatorio() {
  semente ^= semente << 13;
  semente ^= semente >> 17;
  semente ^= semente << 5;
  return semente;
}

// Receptor das linhas de comando como o ESPEC (secao 2) descreve o lado do
// Mega: ignora '\r', descarta a linha com byte fora do ASCII imprimivel ou
// maior que o buffer de 32 bytes ate o proximo '\n' e ignora linha vazia.
// Feito de novo aqui, sem o protocolo_app do Mega, pelo mesmo motivo do
// comChecksum. Guarda so a ultima linha completa.
struct ReceptorMega {
  std::string atual;
  std::string ultima;
  bool descartando;

  ReceptorMega() : descartando(false) {}

  void receber(const std::string& bytes) {
    for (size_t i = 0; i < bytes.size(); ++i) receberByte(bytes[i]);
  }

  void receberByte(char c) {
    if (c == '\r') return;
    if (c == '\n') {
      if (!descartando && !atual.empty()) ultima = atual;
      atual.clear();
      descartando = false;
      return;
    }
    if (descartando) return;
    unsigned char u = (unsigned char)c;
    if (u < 0x20 || u > 0x7E || atual.size() >= 31) {
      atual.clear();
      descartando = true;
      return;
    }
    atual += c;
  }
};

static ponte::ResultadoComando comando(PonteCadeira& p, const char* nome,
                                       bool manter, const char* id, long n,
                                       unsigned long agora,
                                       std::string* saida = NULL) {
  char linha[ponte::TAM_COMANDO];
  ponte::ResultadoComando r =
      p.prepararComando(nome, manter, id, n, agora, linha, sizeof(linha));
  if (saida != NULL) *saida = linha;
  return r;
}

int main(int argc, char** argv) {
  if (argc > 1) {
    arquivo_json = fopen(argv[1], "w");
    if (arquivo_json == NULL) {
      printf("nao foi possivel abrir %s\n", argv[1]);
      return 2;
    }
  }

  printf("Testes da ponte serial e da API do ESP8266\n");

  // ------------------------------------------------------------------
  titulo("1. Checksum XOR");
  {
    verificar(ponte::checksumXor("", 0) == 0, "texto vazio da zero");
    verificar(ponte::checksumXor("A", 1) == 0x41, "um byte da ele mesmo");
    verificar(ponte::checksumXor("AB", 2) == 0x03, "\"AB\" da 0x41 ^ 0x42");
    verificar(ponte::checksumXor("CMD:frente", 10) == 0x7E,
              "\"CMD:frente\" da 7E (conferido com Python)");
    verificar(ponte::checksumXor("CMD:parar", 9) == 0x00,
              "\"CMD:parar\" da 00 (conferido com Python)");
    verificar(ponte::checksumXor("ABCD", 2) == 0x03,
              "so os n primeiros bytes entram na conta");
  }

  // ------------------------------------------------------------------
  titulo("2. Leitor de linhas da serial");
  {
    ponte::LeitorSerial l;
    bool fim = false;
    for (const char* c = "abc\n"; *c; ++c) fim = l.receber(*c);
    verificar(fim && strcmp(l.linha(), "abc") == 0 && l.tamanho() == 3,
              "linha completa sai sem o '\\n'");

    fim = false;
    for (const char* c = "ab\r\n"; *c; ++c) fim = l.receber(*c);
    verificar(fim && strcmp(l.linha(), "ab") == 0, "'\\r' e ignorado");

    verificar(!l.receber('\n'), "linha vazia nao e entregue");

    std::string maxima(ponte::TAM_LINHA - 1, 'x');
    for (size_t i = 0; i < maxima.size(); ++i) l.receber(maxima[i]);
    fim = l.receber('\n');
    verificar(fim && l.tamanho() == ponte::TAM_LINHA - 1 &&
                  l.linha()[ponte::TAM_LINHA - 1] == '\0',
              "linha do tamanho maximo (255 caracteres) cabe");

    std::string demais(ponte::TAM_LINHA, 'y');
    bool entregou = false;
    for (size_t i = 0; i < demais.size(); ++i) entregou |= l.receber(demais[i]);
    entregou |= l.receber('\n');
    verificar(!entregou && l.descartadas() == 1,
              "linha com 256 caracteres e descartada e contada");

    fim = false;
    for (const char* c = "ok\n"; *c; ++c) fim = l.receber(*c);
    verificar(fim && strcmp(l.linha(), "ok") == 0,
              "depois do descarte, a linha seguinte chega inteira");

    // O fim de uma linha longa nunca pode ser lido como se fosse linha nova.
    std::string enorme(1000, 'z');
    enorme += "CMD:frente";
    entregou = false;
    for (size_t i = 0; i < enorme.size(); ++i) entregou |= l.receber(enorme[i]);
    entregou |= l.receber('\n');
    verificar(!entregou && l.descartadas() == 2,
              "o final de uma linha longa e descartado junto, ate o '\\n'");
  }

  // ------------------------------------------------------------------
  titulo("3. Validacao das linhas do Mega");
  {
    std::string tel = comChecksum(telemetria(15230, "FRENTE"));
    std::string evt = comChecksum(evento(19000));
    unsigned int tam = 0;
    verificar(ponte::validarLinhaMega(tel.c_str(), (unsigned int)tel.size(),
                                      &tam) == ponte::LINHA_TELEMETRIA &&
                  tam == tel.size() - 3,
              "telemetria boa e aceita (tamanho sem o *HH)");
    verificar(tipoDe(evt) == ponte::LINHA_EVENTO, "evento bom e aceito");

    const std::string exemplos[] = {
        "{\"t\":\"evt\",\"ms\":120,\"tipo\":\"inicio\",\"voz\":1}",
        "{\"t\":\"evt\",\"ms\":15100,\"tipo\":\"comando\",\"cmd\":\"FRENTE\","
        "\"origem\":\"voz\",\"aceito\":1}",
        "{\"t\":\"evt\",\"ms\":18100,\"tipo\":\"parada\",\"motivo\":\"TEMPO_LIMITE\"}",
        "{\"t\":\"evt\",\"ms\":20000,\"tipo\":\"emergencia\",\"ativa\":1}"};
    bool todos = true;
    for (int i = 0; i < 4; ++i) {
      todos = todos && tipoDe(comChecksum(exemplos[i])) == ponte::LINHA_EVENTO;
    }
    verificar(todos, "os eventos de exemplo do contrato sao aceitos");

    // O exemplo de telemetria do contrato (secao 2.1) traz *4F, mas o XOR
    // daquele texto e 7F. A ponte segue a regra, nao o exemplo.
    std::string contrato =
        "{\"t\":\"tel\",\"ms\":15230,\"estado\":\"FRENTE\",\"pwmEsq\":130,"
        "\"pwmDir\":130,\"nivel\":1,\"obstaculo\":0,\"emergencia\":0,"
        "\"motivo\":\"NENHUMA\",\"voz\":1,\"app\":1}";
    verificar(tipoDe(contrato + "*7F") == ponte::LINHA_TELEMETRIA &&
                  tipoDe(contrato + "*4F") == ponte::LINHA_RUIM,
              "exemplo do contrato: aceito com *7F (XOR real), recusado com *4F");

    std::string errado = tel;
    errado[errado.size() - 1] = errado[errado.size() - 1] == '0' ? '1' : '0';
    verificar(tipoDe(errado) == ponte::LINHA_RUIM, "checksum errado e recusado");

    // Procura um conteudo cujo checksum tenha letra, para o teste valer.
    std::string com_letra;
    for (unsigned long ms = 0; ms < 100 && com_letra.empty(); ++ms) {
      std::string l = comChecksum(evento(ms));
      std::string hh = l.substr(l.size() - 2);
      if (hh.find_first_of("ABCDEF") != std::string::npos) com_letra = l;
    }
    std::string minusculo = com_letra;
    for (size_t i = minusculo.size() - 2; i < minusculo.size(); ++i) {
      if (minusculo[i] >= 'A' && minusculo[i] <= 'F') minusculo[i] += 'a' - 'A';
    }
    verificar(!com_letra.empty() && minusculo != com_letra &&
                  tipoDe(minusculo) == ponte::LINHA_EVENTO,
              "checksum em hex minusculo tambem e aceito");

    std::string controle = evento(5);
    controle.insert(12, 1, '\x01');
    std::string alto = evento(5);
    alto.insert(12, 1, '\xC3');
    std::string tab = evento(5);
    tab.insert(11, 1, '\t');
    verificar(tipoDe(comChecksum(controle)) == ponte::LINHA_RUIM &&
                  tipoDe(comChecksum(alto)) == ponte::LINHA_RUIM &&
                  tipoDe(comChecksum(tab)) == ponte::LINHA_RUIM,
              "byte fora do ASCII imprimivel derruba a linha, mesmo com checksum certo");

    verificar(tipoDe(comChecksum("{\"t\":\"xyz\",\"ms\":1}")) == ponte::LINHA_RUIM &&
                  tipoDe(comChecksum("{\"t\": \"tel\",\"ms\":1}")) == ponte::LINHA_RUIM &&
                  tipoDe(comChecksum("{\"tipo\":\"tel\",\"ms\":1}")) == ponte::LINHA_RUIM &&
                  tipoDe(comChecksum("CMD:frente")) == ponte::LINHA_RUIM,
              "prefixo diferente de {\"t\":\"tel\", e {\"t\":\"evt\", e recusado");

    verificar(tipoDe(comChecksum("{\"t\":\"tel\",\"ms\":1")) == ponte::LINHA_RUIM &&
                  tipoDe(comChecksum("{\"t\":\"tel\",\"ms\":1} ")) == ponte::LINHA_RUIM,
              "sem o } no fim e recusado");

    std::string sem_asterisco = tel;
    sem_asterisco[sem_asterisco.size() - 3] = '#';
    verificar(tipoDe(sem_asterisco) == ponte::LINHA_RUIM &&
                  tipoDe(tel.substr(0, tel.size() - 1)) == ponte::LINHA_RUIM &&
                  tipoDe(tel + "0") == ponte::LINHA_RUIM &&
                  tipoDe(tel.substr(0, tel.size() - 3)) == ponte::LINHA_RUIM,
              "sem '*', com um ou tres digitos de checksum, ou sem checksum: recusado");

    verificar(tipoDe(comChecksum("{\"t\":\"tel\",x}")) == ponte::LINHA_RUIM &&
                  tipoDe(comChecksum("{\"t\":\"evt\",\"a\":01}")) == ponte::LINHA_RUIM &&
                  tipoDe(comChecksum("{\"t\":\"evt\",\"a\":{\"b\":1}}")) == ponte::LINHA_RUIM &&
                  tipoDe(comChecksum("{\"t\":\"evt\",}")) == ponte::LINHA_RUIM &&
                  tipoDe(comChecksum("{\"t\":\"evt\",\"a\":1}}")) == ponte::LINHA_RUIM &&
                  tipoDe(comChecksum("{\"t\":\"evt\",\"a\":\"\\x\"}")) == ponte::LINHA_RUIM,
              "JSON quebrado com checksum certo e recusado (nunca entra no /api/estado)");

    verificar(tipoDe(comChecksum("{\"t\":\"evt\",\"s\":\"a\\\"b\\\\c\\u00e9*\","
                                 "\"x\":-1.5e3,\"y\":true,\"z\":null,\"w\":false}")) ==
                  ponte::LINHA_EVENTO,
              "texto com escape, '*' no meio, numero com expoente e literais sao aceitos");

    // Duas trocas de byte com o mesmo bit errado passam pelo XOR. Sem a
    // validacao de JSON, isso entraria no /api/estado.
    std::string bom = "{\"t\":\"evt\",\"ms\":10,\"tipo\":\"inicio\",\"voz\":1}";
    std::string ruim = bom;
    ruim[14] ^= 0x01;  // '"' vira '#'
    ruim[16] ^= 0x01;  // '1' vira '0'
    std::string ruim_linha = ruim + comChecksum(bom).substr(bom.size());
    verificar(comChecksum(ruim) == ruim_linha &&
                  tipoDe(ruim_linha) == ponte::LINHA_RUIM,
              "erro que o checksum nao pega e barrado pela validacao de JSON");
  }

  // ------------------------------------------------------------------
  titulo("4. Ruido na serial (ESP ou Mega reiniciando, fio solto)");
  {
    PonteCadeira p;
    unsigned long t = 100;
    for (int i = 0; i < 200000; ++i) {
      uint32_t r = aleatorio();
      char c = (r % 40 == 0) ? '\n' : (char)(r >> 8);
      p.receberByte(c, t);
    }
    std::string j = estado(p, t, 0);
    verificar(p.linhasOk() == 0 && !p.temTelemetria() && p.seq() == 0,
              "200 mil bytes aleatorios nao viram nenhuma linha valida");
    verificar(p.linhasErro() > 0, "o lixo e contado em linhas_erro");
    verificar(!p.conectado(t), "lixo nao conta como Mega conectado");
    verificar(j.find("\"tel\":null") != std::string::npos &&
                  j.find("\"eventos\":[]") != std::string::npos,
              "estado continua sem telemetria e sem eventos");

    alimentar(p, "\n" + linhaMega(telemetria(1, "PARADA")), t);
    verificar(p.linhasOk() == 1 && p.temTelemetria(),
              "primeira linha integra depois do lixo e aceita");

    // Qualquer troca de um unico byte numa linha boa: ou a linha e recusada,
    // ou o conteudo continua identico (so o hex mudou de caixa).
    std::string original = telemetria(777, "FRENTE");
    std::string linha = comChecksum(original);
    bool so_identico = true;
    for (int i = 0; i < 20000; ++i) {
      std::string m = linha;
      size_t pos = aleatorio() % m.size();
      char novo = (char)(aleatorio() & 0xFF);
      if (novo == m[pos]) continue;
      m[pos] = novo;
      unsigned int tam = 0;
      if (ponte::validarLinhaMega(m.c_str(), (unsigned int)m.size(), &tam) !=
              ponte::LINHA_RUIM &&
          m.substr(0, tam) != original) {
        so_identico = false;
      }
    }
    verificar(so_identico, "nenhum byte trocado passa como dado diferente");
  }

  // ------------------------------------------------------------------
  titulo("5. Telemetria");
  {
    PonteCadeira p;
    alimentar(p, linhaMega(telemetria(1000, "PARADA")), 5000);
    alimentar(p, linhaMega(telemetria(1250, "FRENTE")), 5250);
    std::string j = estado(p, 5300, 0);
    verificar(j.find("\"estado\":\"FRENTE\"") != std::string::npos &&
                  j.find("\"estado\":\"PARADA\"") == std::string::npos,
              "telemetria nova substitui a anterior");
    verificar(j.find("\"idade_ms\":50,") != std::string::npos,
              "idade_ms conta desde a chegada da ultima telemetria");
    verificar(j.find("\"tel\":" + telemetria(1250, "FRENTE") + ",") !=
                  std::string::npos,
              "tel e a linha do Mega sem o *HH, sem nenhuma mudanca");
    verificar(p.linhasOk() == 2 && p.linhasErro() == 0, "contadores de linha");

    alimentar(p, linhaMega(telemetria(1500, "RE")).substr(0, 20) + "\n", 5500);
    j = estado(p, 5500, 0);
    verificar(j.find("\"estado\":\"FRENTE\"") != std::string::npos &&
                  p.linhasErro() == 1,
              "linha cortada nao apaga a telemetria boa e conta como erro");

    std::string longa = "{\"t\":\"tel\",\"x\":\"" + std::string(300, 'a') + "\"}";
    alimentar(p, linhaMega(longa), 5600);
    verificar(p.linhasErro() == 2 && p.linhasOk() == 2,
              "linha maior que o buffer e descartada e contada como erro");
  }

  // ------------------------------------------------------------------
  titulo("6. Fila de eventos");
  {
    PonteCadeira p;
    unsigned long t = 10000;
    for (unsigned long i = 1; i <= 25; ++i) {
      alimentar(p, linhaMega(evento(i)), t + i);
    }
    verificar(p.seq() == 25 && p.eventosGuardados() == ponte::MAX_EVENTOS,
              "25 eventos: seq chega a 25 e so os 20 ultimos ficam");

    std::string j = estado(p, t + 100, 0);
    verificar(contar(j, "\"dados\":") == 20 && primeiroSeq(j) == 6 &&
                  seqsEmOrdem(j, 6, 25),
              "desde=0: todos do buffer, do seq 6 ao 25, do mais antigo ao mais novo");
    verificar(j.find("\"seq\":25,\"eventos\"") != std::string::npos,
              "seq da resposta e o do ultimo evento");
    verificar(j.find("{\"seq\":6,\"idade_ms\":94,\"dados\":" + evento(6) + "}") !=
                  std::string::npos,
              "cada evento traz seq, idade_ms desde a chegada e a linha do Mega");

    j = estado(p, t + 100, 10);
    verificar(contar(j, "\"dados\":") == 15 && seqsEmOrdem(j, 11, 25),
              "desde=10: so os eventos 11 a 25");
    j = estado(p, t + 100, 25);
    verificar(j.find("\"eventos\":[]") != std::string::npos,
              "desde=seq atual: lista vazia");
    j = estado(p, t + 100, 26);
    verificar(contar(j, "\"dados\":") == 20 && seqsEmOrdem(j, 6, 25),
              "desde maior que seq (ESP reiniciou): tratado como 0");
    j = estado(p, t + 100, 3);
    verificar(contar(j, "\"dados\":") == 20 && seqsEmOrdem(j, 6, 25),
              "desde mais antigo que o buffer: todos do buffer");

    alimentar(p, linhaMega(telemetria(1, "PARADA")), t + 200);
    verificar(p.seq() == 25, "telemetria nao entra na fila de eventos");

    std::string maximo =
        "{\"t\":\"evt\",\"x\":\"" +
        std::string(ponte::TAM_EVENTO - 1 - 18, 'm') + "\"}";
    std::string excesso = "{\"t\":\"evt\",\"x\":\"" +
                          std::string(ponte::TAM_EVENTO - 18, 'm') + "\"}";
    unsigned long erros = p.linhasErro();
    alimentar(p, linhaMega(maximo), t + 300);
    alimentar(p, linhaMega(excesso), t + 300);
    verificar(maximo.size() == ponte::TAM_EVENTO - 1 && p.seq() == 26 &&
                  p.linhasErro() == erros + 1,
              "evento de 127 caracteres cabe; com 128 conta como erro");
  }

  // ------------------------------------------------------------------
  titulo("7. Mega conectado ou nao");
  {
    PonteCadeira p;
    verificar(!p.conectado(0) && !p.conectado(123456),
              "sem nenhuma linha: desconectado");
    alimentar(p, linhaMega(telemetria(1, "PARADA")), 1000);
    verificar(p.conectado(1000) && p.conectado(1000 + TEMPO_MEGA_OFFLINE_MS - 1),
              "conectado ate TEMPO_MEGA_OFFLINE_MS depois da ultima linha");
    verificar(!p.conectado(1000 + TEMPO_MEGA_OFFLINE_MS),
              "desconectado ao completar TEMPO_MEGA_OFFLINE_MS");
    alimentar(p, "lixo qualquer\n", 2000);
    verificar(!p.conectado(1000 + TEMPO_MEGA_OFFLINE_MS),
              "linha invalida nao renova a conexao");
    alimentar(p, linhaMega(evento(2)), 2400);
    verificar(p.conectado(2400 + TEMPO_MEGA_OFFLINE_MS - 1),
              "evento valido tambem renova a conexao");

    // millis() da a volta em 49,7 dias no ESP. Aqui a volta e a do unsigned
    // long do PC, que pode ter 64 bits: a conta sem sinal e a mesma.
    PonteCadeira w;
    unsigned long antes = ~0UL - 255;  // 256 ms antes da volta
    alimentar(w, linhaMega(telemetria(1, "PARADA")), antes);
    unsigned long depois = antes + 512;
    verificar(depois < antes && w.conectado(depois),
              "a volta do millis() nao derruba a conexao (512 ms depois)");
    verificar(!w.conectado(antes + TEMPO_MEGA_OFFLINE_MS),
              "e desconecta no tempo certo mesmo depois da volta");
    alimentar(w, linhaMega(evento(1)), antes + 200);
    std::string j = estado(w, antes + 712, 0);
    verificar(j.find("\"idade_ms\":712,") != std::string::npos &&
                  j.find("{\"seq\":1,\"idade_ms\":512,") != std::string::npos,
              "idade_ms certa atravessando a volta do relogio");
  }

  // ------------------------------------------------------------------
  titulo("8. Comandos do app");
  {
    PonteCadeira p;
    unsigned long t = 50000;
    alimentar(p, linhaMega(telemetria(1, "PARADA")), t);
    std::string l;

    verificar(comando(p, "frente", false, "", -1, t, &l) == ponte::CMD_OK &&
                  l == "\nCMD:frente*7E\n",
              "frente sai exatamente como \"\\nCMD:frente*7E\\n\" (ADENDO 2.3)");
    const char* nomes[] = {"frente", "tras",   "esquerda", "direita",
                           "parar",  "rapido", "devagar"};
    bool todos = true;
    for (int i = 0; i < 7; ++i) {
      todos = todos && comando(p, nomes[i], false, NULL, -1, t, &l) == ponte::CMD_OK &&
              l == "\n" + comChecksum(std::string("CMD:") + nomes[i]) + "\n";
    }
    verificar(todos,
              "os 7 nomes do contrato saem com '\\n' antes e o checksum so do "
              "conteudo");
    verificar(comando(p, "parar", false, NULL, -1, t, &l) == ponte::CMD_OK &&
                  l == "\nCMD:parar*00\n",
              "parar sai como \"\\nCMD:parar*00\\n\"");

    verificar(comando(p, "Frente", false, NULL, -1, t, &l) == ponte::CMD_INVALIDO &&
                  comando(p, "", false, NULL, -1, t) == ponte::CMD_INVALIDO &&
                  comando(p, NULL, false, NULL, -1, t) == ponte::CMD_INVALIDO &&
                  comando(p, "frente ", false, NULL, -1, t) == ponte::CMD_INVALIDO &&
                  comando(p, "re", false, NULL, -1, t) == ponte::CMD_INVALIDO &&
                  comando(p, "xyz", false, NULL, -1, t) == ponte::CMD_INVALIDO &&
                  l.empty(),
              "nome fora da lista: CMD_INVALIDO e nenhuma linha");

    verificar(comando(p, "frente", true, NULL, -1, t, &l) == ponte::CMD_OK &&
                  l == "\nMNT:frente*63\n",
              "renovacao de frente sai como \"\\nMNT:frente*63\\n\"");
    verificar(comando(p, "tras", true, NULL, -1, t, &l) == ponte::CMD_OK &&
                  l == "\nMNT:tras*79\n",
              "renovacao de tras sai como \"\\nMNT:tras*79\\n\"");
    todos = true;
    for (int i = 2; i < 7; ++i) {
      todos = todos && comando(p, nomes[i], true, NULL, -1, t, &l) == ponte::CMD_INVALIDO &&
              l.empty();
    }
    verificar(todos, "manter com giro, parar ou velocidade: CMD_INVALIDO");

    unsigned long sem = t + TEMPO_MEGA_OFFLINE_MS;
    verificar(comando(p, "frente", false, NULL, -1, sem, &l) == ponte::CMD_SEM_CONEXAO &&
                  l.empty() &&
                  comando(p, "tras", true, NULL, -1, sem) == ponte::CMD_SEM_CONEXAO &&
                  comando(p, "rapido", false, NULL, -1, sem) == ponte::CMD_SEM_CONEXAO,
              "sem conexao com o Mega: movimento e velocidade recusados");
    verificar(comando(p, "parar", false, NULL, -1, sem, &l) == ponte::CMD_OK &&
                  l == "\nCMD:parar*00\n",
              "sem conexao, parar sai mesmo assim");
    PonteCadeira nunca;
    verificar(comando(nunca, "parar", false, "abc", 1, 0, &l) == ponte::CMD_OK &&
                  comando(nunca, "frente", false, "abc", 2, 0) == ponte::CMD_SEM_CONEXAO,
              "Mega que nunca falou: parar sai, frente nao");
  }

  // ------------------------------------------------------------------
  titulo("9. Ordem dos comandos (requisicao atrasada pela rede)");
  {
    PonteCadeira p;
    unsigned long t = 1000;
    alimentar(p, linhaMega(telemetria(1, "PARADA")), t);
    std::string l;

    verificar(comando(p, "frente", false, "abc", 1, t) == ponte::CMD_OK,
              "primeiro comando de um id passa");
    verificar(comando(p, "frente", false, "abc", 1, t, &l) == ponte::CMD_ATRASADO &&
                  l.empty(),
              "mesmo n de novo: CMD_ATRASADO e nada e enviado");
    verificar(comando(p, "frente", true, "abc", 0, t) == ponte::CMD_ATRASADO,
              "n menor: CMD_ATRASADO");
    verificar(comando(p, "frente", true, "abc", 2, t) == ponte::CMD_OK,
              "n maior: passa");
    verificar(comando(p, "tras", false, "xyz", 1, t) == ponte::CMD_OK,
              "cada id tem a sua contagem");

    verificar(comando(p, "frente", false, "", 5, t) == ponte::CMD_OK &&
                  comando(p, "frente", false, "", 5, t) == ponte::CMD_OK &&
                  comando(p, "frente", false, "", 1, t) == ponte::CMD_OK &&
                  comando(p, "frente", false, NULL, 1, t) == ponte::CMD_OK,
              "sem id: sem verificacao de ordem (cliente simples)");
    verificar(comando(p, "frente", false, "abc", -1, t) == ponte::CMD_OK &&
                  comando(p, "frente", false, "abc", -1, t) == ponte::CMD_OK,
              "n negativo: sem verificacao de ordem");

    // Comando recusado nao pode avancar a contagem.
    unsigned long sem = t + TEMPO_MEGA_OFFLINE_MS;
    verificar(comando(p, "frente", false, "r1", 10, sem) == ponte::CMD_SEM_CONEXAO,
              "(r1 n=10 sem conexao)");
    alimentar(p, linhaMega(telemetria(2, "PARADA")), sem);
    verificar(comando(p, "frente", false, "r1", 5, sem) == ponte::CMD_OK,
              "n de comando recusado por falta de conexao nao e guardado");
    verificar(comando(p, "voar", false, "r1", 20, sem) == ponte::CMD_INVALIDO &&
                  comando(p, "esquerda", true, "r1", 21, sem) == ponte::CMD_INVALIDO &&
                  comando(p, "frente", false, "r1", 6, sem) == ponte::CMD_OK,
              "n de comando invalido nao e guardado");

    // Parar nunca e bloqueado, nem fora de ordem, e nao faz a contagem voltar.
    verificar(comando(p, "frente", false, "pp", 5, sem) == ponte::CMD_OK &&
                  comando(p, "parar", false, "pp", 3, sem, &l) == ponte::CMD_OK &&
                  l == "\nCMD:parar*00\n",
              "parar atrasado sai mesmo assim");
    verificar(comando(p, "frente", false, "pp", 5, sem) == ponte::CMD_ATRASADO &&
                  comando(p, "frente", false, "pp", 6, sem) == ponte::CMD_OK,
              "parar atrasado nao faz a contagem voltar");
    verificar(comando(p, "parar", false, "pq", 7, sem) == ponte::CMD_OK &&
                  comando(p, "frente", false, "pq", 7, sem) == ponte::CMD_ATRASADO,
              "parar enviado tambem conta na ordem");

    verificar(comando(p, "frente", false, "abc-1", 1, sem) == ponte::CMD_INVALIDO &&
                  comando(p, "frente", false, "abcdefghijklmnopq", 1, sem) ==
                      ponte::CMD_INVALIDO &&
                  comando(p, "frente", false, "abcdefghijklmnop", 1, sem) ==
                      ponte::CMD_OK,
              "id fora de [A-Za-z0-9]{1,16}: CMD_INVALIDO; com 16 caracteres passa");

    // Parar com id fora do formato (outro cliente, por exemplo) sai sem
    // ordenacao, com e sem conexao, e nao guarda nada daquele id.
    PonteCadeira q;
    const char* longo = "abcdefghijklmnopq";  // 17 caracteres
    verificar(comando(q, "parar", false, "cel-1", 5, 0, &l) == ponte::CMD_OK &&
                  l == "\nCMD:parar*00\n" &&
                  comando(q, "parar", false, longo, 5, 0, &l) == ponte::CMD_OK &&
                  l == "\nCMD:parar*00\n",
              "sem conexao, parar com id fora do formato sai mesmo assim");
    alimentar(q, linhaMega(telemetria(1, "FRENTE")), 0);
    verificar(comando(q, "parar", false, "cel-1", 5, 0, &l) == ponte::CMD_OK &&
                  l == "\nCMD:parar*00\n" &&
                  comando(q, "parar", false, "cel-1", 5, 0) == ponte::CMD_OK &&
                  comando(q, "parar", false, longo, 1, 0) == ponte::CMD_OK,
              "com conexao, parar com id fora do formato sai, sem ordenacao");
    verificar(comando(q, "frente", false, "cel-1", 6, 0, &l) == ponte::CMD_INVALIDO &&
                  l.empty() &&
                  comando(q, "frente", false, longo, 6, 0) == ponte::CMD_INVALIDO,
              "os outros comandos com id fora do formato continuam CMD_INVALIDO");
    verificar(comando(q, "frente", false, "abcdefghijklmnop", 1, 0) == ponte::CMD_OK,
              "o id de 17 caracteres do parar nao foi guardado cortado em 16");
  }

  // ------------------------------------------------------------------
  titulo("10. Ate 8 ids lembrados, sai o usado ha mais tempo");
  {
    PonteCadeira p;
    unsigned long t = 1000;
    alimentar(p, linhaMega(telemetria(1, "PARADA")), t);
    char id[16];
    for (int i = 0; i < 8; ++i) {
      snprintf(id, sizeof(id), "id%d", i);
      comando(p, "frente", false, id, 10, t);
    }
    verificar(comando(p, "frente", false, "id0", 10, t) == ponte::CMD_ATRASADO &&
                  comando(p, "frente", false, "id7", 10, t) == ponte::CMD_ATRASADO,
              "os 8 primeiros ids ficam guardados");
    verificar(comando(p, "frente", false, "id8", 1, t) == ponte::CMD_OK,
              "nono id entra");
    verificar(comando(p, "frente", false, "id7", 10, t) == ponte::CMD_ATRASADO &&
                  comando(p, "frente", false, "id1", 10, t) == ponte::CMD_ATRASADO,
              "os outros continuam lembrados");
    verificar(comando(p, "frente", false, "id0", 1, t) == ponte::CMD_OK,
              "o mais antigo (id0) foi esquecido: n=1 volta a passar");
    // id0 voltou e tirou o id1. Agora id2 e usado de novo e o id3 vira o
    // mais antigo.
    verificar(comando(p, "frente", false, "id2", 11, t) == ponte::CMD_OK,
              "(id2 usado de novo)");
    verificar(comando(p, "frente", false, "id9", 1, t) == ponte::CMD_OK &&
                  comando(p, "frente", false, "id2", 11, t) == ponte::CMD_ATRASADO &&
                  comando(p, "frente", false, "id3", 1, t) == ponte::CMD_OK,
              "id usado recentemente fica; sai o que esta parado ha mais tempo");
  }

  // ------------------------------------------------------------------
  titulo("11. Buffer de saida do comando");
  {
    PonteCadeira p;
    alimentar(p, linhaMega(telemetria(1, "PARADA")), 0);
    char buf[40];
    memset(buf, '#', sizeof(buf));
    unsigned int justo = (unsigned int)strlen("\nCMD:frente*7E\n");
    ponte::ResultadoComando r =
        p.prepararComando("frente", false, "cap", 1, 0, buf, justo);
    bool intacto = true;
    for (unsigned int i = justo; i < sizeof(buf); ++i) intacto &= buf[i] == '#';
    verificar(r == ponte::CMD_INVALIDO && buf[0] == '\0' && intacto,
              "sem lugar para o '\\0': CMD_INVALIDO, linha vazia, nada alem de cap");
    memset(buf, '#', sizeof(buf));
    r = p.prepararComando("parar", false, NULL, -1, 0, buf, 0);
    verificar(r == ponte::CMD_INVALIDO && buf[0] == '#',
              "cap 0: nada e escrito");
    r = p.prepararComando("frente", false, "cap", 1, 0, buf, justo + 1);
    verificar(r == ponte::CMD_OK && strcmp(buf, "\nCMD:frente*7E\n") == 0,
              "com lugar para o '\\0' passa, e o n que falhou antes nao contou");
    char maior[ponte::TAM_COMANDO];
    r = p.prepararComando("esquerda", false, NULL, -1, 0, maior, sizeof(maior));
    verificar(r == ponte::CMD_OK && strcmp(maior, "\nCMD:esquerda*70\n") == 0,
              "a maior linha (esquerda) cabe em TAM_COMANDO");
  }

  // ------------------------------------------------------------------
  titulo("12. JSON do /api/estado");
  {
    PonteCadeira p;
    std::string j = estado(p, 1234, 0, 0);
    verificar(j ==
                  "{\"conectado\":false,\"idade_ms\":null,\"tel\":null,\"seq\":0,"
                  "\"eventos\":[],\"esp\":{\"uptime_ms\":1234,\"clientes\":0,"
                  "\"linhas_ok\":0,\"linhas_erro\":0}}",
              "sem nada do Mega: tel e idade_ms null, eventos vazio (texto exato)");

    alimentar(p, linhaMega(telemetria(900, "FRENTE")), 1000);
    alimentar(p, linhaMega(evento(950)), 1100);
    alimentar(p, "{\"t\":\"tel\"}*00\n", 1150);
    alimentar(p, linhaMega("{\"t\":\"evt\",\"ms\":990,\"tipo\":\"parada\","
                           "\"motivo\":\"OBSTACULO\"}"),
              1200);
    j = estado(p, 1300, 0, 2);
    std::string esperado =
        "{\"conectado\":true,\"idade_ms\":300,\"tel\":" + telemetria(900, "FRENTE") +
        ",\"seq\":2,\"eventos\":["
        "{\"seq\":1,\"idade_ms\":200,\"dados\":" + evento(950) + "},"
        "{\"seq\":2,\"idade_ms\":100,\"dados\":{\"t\":\"evt\",\"ms\":990,"
        "\"tipo\":\"parada\",\"motivo\":\"OBSTACULO\"}}],"
        "\"esp\":{\"uptime_ms\":1300,\"clientes\":2,\"linhas_ok\":3,"
        "\"linhas_erro\":1}}";
    verificar(j == esperado, "com telemetria e eventos (texto exato)");

    // Todo tamanho de buffer, do zero ao necessario: nunca escreve alem de
    // cap e, se nao couber, devolve 0 e buffer vazio.
    static char buf[2048];
    bool limites = true;
    unsigned int tam = (unsigned int)esperado.size();
    for (unsigned int cap = 0; cap <= tam + 1; ++cap) {
      memset(buf, '#', sizeof(buf));
      unsigned int n = p.montarEstado(buf, cap, 1300, 0, 2);
      for (unsigned int i = cap; i < cap + 64; ++i) limites &= buf[i] == '#';
      if (cap <= tam) {
        limites &= n == 0 && (cap == 0 ? buf[0] == '#' : buf[0] == '\0');
      } else {
        limites &= n == tam && esperado == buf;
      }
    }
    verificar(limites, "cap de 0 ate o tamanho: 0 e nada alem de cap; tamanho+1: cabe");

    p.iniciar();
    verificar(estado(p, 5, 0, 0) ==
                  "{\"conectado\":false,\"idade_ms\":null,\"tel\":null,\"seq\":0,"
                  "\"eventos\":[],\"esp\":{\"uptime_ms\":5,\"clientes\":0,"
                  "\"linhas_ok\":0,\"linhas_erro\":0}}",
              "iniciar() esquece telemetria, eventos e contadores");

    // Pior caso do ESP (numeros de 32 bits com 10 digitos, telemetria e 20
    // eventos do tamanho maximo) cabe em TAM_ESTADO.
    PonteCadeira g;
    unsigned long quase = 4000000000UL;
    std::string tel_max = "{\"t\":\"tel\",\"x\":\"" +
                          std::string(ponte::TAM_LINHA - 4 - 18, 't') + "\"}";
    std::string evt_max = "{\"t\":\"evt\",\"x\":\"" +
                          std::string(ponte::TAM_EVENTO - 1 - 18, 'e') + "\"}";
    alimentar(g, linhaMega(tel_max), 1);
    for (int i = 0; i < 25; ++i) alimentar(g, linhaMega(evt_max), 1);
    for (int i = 0; i < 1000; ++i) alimentar(g, "x\n", 1);
    j = estado(g, quase, 0, -2147483647 - 1);
    verificar(tel_max.size() == ponte::TAM_LINHA - 4 && g.temTelemetria() &&
                  !j.empty() && contar(j, "\"dados\":") == 20,
              "pior caso (telemetria e 20 eventos maximos) cabe em TAM_ESTADO");
  }

  // ------------------------------------------------------------------
  titulo("13. Parametros da API");
  {
    verificar(ponte::interpretarDesde(NULL) == 0 && ponte::interpretarDesde("") == 0 &&
                  ponte::interpretarDesde("abc") == 0 &&
                  ponte::interpretarDesde("-1") == 0 &&
                  ponte::interpretarDesde(" 5") == 0 &&
                  ponte::interpretarDesde("42") == 42 &&
                  ponte::interpretarDesde("4294967295") == 4294967295UL &&
                  ponte::interpretarDesde("4294967296") == 0,
              "desde: so digitos ate 32 bits, o resto vira 0");
    verificar(ponte::interpretarN(NULL) == -1 && ponte::interpretarN("") == -1 &&
                  ponte::interpretarN("7") == 7 &&
                  ponte::interpretarN("2147483647") == 2147483647L &&
                  ponte::interpretarN("2147483648") == -1 &&
                  ponte::interpretarN("-3") == -1 && ponte::interpretarN("1e3") == -1,
              "n: so digitos ate o maior long de 32 bits, o resto vira -1");
    bool m = true;
    bool ok_vazio = ponte::interpretarManter("", &m) && !m;
    m = true;
    bool ok_zero = ponte::interpretarManter("0", &m) && !m;
    bool ok_um = ponte::interpretarManter("1", &m) && m;
    verificar(ok_vazio && ok_zero && ok_um && ponte::interpretarManter(NULL, &m) &&
                  !ponte::interpretarManter("true", &m) &&
                  !ponte::interpretarManter("2", &m),
              "manter: \"\" e \"0\" = nao, \"1\" = sim, qualquer outro = erro");
  }

  // ------------------------------------------------------------------
  titulo("14. Lixo na Serial do Mega antes do comando (ADENDO 2.3)");
  {
    // Quando o ESP reinicia, o boot da ROM a 74880 bps chega ao Mega (que le
    // a 115200) como bytes estranhos, sem '\n' no fim. O primeiro comando
    // depois disso, mesmo um parar, nao pode se perder.
    PonteCadeira p;
    std::string l;
    comando(p, "parar", false, NULL, -1, 0, &l);
    const char boot[] = {'\x80', '\xF8', '\x1C', '\xE0', '\x9E', '\x00',
                         '\xFC', '\x8C', '\x60', '\x02', '\xF0', '\x6C'};
    const std::string lixos[] = {std::string(boot, sizeof(boot)), "rl$ln",
                                 std::string(300, 'x'), "CMD:fre", ""};
    bool todos = true;
    for (int i = 0; i < 5; ++i) {
      ReceptorMega m;
      m.receber(lixos[i]);
      m.receber(l);
      todos = todos && m.ultima == "CMD:parar*00";
    }
    verificar(todos,
              "lixo binario, imprimivel, longo, comando cortado ou nada antes: "
              "o Mega recebe \"CMD:parar*00\"");

    ReceptorMega sem_adendo;
    sem_adendo.receber(lixos[0]);
    sem_adendo.receber(l.substr(1));
    verificar(sem_adendo.ultima.empty(),
              "(o motivo) sem o '\\n' do comeco, o mesmo lixo engoliria o parar");

    // Ruido qualquer, com semente fixa, antes de cada um dos comandos.
    alimentar(p, linhaMega(telemetria(1, "PARADA")), 0);
    const char* nomes[] = {"frente", "tras",   "esquerda", "direita",
                           "parar",  "rapido", "devagar"};
    todos = true;
    for (int rodada = 0; rodada < 5000; ++rodada) {
      bool manter = aleatorio() % 4 == 0;
      const char* nome =
          manter ? (aleatorio() % 2 ? "frente" : "tras") : nomes[aleatorio() % 7];
      std::string lixo;
      unsigned int tam = aleatorio() % 80;
      for (unsigned int k = 0; k < tam; ++k) lixo += (char)(aleatorio() & 0xFF);
      ReceptorMega m;
      m.receber(lixo);
      if (comando(p, nome, manter, NULL, -1, 0, &l) != ponte::CMD_OK) {
        todos = false;
        break;
      }
      m.receber(l);
      todos = todos &&
              m.ultima == comChecksum(std::string(manter ? "MNT:" : "CMD:") + nome);
    }
    verificar(todos, "5000 rodadas de ruido aleatorio: o comando seguinte sempre chega");
  }

  if (arquivo_json != NULL) fclose(arquivo_json);

  // ------------------------------------------------------------------
  printf("\n----------------------------------------\n");
  printf("Total: %d   Falhas: %d\n", total, falhas);
  if (falhas == 0) printf("Todos os testes passaram.\n");
  return falhas == 0 ? 0 : 1;
}
