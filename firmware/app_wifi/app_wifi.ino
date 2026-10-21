/*
 * ===========================================================================
 *  APP DE CONTROLE PELO CELULAR - firmware do ESP8266 da placa
 * ===========================================================================
 *
 *  Este sketch roda no ESP8266 que vem soldado na placa Mega 2560 + ESP8266,
 *  e nao no Mega. Ele faz a ponte entre o celular e a cadeira:
 *
 *     celular  --Wi-Fi-->  ESP8266  --serial-->  Mega 2560  -->  motores
 *     celular  <--Wi-Fi--  ESP8266  <--serial--  Mega 2560  (telemetria)
 *
 *  1. Cria a rede Wi-Fi "Cadeira-TCC" (senha "cadeira123"). Nao precisa de
 *     roteador nem de internet.
 *  2. Serve o app em http://192.168.4.1 (pagina.h). Funciona no navegador de
 *     qualquer celular, sem instalar nada.
 *  3. Cada toque no app vira uma linha "CMD:x" enviada ao Mega.
 *  4. A telemetria em JSON que o Mega publica e devolvida ao app.
 *
 *  Quem decide se a cadeira anda continua sendo o Mega: os comandos do app
 *  passam pela mesma maquina de estados e pelas mesmas travas da voz. O ESP
 *  so acrescenta mais uma trava, a do app (veja LIMITE_APP_MS).
 *
 *  Gravacao: chaves DIP 5, 6 e 7 ligadas, placa "Generic ESP8266 Module" ou,
 *  no PlatformIO, o ambiente esp8266. Passo a passo em docs/app_celular.md.
 *  Nenhuma biblioteca externa e necessaria: ESP8266WiFi e ESP8266WebServer
 *  vem com o pacote da placa.
 * ===========================================================================
 */

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

#include "pagina.h"

// ---------------------------------------------------------------------------
// Rede Wi-Fi criada pela cadeira
// ---------------------------------------------------------------------------
const char* NOME_REDE = "Cadeira-TCC";
const char* SENHA_REDE = "cadeira123";  // minimo de 8 caracteres

// ---------------------------------------------------------------------------
// Ligacao com o Mega
// ---------------------------------------------------------------------------
// A Serial do ESP chega na Serial3 do Mega pelas chaves DIP 1 e 2.
// Mesma velocidade de BAUD_ESP em firmware/cadeira_voz/config.h.
const long BAUD_MEGA = 115200;

// Sem telemetria do Mega por esse tempo, o app mostra a cadeira desconectada.
const unsigned long LIMITE_MEGA_MS = 1500;

// ---------------------------------------------------------------------------
// Trava do app
// ---------------------------------------------------------------------------
// Enquanto o usuario segura um botao de movimento, o app repete o comando a
// cada 300 ms. Se a repeticao parar de chegar (celular bloqueou a tela, saiu
// do alcance do Wi-Fi, travou), o ESP manda PARAR sozinho. Isso e mais rapido
// que o homem-morto de 3 s do Mega, que continua valendo por baixo.
const unsigned long LIMITE_APP_MS = 1000;

ESP8266WebServer servidor(80);

char linha_mega[200];
unsigned int tam_linha = 0;
char telemetria[200] = "";
unsigned long t_telemetria = 0;
bool recebeu_telemetria = false;

bool app_movendo = false;
unsigned long t_ultimo_app = 0;

// ---------------------------------------------------------------------------
// Comandos
// ---------------------------------------------------------------------------
struct ComandoApp {
  const char* nome;  // como o app pede: /cmd?c=frente
  char letra;        // como o Mega recebe: CMD:F
  bool movimento;    // se precisa de repeticao para continuar valendo
};

// Nomes por extenso porque "+" numa URL vira espaco.
const ComandoApp COMANDOS[] = {
  {"frente", 'F', true},   {"re", 'T', true},      {"esquerda", 'E', true},
  {"direita", 'D', true},  {"parar", 'P', false},  {"rapido", '+', false},
  {"devagar", '-', false},
};
const int TOTAL_COMANDOS = sizeof(COMANDOS) / sizeof(COMANDOS[0]);

void enviarAoMega(char letra) {
  Serial.print("CMD:");
  Serial.print(letra);
  Serial.print('\n');
}

bool megaConectado(unsigned long agora) {
  return recebeu_telemetria && (agora - t_telemetria) < LIMITE_MEGA_MS;
}

// ---------------------------------------------------------------------------
// Rotas HTTP
// ---------------------------------------------------------------------------
void rotaPagina() {
  servidor.sendHeader("Cache-Control", "no-store");
  servidor.send_P(200, "text/html; charset=utf-8", PAGINA_APP);
}

void rotaComando() {
  String nome = servidor.arg("c");
  for (int i = 0; i < TOTAL_COMANDOS; ++i) {
    if (nome == COMANDOS[i].nome) {
      enviarAoMega(COMANDOS[i].letra);
      app_movendo = COMANDOS[i].movimento;
      t_ultimo_app = millis();
      servidor.send(200, "text/plain", "ok");
      return;
    }
  }
  servidor.send(400, "text/plain", "comando desconhecido");
}

// Junta o estado da ligacao com a ultima telemetria do Mega.
void rotaEstado() {
  unsigned long agora = millis();
  String json = "{\"mega\":";
  json += megaConectado(agora) ? "1" : "0";
  json += ",\"clientes\":";
  json += String(WiFi.softAPgetStationNum());
  json += ",\"cadeira\":";
  json += recebeu_telemetria ? telemetria : "null";
  json += "}";
  servidor.sendHeader("Cache-Control", "no-store");
  servidor.send(200, "application/json", json);
}

void rotaNaoEncontrada() {
  servidor.send(404, "text/plain", "nao encontrado");
}

// ---------------------------------------------------------------------------
// Telemetria vinda do Mega
// ---------------------------------------------------------------------------
void lerMega(unsigned long agora) {
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\r') continue;
    if (c != '\n') {
      if (tam_linha < sizeof(linha_mega) - 1) linha_mega[tam_linha++] = c;
      continue;
    }
    linha_mega[tam_linha] = '\0';
    // So aceita linha que tem cara de objeto JSON inteiro. O resto e ruido.
    if (tam_linha >= 2 && linha_mega[0] == '{' &&
        linha_mega[tam_linha - 1] == '}') {
      memcpy(telemetria, linha_mega, tam_linha + 1);
      t_telemetria = agora;
      recebeu_telemetria = true;
    }
    tam_linha = 0;
  }
}

// ---------------------------------------------------------------------------
void setup() {
  Serial.begin(BAUD_MEGA);

  WiFi.mode(WIFI_AP);
  WiFi.softAP(NOME_REDE, SENHA_REDE);

  servidor.on("/", rotaPagina);
  servidor.on("/cmd", rotaComando);
  servidor.on("/estado", rotaEstado);
  servidor.onNotFound(rotaNaoEncontrada);
  servidor.begin();

  // Garante que a cadeira comeca parada do ponto de vista do app.
  enviarAoMega('P');
}

void loop() {
  unsigned long agora = millis();

  servidor.handleClient();
  lerMega(agora);

  // Trava do app: movimento sem repeticao recente vira PARAR.
  if (app_movendo && (agora - t_ultimo_app) >= LIMITE_APP_MS) {
    enviarAoMega('P');
    app_movendo = false;
  }
}
