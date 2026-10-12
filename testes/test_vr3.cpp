/*
 * test_vr3.cpp - Testes do driver do modulo de voz contra um V3 simulado.
 *
 * O modulo simulado so responde quando o Arduino abre a porta no mesmo baud
 * em que ele esta configurado, igual ao hardware real. Em baud diferente ele
 * devolve bytes sem sentido, como acontece com a serial de verdade.
 */
#include <string.h>

#include "Arduino.h"
#include "../firmware/cadeira_voz/vr3.h"

class ModuloSimulado : public HardwareSerial {
 public:
  ModuloSimulado(long baud_modulo, bool ligado)
      : baud_modulo_(baud_modulo), ligado_(ligado), baud_porta_(0),
        n_saida_(0), pos_saida_(0), n_entrada_(0) {
    memset(treinado_, 0, sizeof(treinado_));
  }

  void treinar(uint8_t r) { treinado_[r] = true; }
  void falar(uint8_t registro) {
    const uint8_t q[] = {0xAA, 0x07, 0x0D, 0x00, 0xFF, registro, 0x00, 0x00, 0x0A};
    responder(q, sizeof(q));
  }

  void begin(long baud) override {
    baud_porta_ = baud;
    n_saida_ = pos_saida_ = n_entrada_ = 0;
  }
  int available() override { return n_saida_ - pos_saida_; }
  int read() override {
    return pos_saida_ < n_saida_ ? saida_[pos_saida_++] : -1;
  }
  size_t write(uint8_t b) override {
    if (!ligado_) return 1;
    if (baud_porta_ != baud_modulo_) {
      // Baud errado: o modulo enxerga lixo e responde lixo.
      const uint8_t lixo[] = {0xF8, 0x80, 0x00};
      responder(lixo, sizeof(lixo));
      return 1;
    }
    entrada_[n_entrada_++] = b;
    if (n_entrada_ >= 2 && n_entrada_ == entrada_[1] + 2) {
      processar();
      n_entrada_ = 0;
    }
    return 1;
  }

 private:
  void responder(const uint8_t* q, int n) {
    for (int i = 0; i < n && n_saida_ < (int)sizeof(saida_); ++i)
      saida_[n_saida_++] = q[i];
  }

  void processar() {
    uint8_t cmd = entrada_[2];
    if (cmd == 0x31) {
      const uint8_t q[] = {0xAA, 0x02, 0x31, 0x0A};
      responder(q, sizeof(q));
    } else if (cmd == 0x30) {
      uint8_t n = entrada_[1] - 2;
      uint8_t q[32];
      uint8_t k = 0, ok = 0;
      q[k++] = 0xAA;
      q[k++] = 3 + 2 * n;
      q[k++] = 0x30;
      q[k++] = 0;  // N, preenchido abaixo
      for (uint8_t i = 0; i < n; ++i) {
        uint8_t r = entrada_[3 + i];
        q[k++] = r;
        q[k++] = treinado_[r] ? 0x00 : 0xFE;
        if (treinado_[r]) ++ok;
      }
      q[3] = ok;
      q[k++] = 0x0A;
      responder(q, k);
    }
  }

  long baud_modulo_;
  bool ligado_;
  long baud_porta_;
  uint8_t saida_[256];
  int n_saida_, pos_saida_;
  uint8_t entrada_[32];
  int n_entrada_;
  bool treinado_[80];
};

static int falhas = 0;
static void confere(bool ok, const char* nome) {
  printf("  [%s] %s\n", ok ? " ok " : "FALHOU", nome);
  if (!ok) ++falhas;
}

static const uint8_t REGS[7] = {0, 1, 2, 3, 4, 5, 6};

int main() {
  printf("Testes do driver do modulo de voz V3\n");

  {
    ModuloSimulado m(9600, true);
    for (uint8_t r = 0; r < 7; ++r) m.treinar(r);
    VR3 v(m);
    confere(v.iniciar(REGS, 7), "modulo de fabrica (9600) e encontrado");
    confere(v.baud() == 9600, "baud informado e 9600");
    confere(v.registrosCarregados() == 7, "os 7 registros carregam");
    m.falar(3);
    confere(v.lerReconhecimento() == 3, "palavra reconhecida vira registro 3");
  }
  {
    ModuloSimulado m(38400, true);
    for (uint8_t r = 0; r < 7; ++r) m.treinar(r);
    VR3 v(m);
    confere(v.iniciar(REGS, 7), "modulo com baud mudado (38400) e encontrado");
    confere(v.baud() == 38400, "baud informado e 38400");
    m.falar(4);
    confere(v.lerReconhecimento() == 4, "reconhece depois de trocar o baud");
  }
  {
    ModuloSimulado m(9600, true);
    m.treinar(0);
    m.treinar(4);
    VR3 v(m);
    confere(v.iniciar(REGS, 7), "inicia com parte dos registros treinada");
    confere(v.registrosCarregados() == 2, "conta 2 registros carregados");
    confere(v.statusRegistro(0) == REG_CARREGADO, "registro 0 carregado");
    confere(v.statusRegistro(1) == REG_NAO_TREINADO, "registro 1 nao treinado");
    confere(v.statusRegistro(4) == REG_CARREGADO, "registro 4 carregado");
  }
  {
    ModuloSimulado m(9600, true);
    VR3 v(m);
    confere(!v.iniciar(REGS, 7), "sem nenhum registro treinado, falha");
    confere(v.baud() == 9600, "mas o modulo foi encontrado");
    confere(strstr(v.ultimoErro(), "treinado") != NULL,
            "a mensagem diz que falta treinar");
  }
  {
    ModuloSimulado m(9600, false);
    VR3 v(m);
    confere(!v.iniciar(REGS, 7), "modulo desligado nao e encontrado");
    confere(v.bytesRecebidos() == 0, "nenhum byte recebido");
    confere(v.baud() == 0, "baud fica zerado");
  }

  if (falhas) {
    printf("%d teste(s) falharam.\n", falhas);
    return 1;
  }
  printf("Todos os testes do driver passaram.\n");
  return 0;
}
