#include "vr3.h"

static const uint8_t QUADRO_INICIO = 0xAA;
static const uint8_t QUADRO_FIM = 0x0A;
static const uint8_t CMD_TREINAR = 0x20;     // grava um registro
static const uint8_t CMD_CARREGAR = 0x30;    // carrega registros no reconhecedor
static const uint8_t CMD_LIMPAR = 0x31;      // esvazia o reconhecedor
static const uint8_t CMD_RESULTADO = 0x0D;   // palavra reconhecida

// Posicao do numero do registro dentro do quadro de resultado.
static const uint8_t POS_REGISTRO = 5;

// Posicoes dentro do quadro de resposta do treinamento.
static const uint8_t POS_TREINO_REGISTRO = 4;
static const uint8_t POS_TREINO_STATUS = 5;

// Velocidades aceitas pelo V3, na ordem em que sao tentadas. A de fabrica
// vem primeiro; as outras so importam se alguem mudou o baud do modulo.
static const long BAUDS_VR3[] = {9600, 19200, 38400, 4800, 2400};
static const uint8_t TOTAL_BAUDS = sizeof(BAUDS_VR3) / sizeof(BAUDS_VR3[0]);

VR3::VR3(HardwareSerial& porta)
    : porta_(porta),
      idx_(0),
      quadro_tam_(0),
      bytes_vistos_(0),
      amostra_tam_(0),
      baud_(0),
      carregados_(0),
      erro_("") {
  for (uint8_t i = 0; i < 7; ++i) status_[i] = REG_SEM_RESPOSTA;
}

bool VR3::enviarComando(uint8_t cmd, const uint8_t* dados, uint8_t n) {
  if (n > 20) return false;
  porta_.write(QUADRO_INICIO);
  porta_.write((uint8_t)(n + 2));  // CMD + dados + 0x0A
  porta_.write(cmd);
  for (uint8_t i = 0; i < n; ++i) porta_.write(dados[i]);
  porta_.write(QUADRO_FIM);
  porta_.flush();
  return true;
}

uint8_t VR3::receberQuadro(unsigned long timeout_ms) {
  unsigned long inicio = millis();
  uint8_t n = 0;
  quadro_tam_ = 0;

  while ((millis() - inicio) < timeout_ms) {
    if (!porta_.available()) continue;
    uint8_t b = (uint8_t)porta_.read();
    ++bytes_vistos_;  // conta tudo que chega, ate o que nao faz sentido
    if (amostra_tam_ < sizeof(amostra_)) amostra_[amostra_tam_++] = b;

    if (n == 0) {
      if (b != QUADRO_INICIO) continue;
      quadro_[n++] = b;
    } else if (n == 1) {
      if (b < 2 || b > (uint8_t)(sizeof(quadro_) - 2)) {
        n = 0;  // comprimento impossivel: procura outro inicio de quadro
        continue;
      }
      quadro_[n++] = b;
    } else {
      quadro_[n++] = b;
      if (n >= (uint8_t)(quadro_[1] + 2)) {
        if (quadro_[quadro_[1] + 1] == QUADRO_FIM) {
          quadro_tam_ = n;
          return n;
        }
        n = 0;  // quadro malformado: recomeca
      }
    }
  }
  return 0;
}

bool VR3::esperarComando(uint8_t cmd_esperado, unsigned long timeout_ms) {
  unsigned long inicio = millis();
  while ((millis() - inicio) < timeout_ms) {
    uint8_t n = receberQuadro(timeout_ms);
    if (n == 0) return false;
    if (quadro_[2] == cmd_esperado) return true;
    // Quadro de outro comando: ignora e continua esperando.
  }
  return false;
}

bool VR3::procurar(long baud, uint8_t tentativas) {
  porta_.begin(baud);
  delay(20);
  for (uint8_t t = 0; t < tentativas; ++t) {
    while (porta_.available()) porta_.read();  // descarta lixo acumulado
    enviarComando(CMD_LIMPAR, NULL, 0);
    if (esperarComando(CMD_LIMPAR, 400)) return true;
    delay(250);
  }
  return false;
}

bool VR3::iniciar(const uint8_t* registros, uint8_t quantidade) {
  erro_ = "";
  idx_ = 0;
  baud_ = 0;
  carregados_ = 0;
  for (uint8_t i = 0; i < 7; ++i) status_[i] = REG_SEM_RESPOSTA;

  // O reconhecedor do V3 comporta no maximo 7 registros ao mesmo tempo.
  if (quantidade == 0 || quantidade > 7) {
    erro_ = "quantidade de registros invalida (1 a 7)";
    return false;
  }

  bytes_vistos_ = 0;
  amostra_tam_ = 0;

  // A primeira velocidade recebe varias tentativas, porque o modulo leva
  // algumas centenas de milissegundos para ligar junto com o Arduino. Nas
  // outras ele ja esta ligado, entao uma tentativa basta.
  for (uint8_t i = 0; i < TOTAL_BAUDS && baud_ == 0; ++i) {
    if (procurar(BAUDS_VR3[i], i == 0 ? 6 : 1)) baud_ = BAUDS_VR3[i];
  }

  if (baud_ == 0) {
    porta_.begin(BAUDS_VR3[0]);  // volta ao padrao para nao confundir ninguem
    erro_ = (bytes_vistos_ == 0)
        ? "nenhum byte chegou do modulo (confira alimentacao, GND comum e o "
          "fio TX do modulo no pino 19)"
        : "chegaram bytes, mas nenhum quadro valido em nenhuma velocidade "
          "(mau contato ou fio de sinal longo/solto?)";
    return false;
  }

  enviarComando(CMD_CARREGAR, registros, quantidade);
  if (!esperarComando(CMD_CARREGAR, 1000)) {
    erro_ = "o modulo nao respondeu ao comando de carregar registros";
    return false;
  }

  // Quadro: AA LEN 30 N R0 STA0 R1 STA1 ... 0A. Cada par diz o que aconteceu
  // com um registro pedido.
  uint8_t fim = quadro_[1] + 1;  // posicao do 0x0A
  for (uint8_t p = 4; p + 1 < fim; p += 2) {
    uint8_t reg = quadro_[p];
    uint8_t sta = quadro_[p + 1];
    for (uint8_t i = 0; i < quantidade; ++i) {
      if (registros[i] == reg) status_[i] = sta;
    }
  }
  for (uint8_t i = 0; i < quantidade; ++i) {
    if (status_[i] == REG_CARREGADO || status_[i] == REG_JA_CARREGADO) {
      ++carregados_;
    }
  }

  if (carregados_ == 0) {
    erro_ = "o modulo responde, mas nenhum numero esta gravado (rode a "
            "ferramenta treinar_voz)";
    return false;
  }
  return true;
}

ResultadoTreino VR3::treinar(uint8_t registro, unsigned long timeout_ms) {
  erro_ = "";
  idx_ = 0;

  if (registro > 79) {  // o V3 guarda 80 registros (0 a 79)
    erro_ = "numero de registro fora da faixa (0 a 79)";
    return TREINO_FORA_DA_FAIXA;
  }

  while (porta_.available()) porta_.read();

  uint8_t dados[1] = {registro};
  enviarComando(CMD_TREINAR, dados, 1);

  // O modulo responde um quadro de abertura e depois um por registro.
  // Esperamos o quadro que traz o resultado do registro pedido.
  unsigned long inicio = millis();
  while ((millis() - inicio) < timeout_ms) {
    if (receberQuadro(timeout_ms) == 0) break;
    if (quadro_[2] != CMD_TREINAR) continue;
    if (quadro_[1] < 5) continue;  // quadro de abertura, sem resultado
    if (quadro_[POS_TREINO_REGISTRO] != registro) continue;

    switch (quadro_[POS_TREINO_STATUS]) {
      case 0x00: return TREINO_OK;
      case 0xFE: erro_ = "tempo esgotado: o modulo nao ouviu a palavra";
                 return TREINO_TEMPO_ESGOTADO;
      case 0xFF: erro_ = "numero de registro fora da faixa";
                 return TREINO_FORA_DA_FAIXA;
      default:   erro_ = "as duas amostras de voz nao coincidiram";
                 return TREINO_FALHA;
    }
  }

  erro_ = "o modulo nao respondeu ao comando de treino";
  return TREINO_SEM_RESPOSTA;
}

int VR3::lerReconhecimento() {
  while (porta_.available()) {
    uint8_t b = (uint8_t)porta_.read();

    if (idx_ == 0) {
      if (b != QUADRO_INICIO) continue;
      buf_[idx_++] = b;
      continue;
    }

    if (idx_ == 1) {
      if (b < 2 || b > (uint8_t)(sizeof(buf_) - 2)) {
        idx_ = 0;  // comprimento impossivel: ressincroniza
        continue;
      }
      buf_[idx_++] = b;
      continue;
    }

    buf_[idx_++] = b;

    if (idx_ >= (uint8_t)(buf_[1] + 2)) {
      uint8_t len = buf_[1];
      int registro = -1;
      if (buf_[len + 1] == QUADRO_FIM && buf_[2] == CMD_RESULTADO &&
          len + 1 > POS_REGISTRO) {
        registro = buf_[POS_REGISTRO];
      }
      // Guarda o quadro para depuracao antes de zerar o montador.
      quadro_tam_ = idx_;
      for (uint8_t i = 0; i < idx_ && i < sizeof(quadro_); ++i) {
        quadro_[i] = buf_[i];
      }
      idx_ = 0;
      if (registro >= 0) return registro;
    }

    if (idx_ >= sizeof(buf_)) idx_ = 0;  // protecao contra estouro
  }
  return -1;
}

const char* nomeResultadoTreino(ResultadoTreino r) {
  switch (r) {
    case TREINO_OK: return "gravado com sucesso";
    case TREINO_TEMPO_ESGOTADO: return "tempo esgotado, nada foi ouvido";
    case TREINO_FORA_DA_FAIXA: return "numero de registro invalido";
    case TREINO_FALHA: return "as duas amostras nao coincidiram";
    default: return "sem resposta do modulo";
  }
}

const char* nomeStatusRegistro(StatusRegistro s) {
  switch (s) {
    case REG_CARREGADO: return "carregado";
    case REG_JA_CARREGADO: return "carregado";
    case REG_RECONHECEDOR_CHEIO: return "reconhecedor cheio";
    case REG_NAO_TREINADO: return "NAO GRAVADO";
    case REG_FORA_DA_FAIXA: return "fora da faixa";
    default: return "sem resposta";
  }
}
