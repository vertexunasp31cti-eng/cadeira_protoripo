#include "ESP8266WiFi.h"

bool ESP8266WiFiClass::persistente = true;

ESP8266WiFiClass WiFi;

ESP8266WiFiClass::ESP8266WiFiClass()
    : modo(WIFI_OFF),
      canal(0),
      oculta(0),
      max_clientes(0),
      ap_ligado(false),
      configurado_antes_do_ap(false),
      estacoes(0) {}

void ESP8266WiFiClass::persistent(bool persistent) { persistente = persistent; }

bool ESP8266WiFiClass::mode(WiFiMode_t m, WiFiState*) {
  modo = m;
  return true;
}

WiFiMode_t ESP8266WiFiClass::getMode() { return modo; }

bool ESP8266WiFiClass::softAP(const char* ssid, const char* psk, int channel,
                              int ssid_hidden, int max_connection, int) {
  // Mesmas recusas do core: senha WPA2 de 8 a 63 caracteres, ate 8 clientes.
  size_t tam_senha = psk != NULL ? strlen(psk) : 0;
  if (ssid == NULL || ssid[0] == '\0' || strlen(ssid) > 32) return false;
  if (tam_senha > 0 && (tam_senha < 8 || tam_senha > 63)) return false;
  if (max_connection < 0 || max_connection > 8) return false;
  if (channel < 1 || channel > 13) return false;
  nome_rede = ssid;
  senha = psk != NULL ? psk : "";
  canal = channel;
  oculta = ssid_hidden;
  max_clientes = max_connection;
  ap_ligado = true;
  return true;
}

bool ESP8266WiFiClass::softAP(const String& ssid, const String& passphrase,
                              int channel, int ssid_hidden, int max_connection,
                              int beacon_interval) {
  return softAP(ssid.c_str(), passphrase.c_str(), channel, ssid_hidden,
                max_connection, beacon_interval);
}

bool ESP8266WiFiClass::softAPConfig(IPAddress local_ip, IPAddress gw,
                                    IPAddress subnet) {
  ip = local_ip;
  gateway = gw;
  mascara = subnet;
  configurado_antes_do_ap = !ap_ligado;
  return true;
}

uint8_t ESP8266WiFiClass::softAPgetStationNum() { return estacoes; }

IPAddress ESP8266WiFiClass::softAPIP() { return ip; }
