/*
 * compila_esp.cpp - Verificacao do firmware do ESP8266 no PC.
 *
 * Inclui o sketch inteiro usando os substitutos do core ESP8266
 * (stub_esp8266/), roda setup() e loop() e faz requisicoes HTTP simuladas em
 * todas as rotas, conferindo byte a byte o que vai para a Serial do Mega.
 * Nao substitui o ensaio na placa: serve para pegar erro de compilacao, de
 * rota e de protocolo antes de gravar o ESP.
 *
 * Com um caminho de arquivo como argumento, grava cada corpo JSON recebido
 * (um por linha), para conferir com um leitor de JSON de verdade.
 */
#include "stub_esp8266/Arduino.h"

#include "../firmware/esp_wifi/esp_wifi.ino"

#include <string>

typedef ESP8266WebServer::Resposta Resposta;

static int falhas = 0;
static FILE* arquivo_json = NULL;

static void verificar(bool ok, const char* descricao) {
  if (!ok) {
    ++falhas;
    printf("  [FALHA] %s\n", descricao);
  }
}

// Checksum calculado aqui, sem a ponte, para o teste ser independente.
static std::string linhaMega(const std::string& conteudo) {
  unsigned int x = 0;
  for (size_t i = 0; i < conteudo.size(); ++i) x ^= (unsigned char)conteudo[i];
  char hh[4];
  snprintf(hh, sizeof(hh), "%02X", x);
  return conteudo + "*" + hh + "\n";
}

static bool contem(const std::string& s, const char* trecho) {
  return s.find(trecho) != std::string::npos;
}

static void voltas(int n) {
  for (int i = 0; i < n; ++i) loop();
}

static Resposta pedir(HTTPMethod metodo, const char* caminho,
                      const char* corpo = "") {
  Resposta r = servidor.simular(metodo, caminho, corpo);
  verificar(r.envios == 1, "cada requisicao recebe exatamente uma resposta");
  return r;
}

// Toda resposta de /api/* tem os mesmos cabecalhos e corpo JSON.
static Resposta pedirApi(HTTPMethod metodo, const char* caminho,
                         const char* corpo = "") {
  Resposta r = pedir(metodo, caminho, corpo);
  verificar(r.tipo == "application/json", "/api/* responde application/json");
  verificar(r.cabecalho("Cache-Control") == "no-store",
            "/api/* responde Cache-Control: no-store");
  verificar(r.cabecalho("Access-Control-Allow-Origin") == "*",
            "/api/* responde Access-Control-Allow-Origin: *");
  if (arquivo_json != NULL && !r.corpo.empty()) {
    fprintf(arquivo_json, "%s\n", r.corpo.c_str());
  }
  return r;
}

static Resposta comando(const char* corpo) {
  return pedirApi(HTTP_POST, "/api/comando", corpo);
}

static const char* const OK_CONECTADO = "{\"ok\":true,\"conectado\":true}";
static const char* const INVALIDO = "{\"ok\":false,\"erro\":\"comando_invalido\"}";

int main(int argc, char** argv) {
  if (argc > 1) {
    arquivo_json = fopen(argv[1], "w");
    if (arquivo_json == NULL) {
      printf("nao foi possivel abrir %s\n", argv[1]);
      return 2;
    }
  }

  relogio_simulado = 0;
  setup();

  // --- Inicializacao ---------------------------------------------------------
  verificar(Serial.iniciada() && Serial.baud() == 115200,
            "Serial do Mega em 115200");
  verificar(Serial.tamanhoBufferRx() == 1024 && Serial.bufferMudadoAntesDoBegin(),
            "buffer de recepcao de 1024 bytes, ajustado antes do begin()");
  verificar(!Serial.depuracaoLigada(), "mensagens de depuracao do SDK desligadas");
  verificar(!ESP8266WiFiClass::persistente && WiFi.modo == WIFI_AP,
            "Wi-Fi so ponto de acesso, sem gravar configuracao na flash");
  verificar(WiFi.ap_ligado && WiFi.nome_rede == "Cadeira-TCC" &&
                WiFi.senha == "cadeira123" && WiFi.canal == 6 &&
                WiFi.oculta == 0 && WiFi.max_clientes == 4,
            "rede Cadeira-TCC, canal 6, visivel, ate 4 clientes");
  verificar(WiFi.ip == IPAddress(192, 168, 4, 1) &&
                WiFi.gateway == IPAddress(192, 168, 4, 1) &&
                WiFi.mascara == IPAddress(255, 255, 255, 0) &&
                WiFi.configurado_antes_do_ap,
            "IP 192.168.4.1/24 configurado antes de ligar o ponto de acesso");
  verificar(servidor_dns.iniciado && servidor_dns.porta == 53 &&
                servidor_dns.dominio == "*" &&
                servidor_dns.ip == IPAddress(192, 168, 4, 1) &&
                servidor_dns.codigo_erro == DNSReplyCode::NoError,
            "DNS na porta 53 respondendo qualquer nome com o IP do ESP");
  verificar(servidor.iniciado() && servidor.porta() == 80,
            "servidor web na porta 80");

  voltas(10);
  verificar(servidor_dns.consultas_atendidas == 10 &&
                servidor.voltasAtendidas() == 10,
            "loop() atende DNS e servidor web a cada volta");
  verificar(Serial.enviado().empty() && Serial.escritasSemBegin() == 0,
            "nada e escrito na Serial do Mega sem comando do app");

  // --- Pagina, manifesto e icone --------------------------------------------
  Resposta r = pedir(HTTP_GET, "/");
  verificar(r.status == 200 && r.tipo == "text/html; charset=utf-8" &&
                r.corpo == PAGINA_HTML && !r.corpo.empty(),
            "GET / devolve o app inteiro (pagina.h) como text/html utf-8");
  r = pedir(HTTP_HEAD, "/");
  verificar(r.status == 200, "HEAD / nao cai no redirecionamento para si mesmo");

  r = pedir(HTTP_GET, "/manifest.webmanifest");
  verificar(r.status == 200 && r.tipo == "application/manifest+json" &&
                contem(r.corpo, "\"name\":\"Cadeira por Voz\"") &&
                contem(r.corpo, "\"short_name\":\"Cadeira\"") &&
                contem(r.corpo, "\"start_url\":\"/\"") &&
                contem(r.corpo, "\"display\":\"standalone\"") &&
                contem(r.corpo, "\"src\":\"/icone.svg\""),
            "GET /manifest.webmanifest devolve o manifesto do contrato");
  if (arquivo_json != NULL) fprintf(arquivo_json, "%s\n", r.corpo.c_str());

  r = pedir(HTTP_GET, "/icone.svg");
  verificar(r.status == 200 && r.tipo == "image/svg+xml" &&
                r.corpo.compare(0, 4, "<svg") == 0 && contem(r.corpo, "</svg>"),
            "GET /icone.svg devolve o icone em SVG");

  // --- Estado antes de o Mega falar ------------------------------------------
  relogio_simulado = 1000;
  r = pedirApi(HTTP_GET, "/api/estado");
  verificar(r.status == 200 && contem(r.corpo, "\"conectado\":false") &&
                contem(r.corpo, "\"idade_ms\":null,\"tel\":null") &&
                contem(r.corpo, "\"eventos\":[]") &&
                contem(r.corpo, "\"clientes\":0"),
            "/api/estado sem o Mega: conectado false, tel null, sem eventos");

  // --- O Mega comeca a mandar telemetria e eventos ---------------------------
  std::string tel =
      "{\"t\":\"tel\",\"ms\":15230,\"estado\":\"FRENTE\",\"pwmEsq\":130,"
      "\"pwmDir\":130,\"nivel\":1,\"obstaculo\":0,\"emergencia\":0,"
      "\"motivo\":\"NENHUMA\",\"voz\":1,\"app\":1}";
  std::string evt =
      "{\"t\":\"evt\",\"ms\":15100,\"tipo\":\"comando\",\"cmd\":\"FRENTE\","
      "\"origem\":\"voz\",\"aceito\":1}";
  // Lixo antes, como o do boot: a primeira linha integra depois dele vale.
  Serial.injetar("\xF0\x1C~~ ets Jan  8 2013,rst cause:2\r\n");
  Serial.injetar(linhaMega(tel).c_str());
  Serial.injetar(linhaMega(evt).c_str());
  voltas(3);  // mais de BYTES_POR_VOLTA bytes: precisa de mais de uma volta
  WiFi.estacoes = 2;
  relogio_simulado = 1100;
  r = pedirApi(HTTP_GET, "/api/estado");
  verificar(r.status == 200 && contem(r.corpo, "\"conectado\":true") &&
                contem(r.corpo, ("\"idade_ms\":100,\"tel\":" + tel + ",").c_str()),
            "/api/estado com o Mega: conectado true e tel com a telemetria");
  verificar(contem(r.corpo, "\"seq\":1,\"eventos\":[{\"seq\":1,\"idade_ms\":100,") &&
                contem(r.corpo, ("\"dados\":" + evt + "}]").c_str()),
            "o evento recebido aparece na lista");
  verificar(contem(r.corpo, "\"uptime_ms\":1100,\"clientes\":2,\"linhas_ok\":2,"
                            "\"linhas_erro\":1}"),
            "esp: uptime, clientes conectados e contadores de linha");
  r = pedirApi(HTTP_GET, "/api/estado?desde=1");
  verificar(contem(r.corpo, "\"eventos\":[]"), "desde=1: nada novo");
  r = pedirApi(HTTP_GET, "/api/estado?desde=9");
  verificar(contem(r.corpo, "\"eventos\":[{\"seq\":1,"),
            "desde maior que seq (ESP reiniciou): manda tudo de novo");

  // --- Comandos ----------------------------------------------------------------
  Serial.limparEnviado();
  r = comando("c=frente");
  verificar(r.status == 200 && r.corpo == OK_CONECTADO,
            "POST c=frente: 200 {\"ok\":true,\"conectado\":true}");
  verificar(Serial.enviado() == "\nCMD:frente*7E\n",
            "c=frente escreve exatamente \"\\nCMD:frente*7E\\n\" na Serial "
            "(ADENDO 2.3)");

  Serial.limparEnviado();
  r = comando("c=frente&manter=1");
  verificar(r.status == 200 && Serial.enviado() == "\nMNT:frente*63\n",
            "manter=1 c=frente escreve \"\\nMNT:frente*63\\n\"");

  Serial.limparEnviado();
  r = pedirApi(HTTP_POST, "/api/comando?c=tras");
  verificar(r.status == 200 && Serial.enviado() == "\nCMD:tras*64\n",
            "parametros tambem valem na query string");

  Serial.limparEnviado();
  r = comando("c=xyz");
  verificar(r.status == 400 && r.corpo == INVALIDO && Serial.enviado().empty(),
            "c=xyz: 400 comando_invalido e nada na Serial");
  r = comando("");
  verificar(r.status == 400 && Serial.enviado().empty(), "sem c: 400");
  r = comando("c=esquerda&manter=1");
  verificar(r.status == 400 && r.corpo == INVALIDO && Serial.enviado().empty(),
            "manter=1 com esquerda: 400 e nada na Serial");
  r = comando("c=frente&manter=sim");
  verificar(r.status == 400 && Serial.enviado().empty(),
            "manter com valor estranho: 400, nunca vira comando novo");

  r = comando("c=frente&id=ab12&n=1");
  verificar(r.status == 200, "id/n: primeiro comando passa");
  Serial.limparEnviado();
  r = comando("c=frente&manter=1&id=ab12&n=1");
  verificar(r.status == 409 && r.corpo == "{\"ok\":false,\"erro\":\"comando_atrasado\"}" &&
                Serial.enviado().empty(),
            "mesmo n de novo: 409 comando_atrasado e nada na Serial");
  r = comando("c=frente&manter=1&id=ab12&n=2");
  verificar(r.status == 200 && Serial.enviado() == "\nMNT:frente*63\n",
            "n seguinte passa");

  // id fora de [A-Za-z0-9]{1,16} (outro cliente): parar sai mesmo assim.
  Serial.limparEnviado();
  r = comando("c=frente&id=cel-1&n=5");
  verificar(r.status == 400 && r.corpo == INVALIDO && Serial.enviado().empty(),
            "frente com id fora do formato: 400 e nada na Serial");
  r = comando("c=parar&id=cel-1&n=5");
  verificar(r.status == 200 && r.corpo == OK_CONECTADO &&
                Serial.enviado() == "\nCMD:parar*00\n",
            "parar com id fora do formato: 200 e vai para a Serial");

  Serial.limparEnviado();
  r = pedir(HTTP_GET, "/api/comando?c=frente");
  verificar(r.status == 302 && Serial.enviado().empty(),
            "GET em /api/comando nao comanda nada (so POST)");

  r = pedirApi(HTTP_OPTIONS, "/api/comando");
  verificar(r.status == 204, "OPTIONS /api/comando: pre-verificacao de CORS aceita");

  // --- Mega para de responder -------------------------------------------------
  relogio_simulado = 1000 + TEMPO_MEGA_OFFLINE_MS;
  voltas(3);
  Serial.limparEnviado();
  r = comando("c=frente");
  verificar(r.status == 503 && r.corpo == "{\"ok\":false,\"erro\":\"sem_conexao\"}" &&
                Serial.enviado().empty(),
            "1500 ms sem linha do Mega: frente da 503 sem_conexao");
  r = comando("c=parar");
  verificar(r.status == 200 && r.corpo == "{\"ok\":true,\"conectado\":false}" &&
                Serial.enviado() == "\nCMD:parar*00\n",
            "parar sem conexao: 200 conectado false, e vai para a Serial");
  Serial.limparEnviado();
  r = comando("c=parar&id=abcdefghijklmnopq&n=1");
  verificar(r.status == 200 && r.corpo == "{\"ok\":true,\"conectado\":false}" &&
                Serial.enviado() == "\nCMD:parar*00\n",
            "parar sem conexao e com id de 17 caracteres: 200 e vai para a Serial");
  r = pedirApi(HTTP_GET, "/api/estado");
  verificar(contem(r.corpo, "\"conectado\":false,\"idade_ms\":1500,\"tel\":{"),
            "/api/estado mostra desconectado, com a ultima telemetria e a idade");

  // --- Portal cativo -----------------------------------------------------------
  const char* outras[] = {"/generate_204", "/hotspot-detect.html",
                          "/connecttest.txt", "/qualquer/coisa", "/api"};
  bool todas = true;
  for (int i = 0; i < 5; ++i) {
    r = pedir(HTTP_GET, outras[i]);
    todas = todas && r.status == 302 &&
            r.cabecalho("Location") == "http://192.168.4.1/";
  }
  verificar(todas, "qualquer outra rota: 302 para http://192.168.4.1/");

  // --- Leitura da serial sem travar o servidor --------------------------------
  std::string muitas;
  while (muitas.size() < 600) muitas += linhaMega(tel);
  muitas.resize(600);
  Serial.injetar(muitas.c_str());
  voltas(1);
  verificar(Serial.available() == 600 - BYTES_POR_VOLTA,
            "cada volta do loop() le no maximo BYTES_POR_VOLTA bytes");
  voltas(3);
  verificar(Serial.available() == 0, "as voltas seguintes esvaziam o buffer");

  verificar(chamadas_delay == 0, "o firmware do ESP nunca chama delay()");

  if (arquivo_json != NULL) fclose(arquivo_json);
  if (falhas > 0) {
    printf("compila_esp: %d verificacao(oes) falharam.\n", falhas);
    return 1;
  }
  printf("Firmware do ESP8266 compila e as rotas respondem como no contrato.\n");
  return 0;
}
