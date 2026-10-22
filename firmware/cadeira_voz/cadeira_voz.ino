/*
 * ===========================================================================
 *  PROTOTIPO DE CADEIRA DE RODAS COM COMANDO DE VOZ
 *  Trabalho de Conclusao de Curso - Robotica
 * ===========================================================================
 *
 *  Hardware:
 *    - Placa compativel Arduino Mega 2560 + ESP8266 (CH340)
 *      (o ESP8266 da placa serve o app do celular por Wi-Fi, na Serial3)
 *    - Modulo de Reconhecimento de Voz V3 (Elechouse)
 *    - Ponte H dupla L298N
 *    - Sensor de obstaculo reflexivo infravermelho
 *    - 2x motor DC 3-6V com roda de 68 mm
 *
 *  Comandos de voz (numero do registro gravado no modulo):
 *    0 "frente"    1 "tras"      2 "esquerda"   3 "direita"
 *    4 "parar"     5 "rapido"    6 "devagar"
 *  Os mesmos comandos tambem chegam pelo teclado (MODO_SIMULACAO) e pelo app
 *  (USAR_ESP8266), sempre pela mesma maquina de estados.
 *
 *  Camadas de seguranca implementadas:
 *    1. Sensor IR para a cadeira e recusa novos comandos de avanco.
 *    2. Homem-morto: sem comando novo em 3 s, a cadeira para sozinha.
 *    3. "Parar" corta o PWM no mesmo instante, sem rampa.
 *    4. Botao fisico de emergencia (opcional) em paralelo ao comando de voz.
 *    5. Rampa de aceleracao e passagem obrigatoria por zero na inversao.
 *    6. Giro sempre como pulso curto e temporizado.
 *    7. O app pelo Wi-Fi so pede comandos: todas as travas acima valem para
 *       ele, e perder a conexao nunca deixa a cadeira andando (o homem-morto
 *       a para sozinho).
 *
 *  A logica de decisao esta em controle.cpp, sem dependencia do Arduino, e e
 *  verificada por testes automatizados no PC (pasta testes/).
 * ===========================================================================
 */

#include "config.h"
#include "controle.h"
#include "motores.h"
#include "sensor_obstaculo.h"
#include "vr3.h"
#if USAR_ESP8266
#include "protocolo_app.h"
#endif

// Registros gravados no modulo de voz, na mesma ordem do enum Comando.
static const uint8_t REGISTROS_VOZ[CMD_TOTAL] = {0, 1, 2, 3, 4, 5, 6};

// Limite de bytes lidos de uma serial por volta do laco. Uma rajada de bytes
// nunca segura o laco: o que sobrar fica para a proxima volta.
const uint8_t BYTES_POR_VOLTA = 64;

Motores motores;
SensorObstaculo sensor;
ControleCadeira controle;
VR3 voz(SERIAL_VOZ);

bool voz_pronta = false;
unsigned long t_telemetria = 0;

#if USAR_ESP8266
LeitorLinhas leitor_app;
FilaSaida fila_app;
char linha_app[TAM_LINHA_APP];
unsigned long t_telemetria_app = 0;

// Ultimos valores publicados, para gerar os eventos por borda.
Estado estado_publicado = EST_PARADA;
bool obstaculo_publicado = false;
bool emergencia_publicada = false;
#endif

// ---------------------------------------------------------------------------
// Sinalizacao sonora e visual
//
// O buzzer nunca usa delay() durante a conducao. Um delay de 180 ms para o
// laco inteiro, e nesse intervalo o sensor de obstaculo e o tempo limite de
// seguranca ficam sem ser atendidos. Por isso os bips sao agendados e tocados
// aos poucos, uma fatia por volta do laco.
// ---------------------------------------------------------------------------
const unsigned long BIP_DURACAO_MS = 60;
const unsigned long ALARME_PERIODO_MS = 250;

uint8_t bips_restantes = 0;
bool bip_ligado = false;
unsigned long t_bip = 0;
bool alarme_ligado = false;
unsigned long t_alarme = 0;

void agendarBips(uint8_t quantidade) {
  bips_restantes = quantidade;
  bip_ligado = false;
  t_bip = 0;  // toca ja na proxima volta do laco
}

// Bip bloqueante, usado somente no setup(), com os motores desligados.
void bipDeInicializacao(unsigned int ms) {
  digitalWrite(PINO_BUZZER, HIGH);
  delay(ms);
  digitalWrite(PINO_BUZZER, LOW);
}

void atualizarSinalizacao(unsigned long agora) {
  // Alarme de obstaculo tem prioridade sobre os bips de comando.
  if (controle.obstaculoDetectado()) {
    bips_restantes = 0;
    bip_ligado = false;
    if (agora - t_alarme >= ALARME_PERIODO_MS) {
      t_alarme = agora;
      alarme_ligado = !alarme_ligado;
      digitalWrite(PINO_BUZZER, alarme_ligado ? HIGH : LOW);
      digitalWrite(PINO_LED, alarme_ligado ? HIGH : LOW);
    }
    return;
  }

  if (alarme_ligado) {
    alarme_ligado = false;
    digitalWrite(PINO_BUZZER, LOW);
  }

  if (bips_restantes > 0) {
    if (agora - t_bip >= BIP_DURACAO_MS) {
      t_bip = agora;
      bip_ligado = !bip_ligado;
      digitalWrite(PINO_BUZZER, bip_ligado ? HIGH : LOW);
      if (!bip_ligado) --bips_restantes;  // terminou um bip completo
    }
    return;
  }

  digitalWrite(PINO_BUZZER, LOW);
  digitalWrite(PINO_LED, controle.emMovimento() ? HIGH : LOW);
}

// ---------------------------------------------------------------------------
// Entrada de comandos digitados, para testar sem o modulo de voz
// ---------------------------------------------------------------------------
#if MODO_SIMULACAO
Comando lerComandoDigitado() {
  if (!SERIAL_DEBUG.available()) return CMD_NENHUM;
  char c = (char)SERIAL_DEBUG.read();
  switch (c) {
    case 'f': case 'F': return CMD_FRENTE;
    case 't': case 'T': return CMD_RE;
    case 'e': case 'E': return CMD_ESQUERDA;
    case 'd': case 'D': return CMD_DIREITA;
    case 'p': case 'P': case ' ': return CMD_PARAR;
    case '+': return CMD_RAPIDO;
    case '-': return CMD_DEVAGAR;
    default: return CMD_NENHUM;
  }
}
#endif

// ---------------------------------------------------------------------------
// App do celular, pelo ESP8266 (protocolo em protocolo_app.h)
//
// O ESP so repassa: aqui as linhas viram comandos que passam pela mesma
// maquina de estados da voz. Nada desta parte espera pela serial. A leitura
// tem limite de bytes por volta e a escrita sai por uma fila, so o que cabe
// no buffer de transmissao.
// ---------------------------------------------------------------------------
#if USAR_ESP8266
// Poe na fila a linha que um formatar*() acabou de escrever em linha_app.
// Tamanho 0 quer dizer que nao coube, e entao nada sai.
void enviarAoApp(unsigned int n) {
  if (n > 0) fila_app.colocar(linha_app, n);
}

void escoarFilaApp() {
  int espaco = SERIAL_ESP.availableForWrite();
  while (espaco > 0 && fila_app.pendentes() > 0) {
    SERIAL_ESP.print(fila_app.tirar());
    --espaco;
  }
}

// Eventos por borda (comparando com o ultimo valor publicado, uma vez por
// volta do laco), telemetria periodica e envio do que estiver na fila.
void publicarApp(unsigned long agora, bool emergencia) {
  // A causa sai antes da parada que ela provocou.
  if (emergencia != emergencia_publicada) {
    emergencia_publicada = emergencia;
    enviarAoApp(formatarEventoEmergencia(linha_app, TAM_LINHA_APP, agora,
                                         emergencia));
  }

  bool obstaculo = controle.obstaculoDetectado();
  if (obstaculo != obstaculo_publicado) {
    obstaculo_publicado = obstaculo;
    enviarAoApp(formatarEventoObstaculo(linha_app, TAM_LINHA_APP, agora,
                                        obstaculo));
  }

  Estado estado = controle.estado();
  if (estado == EST_PARADA && estado_publicado != EST_PARADA) {
    enviarAoApp(formatarEventoParada(linha_app, TAM_LINHA_APP, agora,
                                     controle.motivoUltimaParada()));
  }
  estado_publicado = estado;

  // A telemetria se repete a cada 250 ms, entao ela espera a fila esvaziar.
  // Assim nunca ocupa o lugar de um evento, que acontece uma vez so.
  if (agora - t_telemetria_app >= TELEMETRIA_APP_MS &&
      fila_app.pendentes() == 0) {
    t_telemetria_app = agora;
    DadosTelemetria d;
    d.ms = agora;
    d.estado = estado;
    d.pwm_esq = controle.pwmEsquerdo();
    d.pwm_dir = controle.pwmDireito();
    d.nivel = controle.nivelVelocidade() + 1;
    d.obstaculo = obstaculo;
    d.emergencia = emergencia;
    d.motivo = controle.motivoUltimaParada();
    d.voz = voz_pronta;
    d.app = PERMITIR_COMANDO_APP != 0;
    enviarAoApp(formatarTelemetria(linha_app, TAM_LINHA_APP, d));
  }

  escoarFilaApp();
}

void iniciarApp(unsigned long agora) {
  t_telemetria_app = agora;
  estado_publicado = controle.estado();
  obstaculo_publicado = controle.obstaculoDetectado();
  emergencia_publicada = false;
  enviarAoApp(formatarEventoInicio(linha_app, TAM_LINHA_APP, agora,
                                   voz_pronta));
}
#endif

// ---------------------------------------------------------------------------
void mostrarAjuda() {
  SERIAL_DEBUG.println(F("==================================================="));
  SERIAL_DEBUG.println(F(" Cadeira de rodas com comando de voz - prototipo"));
  SERIAL_DEBUG.println(F("==================================================="));
  SERIAL_DEBUG.println(F(" Palavras: frente | tras | esquerda | direita |"));
  SERIAL_DEBUG.println(F("           parar | rapido | devagar"));
#if MODO_SIMULACAO
  SERIAL_DEBUG.println(F(" Teclado:  f t e d p (espaco=parar) + -"));
#endif
#if USAR_ESP8266
  SERIAL_DEBUG.println(PERMITIR_COMANDO_APP
                           ? F(" App:      pelo ESP8266, pode comandar")
                           : F(" App:      pelo ESP8266, so acompanha"));
#endif
  SERIAL_DEBUG.println(F("---------------------------------------------------"));
}

void telemetria(unsigned long agora) {
  if (agora - t_telemetria < TELEMETRIA_MS) return;
  t_telemetria = agora;

  SERIAL_DEBUG.print(F("estado="));
  SERIAL_DEBUG.print(nomeEstado(controle.estado()));
  SERIAL_DEBUG.print(F("  pwm="));
  SERIAL_DEBUG.print(controle.pwmEsquerdo());
  SERIAL_DEBUG.print('/');
  SERIAL_DEBUG.print(controle.pwmDireito());
  SERIAL_DEBUG.print(F("  nivel="));
  SERIAL_DEBUG.print(controle.nivelVelocidade() + 1);
  SERIAL_DEBUG.print(F("  obstaculo="));
  SERIAL_DEBUG.print(controle.obstaculoDetectado() ? F("SIM") : F("nao"));
  SERIAL_DEBUG.print(F("  ultima parada: "));
  SERIAL_DEBUG.println(nomeMotivo(controle.motivoUltimaParada()));
}

// Depuracao e bips de um comando ja decidido.
void anunciarComando(Comando c, const char* origem, bool aceito) {
  SERIAL_DEBUG.print(F("> comando "));
  SERIAL_DEBUG.print(nomeComando(c));
  SERIAL_DEBUG.print(F(" ("));
  SERIAL_DEBUG.print(origem);
  SERIAL_DEBUG.println(aceito ? F(") aceito") : F(") RECUSADO"));

  // Recusa tem som diferente para o usuario perceber sem olhar a tela.
  agendarBips(aceito ? 1 : 2);
}

void tratarComando(Comando c, unsigned long agora, const char* origem) {
  if (c == CMD_NENHUM) return;

  bool aceito = controle.aplicarComando(c, agora);
  anunciarComando(c, origem, aceito);
#if USAR_ESP8266
  enviarAoApp(formatarEventoComando(linha_app, TAM_LINHA_APP, agora, c, origem,
                                    aceito));
#endif
}

#if USAR_ESP8266
// As travas do app (parar sempre aceito, PERMITIR_COMANDO_APP, emergencia)
// ficam em decidirAcaoApp(), testada no PC. Aqui so se executa a decisao.
void aplicarLinhaApp(const LinhaApp& l, unsigned long agora, bool emergencia) {
  switch (decidirAcaoApp(l, PERMITIR_COMANDO_APP != 0, emergencia)) {
    case ACAO_TRATAR:
      tratarComando(l.cmd, agora, "app");
      break;
    case ACAO_RECUSAR:
      anunciarComando(l.cmd, "app", false);
      SERIAL_DEBUG.println(F("  app so acompanha (PERMITIR_COMANDO_APP 0)"));
      enviarAoApp(formatarEventoComando(linha_app, TAM_LINHA_APP, agora, l.cmd,
                                        "app", false));
      break;
    case ACAO_RENOVAR:
      // Sem bip e sem evento: chega a cada 400 ms enquanto o botao e segurado.
      controle.renovarComando(l.cmd, agora);
      break;
    case ACAO_IGNORAR:
      break;
  }
}

// O leitor recebe o tempo para voltar a sincronizar sozinho depois de um
// silencio (lixo do boot do ESP sem '\n' no fim).
void lerComandosApp(unsigned long agora, bool emergencia) {
  for (uint8_t i = 0; i < BYTES_POR_VOLTA; ++i) {
    int b = SERIAL_ESP.read();
    if (b < 0) break;
    if (leitor_app.receber((char)b, agora)) {
      aplicarLinhaApp(interpretarLinhaApp(leitor_app.linha()), agora,
                      emergencia);
    }
  }
}
#endif

// Com o botao de emergencia apertado, tudo que chega e lido e jogado fora. Se
// ficasse guardado no buffer da serial, seria executado no instante em que o
// botao fosse solto.
void descartarComandos(unsigned long agora) {
  if (voz_pronta) voz.lerReconhecimento();

#if MODO_SIMULACAO
  for (uint8_t i = 0; i < BYTES_POR_VOLTA && SERIAL_DEBUG.available(); ++i) {
    SERIAL_DEBUG.read();
  }
#endif

#if USAR_ESP8266
  // Os bytes passam pelo leitor para ele nao perder o compasso das linhas;
  // com emergencia, decidirAcaoApp() ignora toda linha que fica completa.
  lerComandosApp(agora, true);
  leitor_app.descartarLinhaAtual();
#else
  (void)agora;
#endif
}

// ---------------------------------------------------------------------------
void setup() {
  pinMode(PINO_BUZZER, OUTPUT);
  pinMode(PINO_LED, OUTPUT);
  pinMode(PINO_BOTAO_EMERG, INPUT_PULLUP);
  digitalWrite(PINO_BUZZER, LOW);

  // Os motores sao a primeira coisa a ser desligada, antes de qualquer outra
  // inicializacao, para a cadeira nunca partir sozinha ao ligar.
  motores.iniciar();

  SERIAL_DEBUG.begin(BAUD_DEBUG);
  SERIAL_VOZ.begin(BAUD_VOZ);
#if USAR_ESP8266
  SERIAL_ESP.begin(BAUD_ESP);
#endif

  unsigned long agora = millis();
  sensor.iniciar(agora);
  controle.iniciar(agora);

  mostrarAjuda();

  voz_pronta = voz.iniciar(REGISTROS_VOZ, CMD_TOTAL);
  if (voz_pronta) {
    SERIAL_DEBUG.println(F("Modulo de voz pronto: 7 registros carregados."));
    bipDeInicializacao(120);
  } else {
    SERIAL_DEBUG.print(F("ATENCAO - modulo de voz indisponivel: "));
    SERIAL_DEBUG.println(voz.ultimoErro());
    SERIAL_DEBUG.print(F("  bytes recebidos do modulo: "));
    SERIAL_DEBUG.println(voz.bytesRecebidos());
    SERIAL_DEBUG.println(F("  esperado: modulo TX no pino 19, RX no pino 18"));
#if MODO_SIMULACAO
    SERIAL_DEBUG.println(F("Seguindo apenas com os comandos do teclado."));
#endif
    bipDeInicializacao(60);
    delay(60);
    bipDeInicializacao(60);
  }
  t_telemetria = millis();
#if USAR_ESP8266
  // O que o app mandou durante o setup (a voz sozinha pode levar ~4 s) foi
  // pedido antes de a cadeira estar pronta e nao pode rodar atrasado na
  // primeira volta do laco: e jogado fora, como o VR3::iniciar() faz com a
  // voz. O buffer de recepcao tem 63 bytes, entao isso nao segura o setup.
  for (uint8_t i = 0; i < BYTES_POR_VOLTA && SERIAL_ESP.available(); ++i) {
    SERIAL_ESP.read();
  }
  leitor_app.reiniciar();
  iniciarApp(t_telemetria);
#endif
}

void loop() {
  unsigned long agora = millis();

  // 1) Botao fisico de emergencia tem prioridade sobre qualquer comando.
  if (digitalRead(PINO_BOTAO_EMERG) == NIVEL_BOTAO_ACIONADO) {
    controle.pararEmergencia(agora);
    motores.aplicar(0, 0);
    digitalWrite(PINO_LED, HIGH);
    descartarComandos(agora);
    telemetria(agora);
#if USAR_ESP8266
    publicarApp(agora, true);  // o app precisa ver a emergencia
#endif
    return;
  }

  // 2) Comandos de voz.
  if (voz_pronta) {
    int registro = voz.lerReconhecimento();
    if (registro >= 0 && registro < CMD_TOTAL) {
      tratarComando((Comando)registro, agora, "voz");
    }
  }

  // 3) Comandos digitados (bancada e demonstracao).
#if MODO_SIMULACAO
  tratarComando(lerComandoDigitado(), agora, "teclado");
#endif

  // 4) Comandos do app no celular, repassados pelo ESP8266.
#if USAR_ESP8266
  lerComandosApp(agora, false);
#endif

  // 5) Sensor, maquina de estados e saida para os motores.
  bool obstaculo = sensor.atualizar(agora);
  controle.atualizar(agora, obstaculo);
  motores.aplicar(controle.pwmEsquerdo(), controle.pwmDireito());

  atualizarSinalizacao(agora);
  telemetria(agora);
#if USAR_ESP8266
  publicarApp(agora, false);
#endif
}
