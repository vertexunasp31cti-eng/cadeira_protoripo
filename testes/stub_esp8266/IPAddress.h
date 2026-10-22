/*
 * IPAddress.h - Stub do IPAddress do core ESP8266 (so IPv4).
 */
#ifndef IPADDRESS_H_STUB_ESP8266
#define IPADDRESS_H_STUB_ESP8266

#include "Arduino.h"

class IPAddress {
 public:
  IPAddress() : b_{0, 0, 0, 0} {}
  IPAddress(uint8_t first_octet, uint8_t second_octet, uint8_t third_octet,
            uint8_t fourth_octet)
      : b_{first_octet, second_octet, third_octet, fourth_octet} {}
  IPAddress(const uint8_t* address)
      : b_{address[0], address[1], address[2], address[3]} {}

  uint8_t operator[](int index) const { return b_[index & 3]; }
  bool operator==(const IPAddress& outro) const {
    return memcmp(b_, outro.b_, 4) == 0;
  }

  String toString() const {
    char s[16];
    snprintf(s, sizeof(s), "%u.%u.%u.%u", (unsigned)b_[0], (unsigned)b_[1],
             (unsigned)b_[2], (unsigned)b_[3]);
    return String(s);
  }

 private:
  uint8_t b_[4];
};

#endif  // IPADDRESS_H_STUB_ESP8266
