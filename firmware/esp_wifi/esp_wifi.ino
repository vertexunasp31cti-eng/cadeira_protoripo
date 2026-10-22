/*
 * ===========================================================================
 *  CADEIRA DE RODAS COM COMANDO DE VOZ - firmware do ESP8266
 *  Trabalho de Conclusao de Curso - Robotica
 * ===========================================================================
 *
 *  O ESP8266 da placa Mega 2560 + ESP8266 cria a rede Wi-Fi da cadeira,
 *  serve o app (app/index.html, embutido em pagina.h) e faz a ponte entre o
 *  celular e o Mega:
 *
 *    celular  <-- Wi-Fi, HTTP -->  ESP8266  <-- Serial 115200 -->  Mega (Serial3)
 *
 *  O ESP nao decide nada sobre movimento. Ele so repassa os comandos ao Mega,
 *  que os trata como um comando de voz, com todas as travas de seguranca. Se
 *  o Wi-Fi cair, o homem-morto de 3 s do Mega para a cadeira sozinho.
 *
 *  Rotas:
 *    GET  /                       o app
 *    GET  /api/estado?desde=N     telemetria e eventos do Mega, em JSON
 *    POST /api/comando            c=<nome> [manter=1] [id=<texto> n=<numero>]
 *    GET  /manifest.webmanifest   para "Adicionar a tela inicial"
 *    GET  /icone.svg
 *    qualquer outra               302 para o app (portal cativo)
 *
 *  A logica fica em ponte.cpp, sem dependencia do Arduino, e e testada no PC
 *  (testes/test_ponte.cpp e testes/compila_esp.cpp). Este arquivo so liga a
 *  ponte ao Wi-Fi, ao servidor web e a serial.
 *
 *  Como gravar: veja o platformio.ini desta pasta (chaves DIP 5, 6 e 7).
 *  Bibliotecas: so as que vem com o core ESP8266 do Arduino.
 * ===========================================================================
 */

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <stdio.h>
#include <string.h>

#include "config_wifi.h"
#include "ponte.h"
#include "pagina.h"

// Bytes do Mega tratados por volta do loop(). A 115200 bps chegam cerca de
// 11 bytes por milissegundo; 256 por volta esvazia o buffer depressa sem
// deixar o servidor web esperando.
const int BYTES_POR_VOLTA = 256;

// Enquanto o ESP envia a pagina (dezenas de KB), o loop() fica parado e as
// linhas do Mega se acumulam aqui. O padrao do core, 256 bytes, enche em
// menos de meio segundo de telemetria.
const size_t BUFFER_RX_MEGA = 1024;

const uint16_t PORTA_DNS = 53;

ESP8266WebServer servidor(80);
DNSServer servidor_dns;
ponte::PonteCadeira cadeira;

// Estatico: o JSON do estado e montado a cada 300 ms para cada celular, e
// pedir 4 KB ao heap toda vez fragmentaria a pouca memoria do ESP.
static char buf_estado[ponte::TAM_ESTADO];

static char url_app[32];  // "http://192.168.4.1/", montado no setup()

// Mantenha theme_color igual ao <meta name="theme-color"> do app. Aqui no
// .ino o texto e literal comum, sem R"(...)": literal bruto de varias linhas
// ja confundiu o gerador de prototipos do Arduino IDE.
static const char MANIFESTO[] PROGMEM =
    "{\"name\":\"Cadeira por Voz\",\"short_name\":\"Cadeira\","
    "\"description\":\"Acompanhe e comande a cadeira de rodas por voz.\","
    "\"lang\":\"pt-BR\",\"start_url\":\"/\",\"scope\":\"/\","
    "\"display\":\"standalone\",\"background_color\":\"#ffffff\","
    "\"theme_color\":\"#1d4ed8\",\"icons\":[{\"src\":\"/icone.svg\","
    "\"sizes\":\"any\",\"type\":\"image/svg+xml\",\"purpose\":\"any\"}]}";

static const char ICONE_SVG[] PROGMEM =
    "<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 512 512'>"
    "<rect width='512' height='512' rx='112' fill='#1d4ed8'/>"
    "<g transform='translate(16 0)'>"
    "<g fill='none' stroke='#fff' stroke-width='34' stroke-linecap='round' "
    "stroke-linejoin='round'>"
    "<path d='M196 168v118h106l42 92'/>"
    "<path d='M196 222h76'/>"
    "<path d='M150 246a104 104 0 1 0 150 136'/>"
    "<path d='M318 104a56 56 0 0 1 0 80M362 72a104 104 0 0 1 0 144' "
    "stroke-width='24'/>"
    "</g>"
    "<circle cx='196' cy='108' r='38' fill='#fff'/>"
    "</g>"
    "</svg>";

// ---------------------------------------------------------------------------
// Respostas da API
// ---------------------------------------------------------------------------
// no-store: o estado muda a cada 250 ms e nao pode vir de cache. O "*"
// permite o app aberto como arquivo no celular (origem file://) consultar o
// ESP.
void cabecalhosApi() {
  servidor.sendHeader("Cache-Control", "no-store");
  servidor.sendHeader("Access-Control-Allow-Origin", "*");
}

void responderJson(int status, const char* corpo) {
  cabecalhosApi();
  servidor.send(status, "application/json", corpo);
}

// ---------------------------------------------------------------------------
// Rotas
// ---------------------------------------------------------------------------
void rotaPagina() {
  // no-cache: depois de gravar um app novo no ESP, o celular confere antes de
  // reaproveitar a pagina guardada.
  servidor.sendHeader("Cache-Control", "no-cache");
  servidor.send_P(200, "text/html; charset=utf-8", PAGINA_HTML);
}

void rotaEstado() {
  unsigned long desde = ponte::interpretarDesde(servidor.arg("desde").c_str());
  unsigned int n = cadeira.montarEstado(buf_estado, sizeof(buf_estado),
                                        millis(), desde,
                                        (int)WiFi.softAPgetStationNum());
  if (n == 0) {
    // Nao deve acontecer: ponte.cpp confere em tempo de compilacao que o pior
    // caso cabe. Mesmo assim o app recebe JSON valido.
    responderJson(500, "{\"ok\":false,\"erro\":\"estado_grande_demais\"}");
    return;
  }
  responderJson(200, buf_estado);
}

void rotaComando() {
  // arg() junta a query string e o corpo form-urlencoded.
  String nome = servidor.arg("c");
  String manter_txt = servidor.arg("manter");
  String id = servidor.arg("id");
  String n_txt = servidor.arg("n");

  bool manter = false;
  if (!ponte::interpretarManter(manter_txt.c_str(), &manter)) {
    responderJson(400, "{\"ok\":false,\"erro\":\"comando_invalido\"}");
    return;
  }

  unsigned long agora = millis();
  char linha[ponte::TAM_COMANDO];
  ponte::ResultadoComando r = cadeira.prepararComando(
      nome.c_str(), manter, id.c_str(), ponte::interpretarN(n_txt.c_str()),
      agora, linha, sizeof(linha));

  switch (r) {
    case ponte::CMD_OK:
      // Primeiro a serial, depois a resposta: o comando nao espera o Wi-Fi.
      Serial.write((const uint8_t*)linha, strlen(linha));
      responderJson(200, cadeira.conectado(agora)
                             ? "{\"ok\":true,\"conectado\":true}"
                             : "{\"ok\":true,\"conectado\":false}");
      break;
    case ponte::CMD_ATRASADO:
      responderJson(409, "{\"ok\":false,\"erro\":\"comando_atrasado\"}");
      break;
    case ponte::CMD_SEM_CONEXAO:
      responderJson(503, "{\"ok\":false,\"erro\":\"sem_conexao\"}");
      break;
    default:
      responderJson(400, "{\"ok\":false,\"erro\":\"comando_invalido\"}");
      break;
  }
}

// Pre-verificacao de CORS. O app nao precisa dela (manda so requisicoes
// simples), mas um navegador que a fizer recebe permissao em vez de um 302,
// que ele trataria como erro.
void rotaPreVerificacao() {
  cabecalhosApi();
  servidor.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  servidor.sendHeader("Access-Control-Allow-Headers", "Content-Type");
  servidor.sendHeader("Access-Control-Allow-Private-Network", "true");
  servidor.send(204, "application/json", "");
}

void rotaManifesto() {
  servidor.send_P(200, "application/manifest+json", MANIFESTO);
}

void rotaIcone() {
  servidor.sendHeader("Cache-Control", "max-age=86400");
  servidor.send_P(200, "image/svg+xml", ICONE_SVG);
}

// Portal cativo: ao entrar na rede, o celular tenta abrir um endereco de
// teste (connectivitycheck, captive.apple.com...). O DNS responde tudo com o
// IP do ESP e esta rota manda para o app, que entao abre sozinho.
void rotaOutra() {
  servidor.sendHeader("Location", url_app, true);
  servidor.send(302, "text/plain", "");
}

// ---------------------------------------------------------------------------
// Serial do Mega
// ---------------------------------------------------------------------------
void lerMega() {
  unsigned long agora = millis();
  for (int i = 0; i < BYTES_POR_VOLTA && Serial.available() > 0; ++i) {
    cadeira.receberByte((char)Serial.read(), agora);
  }
}

// ---------------------------------------------------------------------------
void setup() {
  // A Serial do ESP vai direto para o Mega: nada de mensagem de depuracao
  // nela, nunca. So saem as linhas CMD/MNT.
  Serial.setRxBufferSize(BUFFER_RX_MEGA);  // precisa vir antes do begin()
  Serial.begin(BAUD_MEGA);
  Serial.setDebugOutput(false);  // mensagens do SDK do Wi-Fi tambem nao

  IPAddress ip(IP_ESP[0], IP_ESP[1], IP_ESP[2], IP_ESP[3]);
  IPAddress mascara(255, 255, 255, 0);
  snprintf(url_app, sizeof(url_app), "http://%u.%u.%u.%u/",
           (unsigned)IP_ESP[0], (unsigned)IP_ESP[1], (unsigned)IP_ESP[2],
           (unsigned)IP_ESP[3]);

  // persistent(false): a configuracao nao e gravada na flash a cada partida,
  // o que desgastaria a flash e poderia reaproveitar uma rede antiga.
  WiFi.persistent(false);
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(ip, ip, mascara);
  WiFi.softAP(NOME_REDE, SENHA_REDE, CANAL_WIFI, 0, MAX_CLIENTES);

  // Qualquer nome ("*") resolve para o ESP. A rede nao tem internet, entao
  // nao ha o que perder, e e isso que faz o portal cativo funcionar.
  servidor_dns.setErrorReplyCode(DNSReplyCode::NoError);
  servidor_dns.start(PORTA_DNS, "*", ip);

  // "/" aceita qualquer metodo: mandar um HEAD / para a rota de redirecionar
  // levaria o navegador de "/" para "/" sem fim.
  servidor.on("/", HTTP_ANY, rotaPagina);
  servidor.on("/api/estado", HTTP_GET, rotaEstado);
  servidor.on("/api/comando", HTTP_POST, rotaComando);
  servidor.on("/api/estado", HTTP_OPTIONS, rotaPreVerificacao);
  servidor.on("/api/comando", HTTP_OPTIONS, rotaPreVerificacao);
  servidor.on("/manifest.webmanifest", HTTP_GET, rotaManifesto);
  servidor.on("/icone.svg", HTTP_GET, rotaIcone);
  servidor.onNotFound(rotaOutra);
  servidor.begin();

  cadeira.iniciar();
}

void loop() {
  // Sem delay() em lugar nenhum: cada volta atende o DNS, um cliente web e o
  // que chegou do Mega, e devolve o controle ao Wi-Fi do ESP.
  servidor_dns.processNextRequest();
  servidor.handleClient();
  lerMega();
}
