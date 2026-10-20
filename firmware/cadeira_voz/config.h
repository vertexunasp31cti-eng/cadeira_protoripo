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

// IN1 a IN4 ficam nos pinos 7 a 10, na fileira simples de cima, ao lado do 5
// e do 6. No conector duplo do fim da placa (22 a 53) e facil encaixar o fio
// um furo para o lado, e isso aconteceu na montagem. Esses pinos so definem o
// sentido de giro com digitalWrite; quem controla a velocidade e ENA/ENB.
// O digitalWrite nos pinos 7 e 8 desliga apenas o canal PWM deles no Timer4,
// sem afetar o PWM do pino 6 (ENB), que usa outro canal do mesmo timer.

// Se uma roda girar ao contrario do esperado, troque para true OU inverta os
// dois fios do motor no L298N. Nunca as duas coisas: elas se anulam.
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
const uint8_t PINO_BUZZER = 12;  // opcional; o 8 agora e o IN2
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
// O baud padrao do modulo V3 e 9600. O driver (vr3.cpp) tenta 9600 primeiro
// e, se nao houver resposta, testa sozinho 19200, 38400, 4800 e 2400.

// ---------------------------------------------------------------------------
// Depuracao e simulacao
// ---------------------------------------------------------------------------
#define SERIAL_DEBUG Serial
const long BAUD_DEBUG = 115200;

// Com 1, os comandos tambem podem ser digitados no Monitor Serial
// (f=frente, t=tras, e=esquerda, d=direita, p=parar, +=rapido, -=devagar).
// Permite testar o carrinho sem o modulo de voz e demonstrar a banca.
#define MODO_SIMULACAO 1

// Intervalo de envio da telemetria pela serial.
const unsigned long TELEMETRIA_MS = 500;

// ---------------------------------------------------------------------------
// ESP8266 embarcado (opcional)
// ---------------------------------------------------------------------------
// Na placa Mega+ESP8266 o modulo ESP fica na Serial3, habilitado pelas chaves
// DIP. Mantenha 0 enquanto nao usar o Wi-Fi.
#define USAR_ESP8266 0
#define SERIAL_ESP Serial3
const long BAUD_ESP = 115200;

#endif  // CONFIG_H
