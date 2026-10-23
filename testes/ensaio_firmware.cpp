/*
 * ensaio_firmware.cpp - Ensaios do sketch inteiro do Mega, no PC.
 *
 * Compilar e rodar:  make -C testes
 *
 * test_controle e test_protocolo testam as pecas. Aqui roda o cadeira_voz.ino
 * de verdade, com o stub da API do Arduino: bytes chegam pela Serial3 como se
 * viessem do ESP8266, o sensor e o botao de emergencia sao pinos simulados, e
 * cada verificacao olha o PWM que sai para a ponte H e as linhas que o Mega
 * manda para o app. E aqui que ficam testadas as travas que so existem no
 * laco principal: o que chega durante o setup() ou durante a emergencia nao
 * pode ser executado depois.
 *
 * O Makefile compila este arquivo tres vezes: com a configuracao padrao, com
 * PERMITIR_COMANDO_APP 0 (app so acompanha) e com USAR_ESP8266 0 (sem app).
 */
#include "stub_arduino/Arduino.h"

#include "../firmware/cadeira_voz/cadeira_voz.ino"

#include <string>
#include <vector>

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

// Avanca o relogio 1 ms por volta do laco, como o Mega real (o laco leva
// poucos ms). Guarda quantos bytes da Serial3 foram lidos na pior volta.
static unsigned long max_lidos_por_volta = 0;
static void rodar(unsigned long ms) {
  for (unsigned long i = 0; i < ms; ++i) {
    ++relogio_simulado;
    unsigned long antes = Serial3.lidos;
    loop();
    if (Serial3.lidos - antes > max_lidos_por_volta) {
      max_lidos_por_volta = Serial3.lidos - antes;
    }
  }
}

#if USAR_ESP8266
// Linha de comando como o ESP manda: '\n' antes, checksum, '\n' no fim.
static std::string linhaEsp(const char* conteudo, bool quebra_antes = true) {
  unsigned int x = 0;
  for (const char* p = conteudo; *p; ++p) x ^= (unsigned char)*p;
  char buf[48];
  snprintf(buf, sizeof(buf), "%s%s*%02X\n", quebra_antes ? "\n" : "",
           conteudo, x);
  return buf;
}

// Linhas completas que o Mega mandou para o ESP desde a ultima chamada.
static size_t lido_ate = 0;
static std::vector<std::string> novas() {
  std::vector<std::string> v;
  const std::string& s = Serial3.saida;
  size_t inicio = lido_ate;
  for (size_t i = lido_ate; i < s.size(); ++i) {
    if (s[i] == '\n') {
      v.push_back(s.substr(inicio, i - inicio));
      inicio = i + 1;
      lido_ate = inicio;
    }
  }
  return v;
}

// Confere uma linha do Mega pelas regras do contrato, sem usar o codigo do
// firmware: so ASCII imprimivel, JSON de telemetria ou evento, "*HH" certo.
static bool linhaValida(const std::string& l) {
  if (l.size() < 5 || l[l.size() - 3] != '*' || l[l.size() - 4] != '}') {
    return false;
  }
  unsigned int x = 0;
  for (size_t i = 0; i + 3 < l.size(); ++i) {
    unsigned char b = (unsigned char)l[i];
    if (b < 0x20 || b > 0x7e) return false;
    x ^= b;
  }
  char hh[3];
  snprintf(hh, sizeof(hh), "%02X", x);
  return l.compare(l.size() - 2, 2, hh) == 0 &&
         (l.compare(0, 11, "{\"t\":\"tel\",") == 0 ||
          l.compare(0, 11, "{\"t\":\"evt\",") == 0 ||
          l.compare(0, 11, "{\"t\":\"est\",") == 0);
}

static bool todasValidas(const std::vector<std::string>& v) {
  for (size_t i = 0; i < v.size(); ++i) {
    if (!linhaValida(v[i])) {
      printf("    linha invalida: %s\n", v[i].c_str());
      return false;
    }
  }
  return true;
}

static int conta(const std::vector<std::string>& v, const char* trecho) {
  int n = 0;
  for (size_t i = 0; i < v.size(); ++i) {
    if (v[i].find(trecho) != std::string::npos) ++n;
  }
  return n;
}

static int indice(const std::vector<std::string>& v, const char* trecho) {
  for (size_t i = 0; i < v.size(); ++i) {
    if (v[i].find(trecho) != std::string::npos) return (int)i;
  }
  return -1;
}

#if USAR_ESP8266 && PERMITIR_COMANDO_APP
// Valor numerico de um campo ("obst", "app"...) numa linha do Mega.
static unsigned long campo(const std::string& linha, const char* nome) {
  std::string chave = std::string("\"") + nome + "\":";
  size_t i = linha.find(chave);
  if (i == std::string::npos) return 999999;
  return strtoul(linha.c_str() + i + chave.size(), NULL, 10);
}
#endif

static bool motoresParados() {
  return pinos_simulados[PINO_ENA] == 0 && pinos_simulados[PINO_ENB] == 0;
}

#if PERMITIR_COMANDO_APP
// Lixo que a Serial3 do Mega le quando o ESP8266 liga: a mensagem de boot
// da ROM sai a 74880 bps e e lida a 115200. Sem nenhum '\n'.
static const unsigned char LIXO_BOOT[] = {
    0x72, 0x6c, 0x00, 0x6c, 0x9c, 0x9f, 0x7c, 0x00, 0x8c, 0x6c, 0xe0, 0x7c,
    0x03, 0x0c, 0x0c, 0x0c, 0x8c, 0x0c, 0x6c, 0xec, 0x0c, 0x62, 0x7c, 0x8f,
    0x82, 0x03, 0xec, 0x12, 0x92, 0x72, 0x92, 0x63, 0x8c, 0x0c, 0x62, 0x8c,
    0xf2, 0x6f, 0x6f, 0x9e, 0x6c, 0x6e, 0x6e, 0x9c, 0xe3, 0xec, 0x0c, 0x62,
    0x1c, 0x70, 0x8c, 0x8f, 0x6c, 0x73, 0x6c, 0x72, 0x6c, 0x70, 0xf2, 0x6f,
    0xe0, 0x10, 0x03, 0x0c, 0x0c, 0x82, 0x0c, 0x6c, 0x0c, 0x8c, 0x9c, 0x0c,
    0x0c, 0x0c, 0x63, 0x0c, 0x6e, 0xe2, 0x7c, 0x02, 0xec, 0x0c, 0x8e, 0x8e,
    0x0c, 0x62, 0x8c, 0xf2, 0x6f, 0x6f, 0xee, 0x00, 0x6c, 0x8c, 0x8f, 0x6c,
    0x60, 0x03, 0x90, 0x12, 0x12, 0x6e, 0x6f, 0x0c, 0x8e, 0x0e, 0x6c, 0x0e,
    0x02, 0x6e, 0x72, 0x8f, 0x93, 0x93, 0x6e, 0x0c, 0x0c, 0x72, 0x73, 0x93,
    0x60, 0x02, 0x70, 0xf2, 0x6e, 0xe0, 0x10, 0x03, 0x0c, 0x0c, 0x72, 0x8c,
    0x9c, 0x9c, 0x9c, 0x1c, 0x80, 0x8c, 0x02, 0x0c, 0x63, 0x0c, 0x6e, 0xe2,
    0x7c, 0x02, 0x0c, 0x8e, 0x0c, 0x62, 0x8c, 0xf2, 0x6e, 0x6f, 0xef, 0x00,
    0x0c, 0x8e, 0x6c, 0x60, 0x02, 0x90, 0x12, 0x13, 0x6e, 0x6e, 0x0c, 0x8f,
    0x0f, 0x6c, 0x0f, 0x03, 0x6e, 0x72, 0x8e, 0x92, 0x92, 0x6f, 0x0c, 0x0c,
    0x72, 0x12, 0x6c, 0x60, 0x02, 0x0f, 0x73, 0x93, 0x92, 0x6e, 0x0c, 0x0c,
    0x73, 0x13, 0x6c, 0x60, 0x02, 0x9c, 0x02, 0x0c, 0x0c, 0x0c, 0x8f, 0x1c,
    0x1c, 0x92, 0x0c, 0x6c, 0x60, 0x03, 0xfc, 0x82, 0x6e, 0x9c, 0x0f, 0x6c};

static std::string lixoBoot() {
  return std::string((const char*)LIXO_BOOT, sizeof(LIXO_BOOT));
}
#endif  // PERMITIR_COMANDO_APP

// Instantes (campo "ms") das telemetrias de uma lista de linhas.
static std::vector<unsigned long> instantesTelemetria(
    const std::vector<std::string>& v) {
  std::vector<unsigned long> t;
  for (size_t i = 0; i < v.size(); ++i) {
    const char prefixo[] = "{\"t\":\"tel\",\"ms\":";
    const size_t n = sizeof(prefixo) - 1;
    if (v[i].compare(0, n, prefixo) == 0) {
      t.push_back(strtoul(v[i].c_str() + n, NULL, 10));
    }
  }
  return t;
}
#endif  // USAR_ESP8266

int main() {
  printf("Ensaios do sketch do Mega: USAR_ESP8266=%d PERMITIR_COMANDO_APP=%d "
         "MODO_SIMULACAO=%d\n",
         USAR_ESP8266, PERMITIR_COMANDO_APP, MODO_SIMULACAO);
  Serial.silenciar(true);
  Serial3.silenciar(true);

  // Caminho livre e botao solto (o pullup do stub deixa o botao em HIGH).
  pinos_simulados[PINO_SENSOR_IR] = !NIVEL_IR_DETECTADO;

#if USAR_ESP8266
  char texto[160];

  // ------------------------------------------------------------------
  titulo("1. Ligar: nada que chega durante o setup() e executado depois");
  {
    // Sem o modulo de voz o setup() leva uns 4 s tentando falar com ele. Um
    // celular que ja estava conectado pode mandar comandos nesse tempo.
    Serial3.injetar(linhaEsp("CMD:frente"), 1500);
    Serial3.injetar(linhaEsp("CMD:esquerda"), 2500);
    setup();
    relogio_automatico = false;
    verificar(relogio_simulado > 2500,
              "o setup() durou mais que a chegada dos dois comandos");
    rodar(1500);
    std::vector<std::string> v = novas();
    verificar(controle.estado() == EST_PARADA && motoresParados() &&
                  conta(v, "\"tipo\":\"comando\"") == 0,
              "CMD:frente e CMD:esquerda do setup nao andam nem geram evento");
    verificar(!v.empty() && v[0].find("\"tipo\":\"inicio\",\"voz\":0}") !=
                                std::string::npos,
              "a primeira linha para o app e o evento inicio (voz 0)");
    std::vector<unsigned long> t = instantesTelemetria(v);
    bool a_cada_250 = t.size() >= 4;
    for (size_t k = 1; k < t.size(); ++k) {
      if (t[k] - t[k - 1] != TELEMETRIA_APP_MS) a_cada_250 = false;
    }
    snprintf(texto, sizeof(texto),
             "%u telemetrias em 1,5 s, uma a cada 250 ms, todas validas",
             (unsigned int)t.size());
    verificar(a_cada_250 && todasValidas(v), texto);
    int i = indice(v, "{\"t\":\"tel\",");
    verificar(i >= 0 &&
                  v[i].find(",\"estado\":\"PARADA\",\"pwmEsq\":0,\"pwmDir\":0,"
                            "\"nivel\":1,\"obstaculo\":0,\"emergencia\":0,"
                            "\"motivo\":\"NENHUMA\",\"voz\":0,\"app\":") !=
                      std::string::npos,
              "telemetria com os campos na ordem do contrato");
  }

#if PERMITIR_COMANDO_APP
  // ------------------------------------------------------------------
  titulo("2. Comando do app anda e o homem-morto para");
  {
    Serial3.injetar(linhaEsp("CMD:frente"));
    rodar(1);
    verificar(controle.estado() == EST_FRENTE, "CMD:frente do app: FRENTE");
    rodar(600);
    verificar(controle.pwmEsquerdo() == cfg::NIVEIS_PWM[0] &&
                  pinos_simulados[PINO_ENA] == cfg::NIVEIS_PWM[0] &&
                  pinos_simulados[PINO_ENB] == cfg::NIVEIS_PWM[0],
              "a rampa chega ao PWM do nivel 1 nos pinos ENA e ENB");
    std::vector<std::string> v = novas();
    verificar(conta(v, "\"cmd\":\"FRENTE\",\"origem\":\"app\",\"aceito\":1") ==
                  1,
              "evento comando com origem app e aceito 1");
    rodar(cfg::TEMPO_LIMITE_COMANDO_MS - 600 - 3);
    bool andando = controle.estado() == EST_FRENTE;
    rodar(5);
    v = novas();
    verificar(andando && controle.estado() == EST_PARADA &&
                  conta(v, "\"tipo\":\"parada\",\"motivo\":\"TEMPO_LIMITE\"") ==
                      1,
              "sem renovacao, para em 3 s e avisa o app (TEMPO_LIMITE)");
  }

  // ------------------------------------------------------------------
  titulo("3. Segure para andar");
  {
    rodar(1000);
    novas();
    Serial3.injetar(linhaEsp("CMD:frente"));
    rodar(1);
    bool sempre = true;
    for (int i = 0; i < 25; ++i) {
      rodar(400);
      if (controle.estado() != EST_FRENTE) sempre = false;
      Serial3.injetar(linhaEsp("MNT:frente"));
    }
    rodar(1);
    std::vector<std::string> v = novas();
    verificar(sempre && controle.estado() == EST_FRENTE,
              "com renovacao a cada 400 ms, anda 10 s sem parar");
    verificar(conta(v, "\"tipo\":\"comando\"") == 1 &&
                  conta(v, "\"tipo\":\"parada\"") == 0,
              "renovacao nao gera evento (so o CMD do comeco)");
    rodar(cfg::TEMPO_LIMITE_COMANDO_MS - 10);
    bool ainda = controle.estado() == EST_FRENTE;
    rodar(15);
    verificar(ainda && controle.estado() == EST_PARADA &&
                  controle.motivoUltimaParada() == PARADA_TEMPO_LIMITE,
              "soltou (renovacoes param de chegar): para 3 s depois da ultima");
    Serial3.injetar(linhaEsp("MNT:frente"));
    rodar(1000);
    verificar(controle.estado() == EST_PARADA && motoresParados(),
              "renovacao depois da parada nao religa a cadeira");
    Serial3.injetar(linhaEsp("MNT:esquerda") + linhaEsp("MNT:parar"));
    rodar(100);
    v = novas();
    verificar(controle.estado() == EST_PARADA &&
                  conta(v, "\"tipo\":\"comando\"") == 0,
              "renovacao de outro comando e ignorada");
  }

  // ------------------------------------------------------------------
  titulo("4. Botao de emergencia");
  {
    Serial3.injetar(linhaEsp("CMD:frente"));
    rodar(500);
    verificar(controle.emMovimento(), "andando antes do botao");
    novas();
    pinos_simulados[PINO_BOTAO_EMERG] = NIVEL_BOTAO_ACIONADO;
    rodar(1);
    verificar(controle.estado() == EST_PARADA && motoresParados() &&
                  controle.motivoUltimaParada() == PARADA_EMERGENCIA,
              "o botao corta os motores na mesma volta do laco");

    // Tudo isto chega com o botao apertado: comandos inteiros do app, uma
    // renovacao, teclas e o comeco de uma linha que so termina depois.
    std::string meia = linhaEsp("CMD:frente");
    Serial3.injetar(linhaEsp("CMD:tras") + linhaEsp("MNT:frente") +
                    linhaEsp("CMD:parar") + meia.substr(0, 7));
#if MODO_SIMULACAO
    Serial.injetar("ff");
#endif
    rodar(300);
    std::vector<std::string> v = novas();
    int ie = indice(v, "\"tipo\":\"emergencia\",\"ativa\":1");
    int ip = indice(v, "\"tipo\":\"parada\",\"motivo\":\"EMERGENCIA\"");
    verificar(ie >= 0 && ip > ie,
              "o app recebe emergencia 1 e depois a parada EMERGENCIA");
    verificar(conta(v, "\"tipo\":\"comando\"") == 0 &&
                  conta(v, "\"emergencia\":1,") >= 1 && todasValidas(v),
              "nenhum comando tratado; a telemetria mostra emergencia 1");
    verificar(Serial3.rx.empty() && Serial3.no_fio.empty() &&
                  Serial.rx.empty(),
              "tudo que chegou foi lido e jogado fora (nada fica guardado)");

    pinos_simulados[PINO_BOTAO_EMERG] = !NIVEL_BOTAO_ACIONADO;
    Serial3.injetar(meia.substr(7));
    rodar(500);
    v = novas();
    verificar(controle.estado() == EST_PARADA && motoresParados() &&
                  conta(v, "\"tipo\":\"comando\"") == 0,
              "ao soltar, nada anda, nem a linha que comecou na emergencia");
    verificar(conta(v, "\"tipo\":\"emergencia\",\"ativa\":0") == 1,
              "o app recebe emergencia 0");
    Serial3.injetar(linhaEsp("CMD:tras"));
    rodar(100);
    verificar(controle.estado() == EST_RE,
              "depois de soltar, um comando novo funciona");
    Serial3.injetar(linhaEsp("CMD:parar"));
    rodar(100);
    novas();
  }

  // ------------------------------------------------------------------
  titulo("5. Sensor de obstaculo");
  {
    Serial3.injetar(linhaEsp("CMD:frente"));
    rodar(300);
    novas();
    pinos_simulados[PINO_SENSOR_IR] = NIVEL_IR_DETECTADO;
    rodar(cfg::RAMPA_INTERVALO_MS + DEBOUNCE_IR_MS + 20);
    std::vector<std::string> v = novas();
    int io = indice(v, "\"tipo\":\"obstaculo\",\"ativo\":1");
    int ip = indice(v, "\"tipo\":\"parada\",\"motivo\":\"OBSTACULO\"");
    verificar(controle.estado() == EST_PARADA && motoresParados() && io >= 0 &&
                  ip > io,
              "para; o app recebe obstaculo 1 e a parada OBSTACULO");
    Serial3.injetar(linhaEsp("CMD:frente"));
    rodar(5);
    v = novas();
    verificar(controle.estado() == EST_PARADA &&
                  conta(v, "\"cmd\":\"FRENTE\",\"origem\":\"app\","
                           "\"aceito\":0") == 1,
              "CMD:frente com obstaculo e recusado (aceito 0)");
    Serial3.injetar(linhaEsp("CMD:tras"));
    rodar(1);
    bool re = true;
    for (int i = 0; i < 15; ++i) {
      rodar(400);
      if (controle.estado() != EST_RE) re = false;
      Serial3.injetar(linhaEsp("MNT:tras"));
    }
    verificar(re, "re segurada pelo app continua liberada com obstaculo");
    pinos_simulados[PINO_SENSOR_IR] = !NIVEL_IR_DETECTADO;
    rodar(DEBOUNCE_IR_MS + 20);
    v = novas();
    verificar(conta(v, "\"tipo\":\"obstaculo\",\"ativo\":0") == 1,
              "o app recebe obstaculo 0 quando o caminho libera");
    Serial3.injetar(linhaEsp("CMD:parar"));
    rodar(10);
    verificar(controle.estado() == EST_PARADA &&
                  controle.motivoUltimaParada() == PARADA_COMANDO,
              "parar do app para a cadeira");
    novas();
  }

  // ------------------------------------------------------------------
  titulo("6. O ESP8266 reinicia com a cadeira andando");
  {
    // Com '\n' antes do comando (como o ESP manda), logo depois do lixo.
    Serial3.injetar(linhaEsp("CMD:frente"));
    rodar(300);
    bool andando = controle.estado() == EST_FRENTE;
    Serial3.injetar(lixoBoot().substr(0, 60));
    rodar(2);
    Serial3.injetar(lixoBoot().substr(60, 60));
    rodar(2);
    Serial3.injetar(linhaEsp("CMD:parar"));
    rodar(50);
    verificar(andando && controle.estado() == EST_PARADA &&
                  controle.motivoUltimaParada() == PARADA_COMANDO,
              "lixo de boot sem '\\n' e logo depois \"\\nCMD:parar\": para");

    // Sem o '\n' antes, depois de um silencio: o leitor se recupera sozinho.
    Serial3.injetar(linhaEsp("CMD:frente"));
    rodar(300);
    andando = controle.estado() == EST_FRENTE;
    Serial3.injetar(lixoBoot().substr(0, 60));
    rodar(SILENCIO_LEITOR_APP_MS + 5);
    Serial3.injetar(linhaEsp("CMD:parar", false));
    rodar(50);
    verificar(andando && controle.estado() == EST_PARADA &&
                  controle.motivoUltimaParada() == PARADA_COMANDO,
              "lixo sem '\\n' e, depois de um silencio, CMD:parar: para");
    novas();
  }

  // ------------------------------------------------------------------
  titulo("6b. Estatisticas de uso, somando os ensaios 1 a 6");
  {
    rodar(ESTATISTICAS_APP_MS + 100);
    std::vector<std::string> v = novas();
    std::string est;
    for (size_t i = 0; i < v.size(); ++i) {
      if (v[i].compare(0, 11, "{\"t\":\"est\",") == 0) est = v[i];
    }
    verificar(!est.empty() && linhaValida(est),
              "o Mega manda a linha de estatisticas pelo menos 1 vez por segundo");
    // Ensaio 4: um botao de emergencia. Ensaio 5: um obstaculo, uma Frente
    // recusada. Paradas: 2 por tempo limite (2 e 3), 1 emergencia (4), 1
    // obstaculo (5) e 4 por comando (4, 5 e duas no 6).
    verificar(campo(est, "emerg") == 1 && campo(est, "pemerg") == 1,
              "conta o botao de emergencia e a parada que ele causou");
    verificar(campo(est, "obst") == 1 && campo(est, "pobst") == 1,
              "conta o obstaculo e a parada por obstaculo");
    verificar(campo(est, "ptlim") == 2 && campo(est, "pcmd") == 4 &&
                  campo(est, "pgiro") == 0,
              "conta as paradas por tempo limite e por comando");
    verificar(campo(est, "app") == 13 && campo(est, "voz") == 0 &&
                  campo(est, "tec") == 0 && campo(est, "rec") == 1,
              "conta os comandos por origem (renovacao nao conta) e a recusa");
    snprintf(texto, sizeof(texto),
             "tempo andando: %lu s de %lu s ligada",
             campo(est, "mov"), campo(est, "lig"));
    verificar(campo(est, "mov") >= 20 && campo(est, "mov") < campo(est, "lig"),
              texto);
  }
#else   // PERMITIR_COMANDO_APP == 0
  // ------------------------------------------------------------------
  titulo("2. App so acompanha (PERMITIR_COMANDO_APP 0)");
  {
    // Um por vez, como o ESP repassa: cada pedido do app e uma linha.
    const char* pedidos[] = {"CMD:frente", "CMD:tras", "CMD:esquerda",
                             "CMD:rapido"};
    for (int i = 0; i < 4; ++i) {
      Serial3.injetar(linhaEsp(pedidos[i]));
      rodar(5);
    }
    std::vector<std::string> v = novas();
    verificar(controle.estado() == EST_PARADA && motoresParados() &&
                  controle.nivelVelocidade() == cfg::NIVEL_INICIAL &&
                  conta(v, "\"origem\":\"app\",\"aceito\":0") == 4,
              "frente, tras, esquerda e rapido do app: recusados, nada muda");
    verificar(bips_restantes == 2 || bip_ligado,
              "recusa toca dois bips, como a recusa por voz");
    verificar(conta(v, "\"app\":0}") >= 1,
              "a telemetria avisa o app (app 0)");
#if MODO_SIMULACAO
    Serial.injetar("f");
    rodar(1);
    bool andou = controle.estado() == EST_FRENTE;
    for (int i = 0; i < 10; ++i) {
      Serial3.injetar(linhaEsp("MNT:frente"));
      rodar(400);
    }
    verificar(andou && controle.estado() == EST_PARADA &&
                  controle.motivoUltimaParada() == PARADA_TEMPO_LIMITE,
              "renovacao do app e ignorada: o homem-morto para em 3 s");
    Serial.injetar("t");
    rodar(100);
    Serial3.injetar(linhaEsp("CMD:parar"));
    rodar(10);
    v = novas();
    verificar(controle.estado() == EST_PARADA &&
                  controle.motivoUltimaParada() == PARADA_COMANDO &&
                  conta(v, "\"cmd\":\"PARAR\",\"origem\":\"app\","
                           "\"aceito\":1") == 1,
              "parar do app e sempre aceito e para a cadeira");
#endif
  }
#endif  // PERMITIR_COMANDO_APP

  // ------------------------------------------------------------------
  titulo("7. Ruido na serial");
  {
    novas();
    uint32_t s = 12345;
    std::string ruido;
    for (int i = 0; i < 20000; ++i) {
      s ^= s << 13;
      s ^= s >> 17;
      s ^= s << 5;
      ruido += (char)(s >> 24);
    }
    Estado antes = controle.estado();
    max_lidos_por_volta = 0;
    // Chega aos poucos, como a 115200 bps: uns 11 bytes por ms.
    for (size_t i = 0; i < ruido.size(); i += 11) {
      Serial3.injetar(ruido.substr(i, 11), relogio_simulado + 1 + i / 11);
    }
    rodar(ruido.size() / 11 + 100);
    std::vector<std::string> v = novas();
    snprintf(texto, sizeof(texto),
             "20000 bytes de ruido: nenhum comando, no maximo %lu bytes lidos "
             "por volta (limite 64)",
             max_lidos_por_volta);
    verificar(conta(v, "\"tipo\":\"comando\"") == 0 &&
                  controle.estado() == antes && max_lidos_por_volta <= 64,
              texto);
    Serial3.injetar(linhaEsp("CMD:parar"));
    rodar(5);
    v = novas();
    verificar(conta(v, "\"cmd\":\"PARAR\"") == 1,
              "depois do ruido, um comando valido funciona");
  }

  // ------------------------------------------------------------------
  titulo("8. Serial de transmissao cheia nao trava o laco");
  {
    rodar(1000);
    novas();
    size_t antes = Serial3.saida.size();
    Serial3.livre_tx = 0;
    rodar(2000);
    bool nada_saiu = Serial3.saida.size() == antes;
    Serial3.livre_tx = 3;
    size_t max_por_volta = 0;
    for (int i = 0; i < 3000; ++i) {
      size_t a = Serial3.saida.size();
      rodar(1);
      if (Serial3.saida.size() - a > max_por_volta) {
        max_por_volta = Serial3.saida.size() - a;
      }
    }
    Serial3.livre_tx = 63;
    rodar(500);
    std::vector<std::string> v = novas();
    verificar(nada_saiu, "sem espaco no buffer, nada e escrito");
    verificar(max_por_volta <= 3 && todasValidas(v) &&
                  conta(v, "{\"t\":\"tel\",") >= 2,
              "com 3 bytes livres sai no maximo 3 por volta, e as linhas "
              "chegam inteiras");
  }

#if MODO_SIMULACAO
  // ------------------------------------------------------------------
  titulo("9. Teclado tambem aparece no historico do app");
  {
    Serial.injetar("+");
    rodar(10);
    std::vector<std::string> v = novas();
    verificar(conta(v, "\"cmd\":\"RAPIDO\",\"origem\":\"teclado\"") == 1,
              "comando do teclado gera evento com origem teclado");
  }
#endif

#else   // USAR_ESP8266 == 0
  titulo("1. Sem o ESP8266, a Serial3 fica em silencio");
  {
    setup();
    relogio_automatico = false;
    rodar(2000);
    verificar(Serial3.saida.empty(), "nenhum byte sai para o ESP");
#if MODO_SIMULACAO
    Serial.injetar("f");
    rodar(10);
    verificar(controle.estado() == EST_FRENTE,
              "o teclado continua comandando a cadeira");
#endif
  }
#endif  // USAR_ESP8266

  printf("\n----------------------------------------\n");
  printf("Total: %d   Falhas: %d\n", total, falhas);
  if (falhas == 0) printf("Todos os ensaios passaram.\n");
  return falhas == 0 ? 0 : 1;
}
