/*
 * test_treinador.cpp - Testa a ferramenta treinar_voz contra um V3 simulado.
 *
 * O modulo simulado entrega cada byte 1 ms depois do anterior, como um V3
 * real a 9600 bps (cerca de 1,04 ms por byte). E esse ritmo que mostra se a
 * ferramenta junta direito um quadro que chega aos poucos.
 */
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <string>

#include "Arduino.h"

class ModuloV3Lento : public HardwareSerial {
 public:
  ModuloV3Lento() : n_(0), pos_(0), n_ent_(0), fim_(0) {
    memset(treinado_, 0, sizeof(treinado_));
  }

  void begin(long) override { n_ = pos_ = n_ent_ = 0; }

  int available() override {
    int k = 0;
    for (int i = pos_; i < n_ && t_[i] <= relogio_simulado; ++i) ++k;
    return k;
  }
  int read() override {
    if (pos_ < n_ && t_[pos_] <= relogio_simulado) return byte_[pos_++];
    return -1;
  }
  size_t write(uint8_t b) override {
    ent_[n_ent_++] = b;
    if (n_ent_ >= 2 && n_ent_ == ent_[1] + 2) {
      processar();
      n_ent_ = 0;
    }
    return 1;
  }

  // Quadro de reconhecimento, igual ao que o V3 manda ao ouvir a palavra.
  void falar(uint8_t r) {
    const uint8_t q[] = {0xAA, 0x07, 0x0D, 0x00, 0xFF, r, 0x00, 0x00, 0x0A};
    responder(q, sizeof(q), 0);
  }

 private:
  void responder(const uint8_t* q, int n, unsigned long atraso) {
    unsigned long t = relogio_simulado + atraso;
    if (t < fim_) t = fim_;
    for (int i = 0; i < n; ++i) {
      t += 1;
      byte_[n_] = q[i];
      t_[n_] = t;
      ++n_;
    }
    fim_ = t;
  }

  void aviso(uint8_t r, const char* texto, unsigned long atraso) {
    uint8_t q[32];
    uint8_t k = 0;
    uint8_t len = (uint8_t)strlen(texto);
    q[k++] = 0xAA;
    q[k++] = len + 3;
    q[k++] = 0x0A;
    q[k++] = r;
    for (uint8_t i = 0; i < len; ++i) q[k++] = (uint8_t)texto[i];
    q[k++] = 0x0A;
    responder(q, k, atraso);
  }

  void processar() {
    uint8_t cmd = ent_[2];
    if (cmd == 0x31) {
      const uint8_t q[] = {0xAA, 0x02, 0x31, 0x0A};
      responder(q, sizeof(q), 5);
    } else if (cmd == 0x30) {
      uint8_t n = ent_[1] - 2, k = 0, ok = 0, q[32];
      q[k++] = 0xAA;
      q[k++] = 3 + 2 * n;
      q[k++] = 0x30;
      q[k++] = 0;
      for (uint8_t i = 0; i < n; ++i) {
        uint8_t r = ent_[3 + i];
        q[k++] = r;
        q[k++] = treinado_[r] ? 0x00 : 0xFE;
        if (treinado_[r]) ++ok;
      }
      q[3] = ok;
      q[k++] = 0x0A;
      responder(q, k, 5);
    } else if (cmd == 0x20) {
      uint8_t r = ent_[3];
      aviso(r, "Speak now", 50);
      aviso(r, "Speak again", 1500);
      aviso(r, "Success", 1500);
      const uint8_t q[] = {0xAA, 0x05, 0x20, 0x01, r, 0x00, 0x0A};
      responder(q, sizeof(q), 10);
      treinado_[r] = true;
    }
  }

  uint8_t byte_[2048];
  unsigned long t_[2048];
  int n_, pos_;
  uint8_t ent_[32];
  int n_ent_;
  unsigned long fim_;
  bool treinado_[80];
};

ModuloV3Lento modulo_sim;
#define SERIAL_VOZ modulo_sim
#include "../firmware/treinar_voz/treinar_voz.ino"

// Captura o que a ferramenta escreve no Monitor Serial (stdout no PC).
static FILE* captura = NULL;
static int stdout_original = -1;

static void comecarCaptura() {
  fflush(stdout);
  captura = tmpfile();
  stdout_original = dup(1);
  dup2(fileno(captura), 1);
}

static std::string terminarCaptura() {
  fflush(stdout);
  dup2(stdout_original, 1);
  close(stdout_original);
  std::string s;
  rewind(captura);
  int c;
  while ((c = fgetc(captura)) != EOF) s += (char)c;
  fclose(captura);
  return s;
}

static int falhas = 0;
static void confere(bool ok, const char* nome) {
  printf("  [%s] %s\n", ok ? " ok " : "FALHOU", nome);
  if (!ok) ++falhas;
}
static bool tem(const std::string& s, const char* trecho) {
  return s.find(trecho) != std::string::npos;
}

int main() {
  printf("Testes da ferramenta de treinamento (modulo a 9600 bps)\n");

  comecarCaptura();
  setup();
  std::string saida = terminarCaptura();
  confere(tem(saida, "Modulo encontrado a 9600"), "encontra o modulo ao ligar");

  comecarCaptura();
  treinar(3);
  saida = terminarCaptura();
  confere(tem(saida, "FALE A PALAVRA AGORA"), "avisa a hora de falar");
  confere(tem(saida, "FALE A PALAVRA DE NOVO"), "avisa a hora de repetir");
  confere(tem(saida, "GRAVADO com sucesso"), "informa que gravou");

  comecarCaptura();
  carregar();
  saida = terminarCaptura();
  confere(tem(saida, "registro 3 (direita): carregado"),
          "lista o registro gravado como carregado");
  confere(tem(saida, "registro 0 (frente): NAO TREINADO"),
          "lista o registro vazio como nao treinado");
  confere(escutando, "entra no modo de escuta");

  comecarCaptura();
  modulo_sim.falar(3);
  for (int i = 0; i < 100; ++i) {
    relogio_simulado += 1;
    escutar();
  }
  saida = terminarCaptura();
  confere(tem(saida, "reconhecido: registro 3"),
          "modo de escuta mostra a palavra que chega aos poucos");

  if (falhas) {
    printf("%d teste(s) falharam.\n", falhas);
    return 1;
  }
  printf("Todos os testes da ferramenta passaram.\n");
  return 0;
}
