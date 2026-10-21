#include "protocolo_app.h"

LeitorProtocoloApp::LeitorProtocoloApp()
    : tam_(0), estourou_(false), invalidas_(0) {}

char letraDoComando(Comando c) {
  switch (c) {
    case CMD_FRENTE: return 'F';
    case CMD_RE: return 'T';
    case CMD_ESQUERDA: return 'E';
    case CMD_DIREITA: return 'D';
    case CMD_PARAR: return 'P';
    case CMD_RAPIDO: return '+';
    case CMD_DEVAGAR: return '-';
    default: return 0;
  }
}

Comando LeitorProtocoloApp::interpretar() const {
  // Formato exato: "CMD:" seguido de uma letra, nada mais.
  if (tam_ != 5) return CMD_NENHUM;
  if (linha_[0] != 'C' || linha_[1] != 'M' || linha_[2] != 'D' ||
      linha_[3] != ':') {
    return CMD_NENHUM;
  }
  for (int c = 0; c < CMD_TOTAL; ++c) {
    if (letraDoComando((Comando)c) == linha_[4]) return (Comando)c;
  }
  return CMD_NENHUM;
}

Comando LeitorProtocoloApp::alimentar(char c) {
  if (c == '\r') return CMD_NENHUM;  // aceita fim de linha no estilo Windows

  if (c != '\n') {
    if (tam_ < TAMANHO_MAXIMO) {
      linha_[tam_++] = c;
    } else {
      estourou_ = true;  // linha longa demais: sera descartada inteira
    }
    return CMD_NENHUM;
  }

  // Fim de linha: interpreta e recomeca.
  Comando cmd = estourou_ ? CMD_NENHUM : interpretar();
  if (cmd == CMD_NENHUM && (tam_ > 0 || estourou_)) ++invalidas_;
  tam_ = 0;
  estourou_ = false;
  return cmd;
}
