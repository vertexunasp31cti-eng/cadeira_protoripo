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
 *  A logica de decisao esta em controle.cpp, sem dependencia do Arduino, e e
 *  verificada por testes automatizados no PC (pasta testes/).
 * ===========================================================================
 */

#include "config.h"
#include "controle.h"
#include "motores.h"
#include "sensor_obstaculo.h"
#include "vr3.h"

// Registros gravados no modulo de voz, na mesma ordem do enum Comando.
static const uint8_t REGISTROS_VOZ[CMD_TOTAL] = {0, 1, 2, 3, 4, 5, 6};

Motores motores;
SensorObstaculo sensor;
ControleCadeira controle;
VR3 voz(SERIAL_VOZ);

bool voz_pronta = false;
unsigned long t_aviso_emergencia = 0;
bool em_emergencia = false;

void tratarComando(Comando c, unsigned long agora, const char* origem);
unsigned long t_telemetria = 0;

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
Comando comandoDaTecla(char c) {
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
// Diagnostico de bancada
// ---------------------------------------------------------------------------
bool emergenciaAcionada() {
  return digitalRead(PINO_BOTAO_EMERG) == NIVEL_BOTAO_ACIONADO;
}

// Mostra tudo que pode impedir a cadeira de andar. Roda no fim do setup() e
// quando se digita '?' no Monitor Serial.
void diagnostico() {
  SERIAL_DEBUG.println(F("--- diagnostico ---"));

  SERIAL_DEBUG.print(F("modulo de voz: "));
  SERIAL_DEBUG.println(voz_pronta ? F("pronto") : F("INDISPONIVEL"));

  SERIAL_DEBUG.print(F("botao de emergencia (pino 3): "));
  if (emergenciaAcionada()) {
    SERIAL_DEBUG.println(F("ACIONADO - a cadeira fica travada"));
    SERIAL_DEBUG.println(F("  O pino 3 esta em GND. Se nao ha botao, nada deve"));
    SERIAL_DEBUG.println(F("  estar ligado nele. Botao precisa ser NA (normal-"));
    SERIAL_DEBUG.println(F("  mente aberto), nao NF."));
  } else {
    SERIAL_DEBUG.println(F("livre"));
  }

  SERIAL_DEBUG.print(F("sensor IR (pino 2): "));
  if (digitalRead(PINO_SENSOR_IR) == NIVEL_IR_DETECTADO) {
    SERIAL_DEBUG.println(F("OBSTACULO - 'frente' sera recusado"));
    SERIAL_DEBUG.println(F("  Sem nada na frente? Gire o trimpot do sensor, confira"));
    SERIAL_DEBUG.println(F("  VCC/GND/OUT, ou mude NIVEL_IR_DETECTADO no config.h."));
  } else {
    SERIAL_DEBUG.println(F("livre"));
  }

  SERIAL_DEBUG.println(F("Digite m para testar os motores direto, sem a logica."));
  SERIAL_DEBUG.println(F("-------------------"));
}

// Espera sem travar a resposta ao botao de emergencia. Retorna false se o
// botao foi acionado durante a espera.
bool esperarTesteMotor(unsigned long ms) {
  unsigned long inicio = millis();
  while (millis() - inicio < ms) {
    if (emergenciaAcionada()) return false;
  }
  return true;
}

// Liga cada motor nos dois sentidos, passando por cima da maquina de estados.
// Serve para separar problema de fiacao/alimentacao de problema de programa:
// se aqui a roda nao gira, o defeito esta na ponte H, nos motores ou na
// bateria. Levante as rodas do chao antes de usar.
void testeMotores() {
  if (controle.emMovimento()) {
    SERIAL_DEBUG.println(F("Pare a cadeira antes do teste de motores."));
    return;
  }
  const int PWM_TESTE = 200;
  const char* const nomes[4] = {"esquerdo para frente", "esquerdo para tras",
                                "direito para frente", "direito para tras"};
  const int esq[4] = {PWM_TESTE, -PWM_TESTE, 0, 0};
  const int dir[4] = {0, 0, PWM_TESTE, -PWM_TESTE};

  SERIAL_DEBUG.println(F("Teste de motores (PWM 200, 1 s cada):"));
  for (uint8_t i = 0; i < 4; ++i) {
    SERIAL_DEBUG.print(F("  motor "));
    SERIAL_DEBUG.println(nomes[i]);
    motores.aplicar(esq[i], dir[i]);
    bool ok = esperarTesteMotor(1000);
    motores.desligar();
    if (!ok) {
      SERIAL_DEBUG.println(F("  interrompido pelo botao de emergencia"));
      return;
    }
    esperarTesteMotor(300);
  }
  SERIAL_DEBUG.println(F("Fim do teste. Se alguma roda nao girou, confira:"));
  SERIAL_DEBUG.println(F("  bateria no VMS/12V do L298N, GND da bateria ligado"));
  SERIAL_DEBUG.println(F("  ao GND do Mega, jumpers ENA/ENB retirados, fios"));
  SERIAL_DEBUG.println(F("  ENA=5 IN1=35 IN2=33 ENB=6 IN3=34 IN4=32."));
}

void lerTeclado(unsigned long agora) {
  if (!SERIAL_DEBUG.available()) return;
  char c = (char)SERIAL_DEBUG.read();
  if (c == '?') {
    diagnostico();
  } else if (c == 'm' || c == 'M') {
    testeMotores();
  }
#if MODO_SIMULACAO
  else {
    tratarComando(comandoDaTecla(c), agora, "teclado");
  }
#else
  (void)agora;
#endif
}

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
  SERIAL_DEBUG.println(F("           ? diagnostico   m testa os motores"));
  SERIAL_DEBUG.println(F(" Cada comando de movimento dura 3 s: repita para"));
  SERIAL_DEBUG.println(F(" continuar andando (protecao homem-morto)."));
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

#if USAR_ESP8266
  SERIAL_ESP.print(F("{\"estado\":\""));
  SERIAL_ESP.print(nomeEstado(controle.estado()));
  SERIAL_ESP.print(F("\",\"pwmEsq\":"));
  SERIAL_ESP.print(controle.pwmEsquerdo());
  SERIAL_ESP.print(F(",\"pwmDir\":"));
  SERIAL_ESP.print(controle.pwmDireito());
  SERIAL_ESP.print(F(",\"obstaculo\":"));
  SERIAL_ESP.print(controle.obstaculoDetectado() ? 1 : 0);
  SERIAL_ESP.println('}');
#endif
}

void tratarComando(Comando c, unsigned long agora, const char* origem) {
  if (c == CMD_NENHUM) return;

  bool aceito = controle.aplicarComando(c, agora);

  SERIAL_DEBUG.print(F("> comando "));
  SERIAL_DEBUG.print(nomeComando(c));
  SERIAL_DEBUG.print(F(" ("));
  SERIAL_DEBUG.print(origem);
  SERIAL_DEBUG.println(aceito ? F(") aceito") : F(") RECUSADO"));

  // Recusa tem som diferente para o usuario perceber sem olhar a tela.
  agendarBips(aceito ? 1 : 2);
}

// Mostra o que aconteceu com cada registro ao carregar o reconhecedor. Um
// registro "NAO TREINADO" nunca sera reconhecido, mesmo com o modulo ligado.
void mostrarRegistrosVoz() {
  static const char* const PALAVRAS[CMD_TOTAL] = {
      "frente", "tras", "esquerda", "direita", "parar", "rapido", "devagar"};
  for (uint8_t i = 0; i < CMD_TOTAL; ++i) {
    SERIAL_DEBUG.print(F("  registro "));
    SERIAL_DEBUG.print((int)REGISTROS_VOZ[i]);
    SERIAL_DEBUG.print(F(" ("));
    SERIAL_DEBUG.print(PALAVRAS[i]);
    SERIAL_DEBUG.print(F("): "));
    SERIAL_DEBUG.println(nomeStatusRegistro(voz.statusRegistro(i)));
  }
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
  // A Serial1 e aberta pelo driver do modulo de voz, que procura o baud.
#if USAR_ESP8266
  SERIAL_ESP.begin(BAUD_ESP);
#endif

  unsigned long agora = millis();
  sensor.iniciar(agora);
  controle.iniciar(agora);

  mostrarAjuda();

  SERIAL_DEBUG.println(F("Procurando o modulo de voz..."));
  voz_pronta = voz.iniciar(REGISTROS_VOZ, CMD_TOTAL);
  if (voz.baud() > 0) {
    SERIAL_DEBUG.print(F("Modulo de voz respondeu a "));
    SERIAL_DEBUG.print(voz.baud());
    SERIAL_DEBUG.println(F(" bps."));
    mostrarRegistrosVoz();
  }
  if (voz_pronta) {
    SERIAL_DEBUG.print(F("Modulo de voz pronto: "));
    SERIAL_DEBUG.print((int)voz.registrosCarregados());
    SERIAL_DEBUG.println(F(" de 7 registros carregados."));
    bipDeInicializacao(120);
  } else {
    SERIAL_DEBUG.print(F("ATENCAO - modulo de voz indisponivel: "));
    SERIAL_DEBUG.println(voz.ultimoErro());
    SERIAL_DEBUG.print(F("  bytes recebidos do modulo: "));
    SERIAL_DEBUG.print(voz.bytesRecebidos());
    for (uint8_t i = 0; i < voz.tamanhoAmostra(); ++i) {
      SERIAL_DEBUG.print(' ');
      if (voz.amostraBytes()[i] < 0x10) SERIAL_DEBUG.print('0');
      SERIAL_DEBUG.print((int)voz.amostraBytes()[i], HEX);
    }
    SERIAL_DEBUG.println();
    SERIAL_DEBUG.println(F("  esperado: modulo TX no pino 19, RX no pino 18"));
#if MODO_SIMULACAO
    SERIAL_DEBUG.println(F("Seguindo apenas com os comandos do teclado."));
#endif
    bipDeInicializacao(60);
    delay(60);
    bipDeInicializacao(60);
  }
  diagnostico();
  t_telemetria = millis();
}

void loop() {
  unsigned long agora = millis();

  // 1) Botao fisico de emergencia tem prioridade sobre qualquer comando.
  //    Avisa pela serial a cada 2 s, senao a cadeira parece travada sem
  //    motivo: nessa situacao nenhum comando nem a telemetria sao atendidos.
  //    Tudo que chegar pela voz ou pelo teclado durante a emergencia e
  //    descartado. Sem isso o comando ficaria na fila da serial e a cadeira
  //    partiria sozinha no instante em que o botao fosse solto.
  if (emergenciaAcionada()) {
    controle.pararEmergencia(agora);
    motores.aplicar(0, 0);
    digitalWrite(PINO_LED, HIGH);
    while (SERIAL_DEBUG.available()) SERIAL_DEBUG.read();
    if (voz_pronta) {
      while (voz.lerReconhecimento() >= 0) {
      }
    }
    em_emergencia = true;
    if (agora - t_aviso_emergencia >= 2000) {
      t_aviso_emergencia = agora;
      SERIAL_DEBUG.println(F("EMERGENCIA: pino 3 em GND, comandos ignorados."));
    }
    return;
  }
  if (em_emergencia) {
    em_emergencia = false;
    SERIAL_DEBUG.println(F("Emergencia liberada. A cadeira so anda com um "
                           "comando novo."));
  }

  // 2) Comandos de voz.
  if (voz_pronta) {
    int registro = voz.lerReconhecimento();
    if (registro >= 0 && registro < CMD_TOTAL) {
      tratarComando((Comando)registro, agora, "voz");
    }
  }

  // 3) Comandos digitados (bancada e demonstracao) e diagnostico.
  lerTeclado(agora);

  // 4) Sensor, maquina de estados e saida para os motores.
  bool obstaculo = sensor.atualizar(agora);
  controle.atualizar(agora, obstaculo);
  motores.aplicar(controle.pwmEsquerdo(), controle.pwmDireito());

  atualizarSinalizacao(agora);
  telemetria(agora);
}
