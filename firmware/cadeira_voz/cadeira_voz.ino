/*
 * ===========================================================================
 *  PROTOTIPO DE CADEIRA DE RODAS COM COMANDO DE VOZ
 *  Trabalho de Conclusao de Curso - Robotica
 * ===========================================================================
 *
 *  Hardware:
 *    - Placa compativel Arduino Mega 2560 + ESP8266 (CH340)
 *    - Modulo de Reconhecimento de Voz V3 (Elechouse)
 *    - Ponte H dupla L298N
 *    - Sensor de obstaculo reflexivo infravermelho
 *    - 2x motor DC 3-6V com roda de 68 mm
 *
 *  Comandos de voz (numero do registro gravado no modulo):
 *    0 "frente"    1 "tras"      2 "esquerda"   3 "direita"
 *    4 "parar"     5 "rapido"    6 "devagar"
 *
 *  Camadas de seguranca implementadas:
 *    1. Sensor IR para a cadeira e recusa novos comandos de avanco.
 *    2. Homem-morto: sem comando novo em 3 s, a cadeira para sozinha.
 *    3. "Parar" corta o PWM no mesmo instante, sem rampa.
 *    4. Botao fisico de emergencia (opcional) em paralelo ao comando de voz.
 *    5. Rampa de aceleracao e passagem obrigatoria por zero na inversao.
 *    6. Giro sempre como pulso curto e temporizado.
 *
 *  Controle pelo celular (opcional): o ESP8266 da placa cria a rede Wi-Fi
 *  "Cadeira-TCC" e serve o app em http://192.168.4.1. Os comandos do app
 *  chegam pela Serial3 e passam pelas mesmas travas da voz.
 *
 *  A logica de decisao esta em controle.cpp, sem dependencia do Arduino, e e
 *  verificada por testes automatizados no PC (pasta testes/).
 * ===========================================================================
 */

#include "config.h"
#include "controle.h"
#include "motores.h"
#include "protocolo_app.h"
#include "sensor_obstaculo.h"
#include "vr3.h"

// Registros gravados no modulo de voz, na mesma ordem do enum Comando.
static const uint8_t REGISTROS_VOZ[CMD_TOTAL] = {0, 1, 2, 3, 4, 5, 6};

Motores motores;
SensorObstaculo sensor;
ControleCadeira controle;
VR3 voz(SERIAL_VOZ);

bool voz_pronta = false;
unsigned long t_telemetria = 0;

#if USAR_ESP8266
LeitorProtocoloApp leitor_app;
unsigned long t_telemetria_app = 0;

// O app repete o comando de movimento enquanto o botao esta apertado. A
// repeticao so renova o homem-morto: nao apita nem enche o Monitor Serial.
const unsigned long JANELA_REPETICAO_APP_MS = 1000;
Comando ultimo_cmd_app = CMD_NENHUM;
unsigned long t_ultimo_cmd_app = 0;
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
// Comandos vindos do app do celular, pelo ESP8266 na Serial3
// ---------------------------------------------------------------------------
#if USAR_ESP8266
Comando lerComandoApp() {
  // Le tudo o que chegou, mas devolve no maximo um comando por volta do laco,
  // como fazem a voz e o teclado.
  while (SERIAL_ESP.available()) {
    Comando c = leitor_app.alimentar((char)SERIAL_ESP.read());
    if (c != CMD_NENHUM) return c;
  }
  return CMD_NENHUM;
}

// Joga fora o que o app mandou enquanto o botao de emergencia estava
// apertado. Sem isso, um "frente" tocado durante a emergencia ficaria
// guardado na serial e seria executado assim que o botao fosse solto.
void descartarComandosApp() {
  while (SERIAL_ESP.available()) leitor_app.alimentar((char)SERIAL_ESP.read());
}

// Uma linha JSON por envio. O ESP8266 guarda a ultima e entrega ao app.
void telemetriaApp(unsigned long agora, bool emergencia) {
  if (agora - t_telemetria_app < TELEMETRIA_APP_MS) return;
  t_telemetria_app = agora;

  SERIAL_ESP.print(F("{\"estado\":\""));
  SERIAL_ESP.print(nomeEstado(controle.estado()));
  SERIAL_ESP.print(F("\",\"pwmEsq\":"));
  SERIAL_ESP.print(controle.pwmEsquerdo());
  SERIAL_ESP.print(F(",\"pwmDir\":"));
  SERIAL_ESP.print(controle.pwmDireito());
  SERIAL_ESP.print(F(",\"nivel\":"));
  SERIAL_ESP.print(controle.nivelVelocidade() + 1);
  SERIAL_ESP.print(F(",\"obstaculo\":"));
  SERIAL_ESP.print(controle.obstaculoDetectado() ? 1 : 0);
  SERIAL_ESP.print(F(",\"emergencia\":"));
  SERIAL_ESP.print(emergencia ? 1 : 0);
  SERIAL_ESP.print(F(",\"voz\":"));
  SERIAL_ESP.print(voz_pronta ? 1 : 0);
  SERIAL_ESP.print(F(",\"recusado\":"));
  SERIAL_ESP.print(controle.ultimoComandoRecusado() ? 1 : 0);
  SERIAL_ESP.print(F(",\"motivo\":\""));
  SERIAL_ESP.print(nomeMotivo(controle.motivoUltimaParada()));
  SERIAL_ESP.println(F("\"}"));
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
  SERIAL_DEBUG.println(F(" App:      Wi-Fi \"Cadeira-TCC\", abra 192.168.4.1"));
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

void tratarComando(Comando c, unsigned long agora, const char* origem,
                   bool avisar) {
  if (c == CMD_NENHUM) return;

  bool aceito = controle.aplicarComando(c, agora);

  // Comando repetido e aceito passa em silencio. Recusa sempre avisa.
  if (!avisar && aceito) return;

  SERIAL_DEBUG.print(F("> comando "));
  SERIAL_DEBUG.print(nomeComando(c));
  SERIAL_DEBUG.print(F(" ("));
  SERIAL_DEBUG.print(origem);
  SERIAL_DEBUG.println(aceito ? F(") aceito") : F(") RECUSADO"));

  // Recusa tem som diferente para o usuario perceber sem olhar a tela.
  agendarBips(aceito ? 1 : 2);
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
  // Descarta o texto que o ESP8266 despeja ao ligar, que chegou durante o
  // setup e nao e comando.
  while (SERIAL_ESP.available()) SERIAL_ESP.read();
#endif
}

void loop() {
  unsigned long agora = millis();

  // 1) Botao fisico de emergencia tem prioridade sobre qualquer comando.
  if (digitalRead(PINO_BOTAO_EMERG) == NIVEL_BOTAO_ACIONADO) {
    controle.pararEmergencia(agora);
    motores.aplicar(0, 0);
    digitalWrite(PINO_LED, HIGH);
#if USAR_ESP8266
    // O app continua sabendo o que acontece, mas nenhum comando dele vale
    // enquanto o botao estiver apertado.
    descartarComandosApp();
    telemetriaApp(agora, true);
#endif
    return;
  }

  // 2) Comandos de voz.
  if (voz_pronta) {
    int registro = voz.lerReconhecimento();
    if (registro >= 0 && registro < CMD_TOTAL) {
      tratarComando((Comando)registro, agora, "voz", true);
    }
  }

  // 3) Comandos digitados (bancada e demonstracao).
#if MODO_SIMULACAO
  tratarComando(lerComandoDigitado(), agora, "teclado", true);
#endif

  // 4) Comandos do app do celular, pelo ESP8266. Passam pela mesma maquina de
  //    estados, entao valem as mesmas travas de seguranca da voz.
#if USAR_ESP8266
  Comando cmd_app = lerComandoApp();
  if (cmd_app != CMD_NENHUM) {
    bool repetido = cmd_app == ultimo_cmd_app &&
                    (agora - t_ultimo_cmd_app) < JANELA_REPETICAO_APP_MS &&
                    controle.estado() != EST_PARADA;
    ultimo_cmd_app = cmd_app;
    t_ultimo_cmd_app = agora;
    tratarComando(cmd_app, agora, "app", !repetido);
  }
#endif

  // 5) Sensor, maquina de estados e saida para os motores.
  bool obstaculo = sensor.atualizar(agora);
  controle.atualizar(agora, obstaculo);
  motores.aplicar(controle.pwmEsquerdo(), controle.pwmDireito());

  atualizarSinalizacao(agora);
  telemetria(agora);
#if USAR_ESP8266
  telemetriaApp(agora, false);
#endif
}
