/*
 * test_app.cpp - Testes da ligacao app -> ESP8266 -> Mega, executados no PC.
 *
 * Compilar e rodar:  make -C testes
 *
 * Inclui o firmware do ESP8266 (firmware/app_wifi) com stubs do Wi-Fi e do
 * servidor HTTP, e liga a saida serial dele direto no leitor de protocolo e
 * na maquina de estados do Mega. Assim cada toque simulado no app percorre o
 * mesmo caminho que percorre na cadeira de verdade.
 */
#include "stub_arduino/Arduino.h"

#include "../firmware/app_wifi/app_wifi.ino"
#include "../firmware/cadeira_voz/protocolo_app.h"

#include <string>

WiFiStub WiFi;
EEPROMClass EEPROM;

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

// Passa tudo o que o ESP mandou pela serial no leitor do Mega e devolve o
// ultimo comando reconhecido.
static Comando entregarAoMega(LeitorProtocoloApp& leitor) {
  std::string s = Serial.retirarSaida();
  Comando ultimo = CMD_NENHUM;
  for (size_t i = 0; i < s.size(); ++i) {
    Comando c = leitor.alimentar(s[i]);
    if (c != CMD_NENHUM) ultimo = c;
  }
  return ultimo;
}

static Comando lerLinha(LeitorProtocoloApp& leitor, const char* s) {
  Comando ultimo = CMD_NENHUM;
  for (; *s; ++s) {
    Comando c = leitor.alimentar(*s);
    if (c != CMD_NENHUM) ultimo = c;
  }
  return ultimo;
}

static bool contem(const std::string& s, const char* trecho) {
  return s.find(trecho) != std::string::npos;
}

int main() {
  printf("Testes da ligacao entre o app e a cadeira\n");
  Serial.silenciar(true);

  // ------------------------------------------------------------------
  titulo("1. Protocolo serial lido pelo Mega");
  {
    LeitorProtocoloApp l;
    verificar(lerLinha(l, "CMD:F\n") == CMD_FRENTE, "CMD:F vira FRENTE");
    verificar(lerLinha(l, "CMD:T\n") == CMD_RE, "CMD:T vira RE");
    verificar(lerLinha(l, "CMD:E\n") == CMD_ESQUERDA, "CMD:E vira ESQUERDA");
    verificar(lerLinha(l, "CMD:D\n") == CMD_DIREITA, "CMD:D vira DIREITA");
    verificar(lerLinha(l, "CMD:P\n") == CMD_PARAR, "CMD:P vira PARAR");
    verificar(lerLinha(l, "CMD:+\n") == CMD_RAPIDO, "CMD:+ vira RAPIDO");
    verificar(lerLinha(l, "CMD:-\n") == CMD_DEVAGAR, "CMD:- vira DEVAGAR");
    verificar(lerLinha(l, "CMD:F\r\n") == CMD_FRENTE,
              "aceita fim de linha \\r\\n");
  }
  {
    LeitorProtocoloApp l;
    const char* boot =
        "\n ets Jan  8 2013,rst cause:2, boot mode:(3,6)\n"
        "load 0x4010f000, len 3460, room 16\ntail 4\nchksum 0xcc\n"
        "f\nt\ne\nd\n+\n";
    verificar(lerLinha(l, boot) == CMD_NENHUM,
              "texto de boot do ESP8266 e letras soltas sao ignorados");
    verificar(l.linhasInvalidas() > 0, "linhas descartadas sao contadas");
    verificar(lerLinha(l, "CMD:FF\n") == CMD_NENHUM, "CMD:FF e recusado");
    verificar(lerLinha(l, "xCMD:F\n") == CMD_NENHUM, "lixo antes do quadro e recusado");
    verificar(lerLinha(l, "cmd:f\n") == CMD_NENHUM, "minusculas sao recusadas");
    verificar(lerLinha(l, "CMD:F") == CMD_NENHUM,
              "sem fim de linha o comando ainda nao vale");
    verificar(lerLinha(l, "\n") == CMD_FRENTE, "vale quando a linha termina");
    verificar(lerLinha(l, "CMD:FFFFFFFFFFFFFFFFFFFFFFFFFFFFFF\nCMD:P\n") ==
                  CMD_PARAR,
              "linha longa e descartada e a seguinte e lida normalmente");
  }

  // ------------------------------------------------------------------
  titulo("2. Rede Wi-Fi e pagina do app");
  setup();
  {
    verificar(WiFi.nome_ == "Cadeira-TCC", "rede Wi-Fi se chama Cadeira-TCC");
    verificar(WiFi.senha_.size() >= 8, "senha tem o minimo de 8 caracteres do WPA2");
    LeitorProtocoloApp l;
    verificar(entregarAoMega(l) == CMD_PARAR, "ao ligar, o ESP manda PARAR");

    int cod = servidor.requisitar("/");
    verificar(cod == 200 && contem(servidor.corpo(), "<title>Cadeira TCC</title>"),
              "GET / entrega a pagina do app");
    verificar(contem(servidor.tipo(), "text/html"), "pagina e servida como HTML");
    verificar(!contem(servidor.corpo(), "http://") &&
                  !contem(servidor.corpo(), "https://"),
              "pagina nao depende de nada da internet");
    verificar(servidor.requisitar("/nada") == 404, "rota desconhecida da 404");
  }

  // ------------------------------------------------------------------
  titulo("3. Cada botao do app chega ao Mega como o comando certo");
  {
    struct Caso { const char* nome; Comando esperado; };
    const Caso casos[] = {
      {"frente", CMD_FRENTE},  {"re", CMD_RE},        {"esquerda", CMD_ESQUERDA},
      {"direita", CMD_DIREITA}, {"parar", CMD_PARAR}, {"rapido", CMD_RAPIDO},
      {"devagar", CMD_DEVAGAR},
    };
    bool todos = true;
    for (unsigned i = 0; i < sizeof(casos) / sizeof(casos[0]); ++i) {
      LeitorProtocoloApp l;
      int cod = servidor.requisitar("/cmd", "c", casos[i].nome);
      if (cod != 200 || entregarAoMega(l) != casos[i].esperado) todos = false;
    }
    verificar(todos, "os 7 comandos do app viram os 7 comandos do Mega");

    LeitorProtocoloApp l;
    verificar(servidor.requisitar("/cmd", "c", "voar") == 400,
              "comando desconhecido e recusado com 400");
    verificar(entregarAoMega(l) == CMD_NENHUM, "e nada e enviado ao Mega");
  }

  // ------------------------------------------------------------------
  titulo("4. Do toque no app ate a roda girar");
  {
    ControleCadeira c;
    LeitorProtocoloApp l;
    unsigned long t = 0;
    c.iniciar(t);
    servidor.requisitar("/cmd", "c", "frente");
    c.aplicarComando(entregarAoMega(l), t);
    for (int i = 0; i < 500; ++i) c.atualizar(++t, false);
    verificar(c.estado() == EST_FRENTE && c.pwmEsquerdo() > 0,
              "toque em Frente faz a cadeira andar");

    // Com obstaculo, a trava do Mega vale para o app tambem.
    c.atualizar(++t, true);
    servidor.requisitar("/cmd", "c", "frente");
    bool aceito = c.aplicarComando(entregarAoMega(l), t);
    verificar(!aceito && !c.emMovimento(),
              "com obstaculo, Frente vinda do app e recusada");

    servidor.requisitar("/cmd", "c", "re");
    aceito = c.aplicarComando(entregarAoMega(l), t);
    verificar(aceito && c.estado() == EST_RE,
              "com obstaculo, Re vinda do app continua liberada");
  }

  // ------------------------------------------------------------------
  titulo("5. Trava do app: sem repeticao, o ESP manda PARAR");
  {
    LeitorProtocoloApp l;
    relogio_simulado = 100000;
    servidor.requisitar("/cmd", "c", "frente");
    entregarAoMega(l);
    unsigned long inicio = relogio_simulado;

    // Usuario segurando o botao: repeticao a cada 300 ms.
    bool parouCedo = false;
    for (int i = 0; i < 10; ++i) {
      relogio_simulado += 300;
      loop();
      if (entregarAoMega(l) == CMD_PARAR) parouCedo = true;
      servidor.requisitar("/cmd", "c", "frente");
      entregarAoMega(l);
    }
    verificar(!parouCedo, "enquanto o botao e segurado, a cadeira nao para");

    // Usuario soltou sem o PARAR chegar (perdeu o Wi-Fi, bloqueou a tela).
    inicio = relogio_simulado;
    relogio_simulado = inicio + LIMITE_APP_MS - 50;
    loop();
    verificar(entregarAoMega(l) == CMD_NENHUM, "antes do limite nada acontece");
    relogio_simulado = inicio + LIMITE_APP_MS + 10;
    loop();
    verificar(entregarAoMega(l) == CMD_PARAR,
              "passado o limite sem repeticao, o ESP manda PARAR sozinho");
    relogio_simulado += 5000;
    loop();
    verificar(entregarAoMega(l) == CMD_NENHUM, "e manda PARAR uma vez so");

    servidor.requisitar("/cmd", "c", "rapido");
    entregarAoMega(l);
    relogio_simulado += 5000;
    loop();
    verificar(entregarAoMega(l) == CMD_NENHUM,
              "Rapido e Devagar nao precisam de repeticao");
  }

  // ------------------------------------------------------------------
  titulo("6. Telemetria do Mega chega ao app");
  {
    servidor.requisitar("/estado");
    verificar(contem(servidor.corpo(), "\"mega\":0") &&
                  contem(servidor.corpo(), "\"cadeira\":null"),
              "sem telemetria, o app ve a cadeira desconectada");

    const char* json =
        "{\"estado\":\"FRENTE\",\"pwmEsq\":130,\"pwmDir\":130,\"nivel\":1,"
        "\"obstaculo\":0,\"emergencia\":0,\"voz\":1,\"recusado\":0,"
        "\"motivo\":\"-\"}";
    Serial.injetar("ruido sem chave\n");
    Serial.injetar(json);
    Serial.injetar("\n");
    relogio_simulado += 10;
    loop();
    servidor.requisitar("/estado");
    verificar(contem(servidor.corpo(), "\"mega\":1"), "com telemetria, mega=1");
    verificar(contem(servidor.corpo(), json), "o JSON do Mega e repassado inteiro");
    verificar(contem(servidor.tipo(), "application/json"), "resposta e JSON");

    Serial.injetar("lixo}\n");
    loop();
    servidor.requisitar("/estado");
    verificar(contem(servidor.corpo(), json), "linha que nao e JSON e ignorada");

    relogio_simulado += LIMITE_MEGA_MS + 10;
    loop();
    servidor.requisitar("/estado");
    verificar(contem(servidor.corpo(), "\"mega\":0"),
              "Mega calado por mais de 1,5 s aparece como desconectado");
  }

  // ------------------------------------------------------------------
  titulo("7. Login: nome e data do primeiro acesso");
  {
    servidor.requisitar("/estado");
    verificar(contem(servidor.corpo(), "\"login\":null"),
              "flash nova: ainda sem login");

    verificar(servidor.requisitar("/login", "nome", "Ana") == 400,
              "login sem data e recusado");
    verificar(servidor.requisitar("/login", "t", "1792540800") == 400,
              "login sem nome e recusado");

    servidor.definirArgs("nome", "Ana \"Maria\"\\", "t", "1792540800");
    verificar(servidor.requisitarSemLimpar("/login") == 200, "login valido e aceito");
    verificar(EEPROM.gravacoes > 0, "login e gravado na flash");

    servidor.requisitar("/estado");
    verificar(contem(servidor.corpo(),
                     "\"login\":{\"nome\":\"Ana Maria\",\"desde\":1792540800}"),
              "/estado devolve o nome e a data, sem aspas que quebrem o JSON");

    // Desliga e liga a placa: o login continua la.
    memset(&login, 0, sizeof(login));
    carregarLogin();
    servidor.requisitar("/estado");
    verificar(contem(servidor.corpo(), "\"desde\":1792540800"),
              "login sobrevive a desligar e ligar a placa");

    servidor.definirArgs("nome", "Ana", "t", "1600000000");
    verificar(servidor.requisitarSemLimpar("/login") == 400,
              "data antes de 2024 (relogio do celular errado) e recusada");

    servidor.requisitar("/login", "apagar", "1");
    servidor.requisitar("/estado");
    verificar(contem(servidor.corpo(), "\"login\":null"),
              "apagar o login volta para a tela de login");
  }

  printf("\n----------------------------------------\n");
  printf("Total: %d   Falhas: %d\n", total, falhas);
  if (falhas == 0) printf("Todos os testes passaram.\n");
  return falhas == 0 ? 0 : 1;
}
