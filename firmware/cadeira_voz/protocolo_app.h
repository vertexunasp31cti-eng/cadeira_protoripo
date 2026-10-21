/*
 * protocolo_app.h - Leitura dos comandos que chegam do app pelo ESP8266.
 *
 * O ESP8266 da placa hospeda o app de controle pelo celular e repassa cada
 * toque como uma linha de texto na Serial3:
 *
 *   CMD:F\n   frente        CMD:P\n   parar
 *   CMD:T\n   tras          CMD:+\n   mais rapido
 *   CMD:E\n   esquerda      CMD:-\n   mais devagar
 *   CMD:D\n   direita
 *
 * O comando precisa vir enquadrado, e nao como uma letra solta, porque o
 * ESP8266 despeja texto de inicializacao na serial toda vez que liga. Esse
 * texto tem letras como "e", "t" e "d", e se uma letra solta fosse aceita a
 * cadeira poderia partir sozinha ao ligar o Wi-Fi. Qualquer linha que nao
 * seja exatamente um dos quadros acima e descartada.
 *
 * Igual a controle.h, este arquivo nao depende da biblioteca Arduino e e
 * testado no PC (testes/test_controle.cpp).
 */
#ifndef PROTOCOLO_APP_H
#define PROTOCOLO_APP_H

#include "controle.h"

class LeitorProtocoloApp {
 public:
  LeitorProtocoloApp();

  // Entrega um byte recebido. Retorna o comando quando uma linha valida
  // termina, ou CMD_NENHUM nos demais casos.
  Comando alimentar(char c);

  // Linhas descartadas por estarem fora do formato (diagnostico).
  unsigned int linhasInvalidas() const { return invalidas_; }

 private:
  Comando interpretar() const;

  static const unsigned int TAMANHO_MAXIMO = 16;
  char linha_[TAMANHO_MAXIMO];
  unsigned int tam_;
  bool estourou_;
  unsigned int invalidas_;
};

// Letra usada no quadro "CMD:x" para cada comando, ou 0 se nao houver.
char letraDoComando(Comando c);

#endif  // PROTOCOLO_APP_H
