/*
 * config.h - Mapa de pinos e opcoes de compilacao.
 *
 * Placa: Arduino Mega 2560 + ESP8266 (CH340).
 * Todo pino usado pelo prototipo esta declarado aqui. Nao espalhe numeros de
 * pino pelo resto do codigo.
 */
#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ---------------------------------------------------------------------------
// Ponte H dupla L298N
// ---------------------------------------------------------------------------
// ENA e ENB precisam ser pinos PWM. No Mega evite os pinos 4 e 13: eles usam
// o Timer0, o mesmo do millis(). Os pinos 5 e 6 estao em timers livres.
const uint8_t PINO_ENA = 5;   // PWM - velocidade do motor esquerdo
const uint8_t PINO_IN1 = 7;   // sentido do motor esquerdo
const uint8_t PINO_IN2 = 8;
const uint8_t PINO_ENB = 6;   // PWM - velocidade do motor direito
const uint8_t PINO_IN3 = 9;   // sentido do motor direito
const uint8_t PINO_IN4 = 10;

// IN1 a IN4 so definem o sentido de giro, com nivel alto ou baixo. Quem
// controla a velocidade e ENA/ENB. Com os jumpers de ENA/ENB colocados na
// placa, nada vai nos pinos 5 e 6 e os motores andam sempre na velocidade
// maxima.

// Se uma roda girar ao contrario do esperado, troque para true em vez de
// inverter os fios no L298N.
const bool INVERTER_MOTOR_ESQ = false;
const bool INVERTER_MOTOR_DIR = false;

// ---------------------------------------------------------------------------
// Sensor de obstaculo infravermelho reflexivo
// ---------------------------------------------------------------------------
const uint8_t PINO_SENSOR_IR = 2;

// A maioria dos modulos IR reflexivos leva a saida para nivel BAIXO quando
// detecta objeto. Se o seu faz o contrario, mude para HIGH.
const uint8_t NIVEL_IR_DETECTADO = LOW;

// Tempo que a leitura precisa ficar estavel para ser aceita (anti-ruido).
const unsigned long DEBOUNCE_IR_MS = 30;

// ---------------------------------------------------------------------------
// Sinalizacao e emergencia
// ---------------------------------------------------------------------------
const uint8_t PINO_BUZZER = 12;  // o 8 e o IN2 da ponte H
const uint8_t PINO_LED = 13;          // LED da propria placa
const uint8_t PINO_BOTAO_EMERG = 3;   // botao NA para GND (opcional)

// Com INPUT_PULLUP o pino fica em HIGH quando o botao nao esta instalado,
// entao a ausencia do botao nunca dispara a emergencia.
const uint8_t NIVEL_BOTAO_ACIONADO = LOW;

// ---------------------------------------------------------------------------
// Modulo de reconhecimento de voz V3
// ---------------------------------------------------------------------------
// Ligado na Serial1 do Mega: pino 19 (RX1) e pino 18 (TX1).
// Atencao ao cruzamento: TX do modulo vai no RX1, RX do modulo vai no TX1.
#define SERIAL_VOZ Serial1
const long BAUD_VOZ = 9600;  // padrao de fabrica do modulo V3

// ---------------------------------------------------------------------------
// Depuracao e simulacao
// ---------------------------------------------------------------------------
#define SERIAL_DEBUG Serial
const long BAUD_DEBUG = 115200;

// Com 1, os comandos tambem podem ser digitados no Monitor Serial
// (f=frente, t=tras, e=esquerda, d=direita, p=parar, +=rapido, -=devagar).
// Permite testar o carrinho sem o modulo de voz e demonstrar a banca.
// As opcoes com #ifndef tambem podem ser trocadas na compilacao, com -D.
#ifndef MODO_SIMULACAO
#define MODO_SIMULACAO 1
#endif

// Intervalo de envio da telemetria pela serial.
const unsigned long TELEMETRIA_MS = 500;

// ---------------------------------------------------------------------------
// ESP8266 embarcado: app do celular por Wi-Fi
// ---------------------------------------------------------------------------
// Na placa Mega+ESP8266 o modulo ESP fica na Serial3, ligado pelas chaves DIP
// 1 e 2. O ESP tem firmware proprio (firmware/esp_wifi/): cria a rede Wi-Fi,
// serve o app e repassa as linhas. Toda decisao de movimento continua aqui.
// Sem o ESP ligado nada muda: ruido na Serial3 nunca vira comando.
#ifndef USAR_ESP8266
#define USAR_ESP8266 1
#endif
#define SERIAL_ESP Serial3
const long BAUD_ESP = 115200;

// 1 = o app pode comandar o movimento, passando pelas mesmas travas da voz.
// 0 = o app so acompanha (monitoramento): todo comando dele e recusado, menos
//     "parar", que e sempre aceito porque parar e sempre seguro.
#ifndef PERMITIR_COMANDO_APP
#define PERMITIR_COMANDO_APP 1
#endif

// Intervalo de envio da telemetria para o app.
const unsigned long TELEMETRIA_APP_MS = 250;

// Intervalo de envio das estatisticas de uso (contadores desde que ligou).
const unsigned long ESTATISTICAS_APP_MS = 1000;

// Dois usos no mesmo pino fazem um ligar o outro: com o buzzer no pino do
// IN2, cada bip girava uma roda. A compilacao para se isso voltar a acontecer.
static_assert(PINO_BUZZER != PINO_IN1 && PINO_BUZZER != PINO_IN2 &&
                  PINO_BUZZER != PINO_IN3 && PINO_BUZZER != PINO_IN4 &&
                  PINO_BUZZER != PINO_ENA && PINO_BUZZER != PINO_ENB,
              "buzzer no mesmo pino da ponte H");
static_assert(PINO_SENSOR_IR != PINO_IN1 && PINO_SENSOR_IR != PINO_IN2 &&
                  PINO_SENSOR_IR != PINO_IN3 && PINO_SENSOR_IR != PINO_IN4 &&
                  PINO_BOTAO_EMERG != PINO_IN1 && PINO_BOTAO_EMERG != PINO_IN2 &&
                  PINO_BOTAO_EMERG != PINO_IN3 && PINO_BOTAO_EMERG != PINO_IN4,
              "sensor ou botao no mesmo pino da ponte H");

#endif  // CONFIG_H
