/*
 * compila_firmware.cpp - Verificacao de compilacao do firmware no PC.
 *
 * Inclui o sketch inteiro usando o stub da API do Arduino. Nao executa a
 * cadeira: serve para garantir que o codigo embarcado compila sem erro antes
 * de abrir o Arduino IDE.
 *
 * Tambem confere o caminho dos comandos do app pela Serial3, que so existe
 * dentro do sketch e por isso nao cabe em test_controle.cpp.
 */
#include "stub_arduino/Arduino.h"

#include "../firmware/cadeira_voz/cadeira_voz.ino"

static int falhas = 0;

static void verificar(bool ok, const char* descricao) {
  printf(ok ? "  [ok]    %s\n" : "  [FALHA] %s\n", descricao);
  if (!ok) ++falhas;
}

static void rodar(int voltas) {
  for (int i = 0; i < voltas; ++i) {
    relogio_simulado += 10;
    loop();
  }
}

int main() {
  Serial.silenciar(true);
  Serial3.silenciar(true);
  setup();
  rodar(50);
  printf("Firmware compila e o loop principal executa sem travar.\n");

#if USAR_ESP8266
  printf("\nComandos do app pela Serial3\n");

  // Caminho livre: no stub o pino do sensor comeca em LOW, que e "obstaculo".
  pinos_simulados[PINO_SENSOR_IR] = !NIVEL_IR_DETECTADO;
  rodar(10);

  // Texto de inicializacao do ESP8266 cheio de letras de comando soltas.
  Serial3.injetar("ets Jan  8 2013,rst cause:2, boot mode:(3,6)\nload 0x4010f000\n");
  rodar(20);
  verificar(controle.estado() == EST_PARADA,
            "texto de boot do ESP8266 nao move a cadeira");

  Serial3.injetar("CMD:F\n");
  rodar(5);
  verificar(controle.estado() == EST_FRENTE, "CMD:F vindo do app anda para frente");

  Serial3.retirarSaida();
  rodar(30);
  std::string saida = Serial3.retirarSaida();
  verificar(saida.find("{\"estado\":\"FRENTE\"") != std::string::npos,
            "telemetria em JSON sai pela Serial3");

  Serial3.injetar("CMD:P\n");
  rodar(2);
  verificar(controle.estado() == EST_PARADA && !controle.emMovimento(),
            "CMD:P vindo do app para na hora");

  // Comando tocado no app enquanto o botao de emergencia esta apertado.
  pinos_simulados[PINO_BOTAO_EMERG] = NIVEL_BOTAO_ACIONADO;
  rodar(5);
  Serial3.injetar("CMD:F\n");
  rodar(5);
  pinos_simulados[PINO_BOTAO_EMERG] = !NIVEL_BOTAO_ACIONADO;
  rodar(5);
  verificar(controle.estado() == EST_PARADA,
            "comando do app durante a emergencia e descartado, nao guardado");

  if (falhas) return 1;
#endif
  return 0;
}
