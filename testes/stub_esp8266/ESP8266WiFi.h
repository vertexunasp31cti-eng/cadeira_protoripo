/*
 * ESP8266WiFi.h - Stub do Wi-Fi do core ESP8266, so o modo ponto de acesso.
 *
 * Guarda a configuracao recebida para o teste conferir nome da rede, canal,
 * limite de clientes e IP.
 */
#ifndef ESP8266WIFI_H_STUB_ESP8266
#define ESP8266WIFI_H_STUB_ESP8266

#include "Arduino.h"
#include "IPAddress.h"

enum WiFiMode { WIFI_OFF = 0, WIFI_STA = 1, WIFI_AP = 2, WIFI_AP_STA = 3 };
typedef enum WiFiMode WiFiMode_t;

struct WiFiState;

class ESP8266WiFiClass {
 public:
  ESP8266WiFiClass();

  static void persistent(bool persistent);
  bool mode(WiFiMode_t m, WiFiState* state = nullptr);
  WiFiMode_t getMode();

  bool softAP(const char* ssid, const char* psk = NULL, int channel = 1,
              int ssid_hidden = 0, int max_connection = 4,
              int beacon_interval = 100);
  bool softAP(const String& ssid, const String& passphrase = emptyString,
              int channel = 1, int ssid_hidden = 0, int max_connection = 4,
              int beacon_interval = 100);
  bool softAPConfig(IPAddress local_ip, IPAddress gateway, IPAddress subnet);
  uint8_t softAPgetStationNum();
  IPAddress softAPIP();

  // --- Ajudantes de teste ---------------------------------------------------
  static bool persistente;  // o core comeca com true
  WiFiMode_t modo;
  std::string nome_rede;
  std::string senha;
  int canal;
  int oculta;
  int max_clientes;
  bool ap_ligado;
  bool configurado_antes_do_ap;
  IPAddress ip;
  IPAddress gateway;
  IPAddress mascara;
  uint8_t estacoes;  // o que softAPgetStationNum() devolve
};

extern ESP8266WiFiClass WiFi;

#endif  // ESP8266WIFI_H_STUB_ESP8266
