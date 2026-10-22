/*
 * config_wifi.h - Rede Wi-Fi e ligacao com o Mega, no ESP8266.
 *
 * Tudo que se ajusta no ESP fica aqui. O arquivo nao depende da biblioteca
 * Arduino porque ponte.h tambem o usa, e a ponte e testada no PC.
 */
#ifndef CONFIG_WIFI_H
#define CONFIG_WIFI_H

#include <stdint.h>

// ---------------------------------------------------------------------------
// Rede criada pelo proprio ESP (ponto de acesso). Nao precisa de roteador nem
// de internet: o celular entra nesta rede e abre http://192.168.4.1
// ---------------------------------------------------------------------------
const char NOME_REDE[] = "Cadeira-TCC";

// TROQUE esta senha antes de levar a cadeira para fora da bancada: com a senha
// padrao, qualquer um que leia o repositorio entra na rede e comanda a
// cadeira. O WPA2 exige no minimo 8 caracteres; com menos, o softAP falha e a
// rede nem aparece.
const char SENHA_REDE[] = "cadeira123";

// 192.168.4.1 e o padrao do ESP8266 em modo ponto de acesso. Se mudar, mude
// tambem o endereco que o app usa quando e aberto como arquivo.
const uint8_t IP_ESP[4] = {192, 168, 4, 1};

// Canal fixo: o 6 fica no meio da faixa e e aceito em qualquer pais.
const int CANAL_WIFI = 6;

// Celulares conectados ao mesmo tempo. Cada um faz uma consulta de estado a
// cada 300 ms; mais clientes deixam o servidor lento para todos.
const int MAX_CLIENTES = 4;

// ---------------------------------------------------------------------------
// Serial com o Mega (Serial3 do Mega <-> Serial do ESP, chaves DIP 1 e 2)
// ---------------------------------------------------------------------------
// Precisa ser igual ao BAUD_ESP do config.h do Mega.
const long BAUD_MEGA = 115200;

// O Mega manda telemetria a cada 250 ms. Sem nenhuma linha valida por mais
// que isto, o app mostra a placa como desconectada e o ESP recusa comandos de
// movimento (parar continua passando sempre).
const unsigned long TEMPO_MEGA_OFFLINE_MS = 1500;

#endif  // CONFIG_WIFI_H
