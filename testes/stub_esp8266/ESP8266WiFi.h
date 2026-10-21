/*
 * ESP8266WiFi.h - Stub minimo do Wi-Fi do ESP8266, so para os testes no PC.
 *
 * Junto com stub_arduino/Arduino.h e ESP8266WebServer.h permite compilar e
 * exercitar firmware/app_wifi/app_wifi.ino sem a placa.
 */
#ifndef ESP8266WIFI_H_STUB
#define ESP8266WIFI_H_STUB

#include <Arduino.h>

#include <string>

// String do Arduino, so com o que o sketch usa.
class String {
 public:
  String() {}
  String(const char* s) : s_(s ? s : "") {}
  explicit String(int v) : s_(std::to_string(v)) {}
  explicit String(unsigned int v) : s_(std::to_string(v)) {}
  String& operator+=(const char* s) { s_ += s; return *this; }
  String& operator+=(const String& o) { s_ += o.s_; return *this; }
  bool operator==(const char* s) const { return s_ == s; }
  const char* c_str() const { return s_.c_str(); }

 private:
  std::string s_;
};

enum WiFiMode { WIFI_OFF, WIFI_STA, WIFI_AP, WIFI_AP_STA };

class WiFiStub {
 public:
  void mode(WiFiMode) {}
  bool softAP(const char* nome, const char* senha) {
    nome_ = nome;
    senha_ = senha;
    return true;
  }
  unsigned int softAPgetStationNum() { return 1; }
  std::string nome_, senha_;
};

extern WiFiStub WiFi;

#endif  // ESP8266WIFI_H_STUB
