/*
 * ===========================================================================
 *  FERRAMENTA DE TREINAMENTO DO MODULO DE VOZ V3
 *  Prototipo de cadeira de rodas com comando de voz - TCC de Robotica
 * ===========================================================================
 *
 *  Envie este sketch para o Mega, abra o Monitor Serial em 115200 com final
 *  de linha "Nova linha" e siga o menu. E autocontido de proposito: nao
 *  depende de biblioteca externa nem exige remontar a fiacao.
 *
 *  Fiacao (a mesma do firmware principal):
 *      modulo VCC -> 5V        modulo GND -> GND
 *      modulo TX  -> pino 19 (RX1)
 *      modulo RX  -> pino 18 (TX1)
 *
 *  Ordem dos registros esperada pelo firmware:
 *      0 frente | 1 tras | 2 esquerda | 3 direita
 *      4 parar  | 5 rapido | 6 devagar
 * ===========================================================================
 */

#define SERIAL_VOZ Serial1

static const uint8_t QUADRO_INICIO = 0xAA;
static const uint8_t QUADRO_FIM = 0x0A;
static const uint8_t CMD_TREINAR = 0x20;
static const uint8_t CMD_CARREGAR = 0x30;
static const uint8_t CMD_LIMPAR = 0x31;
static const uint8_t CMD_RESULTADO = 0x0D;

static const char* const PALAVRAS[7] = {"frente", "tras",   "esquerda",
                                        "direita", "parar", "rapido",
                                        "devagar"};

uint8_t quadro[32];
uint8_t quadro_tam = 0;
bool escutando = false;

// ---------------------------------------------------------------------------
void enviarComando(uint8_t cmd, const uint8_t* dados, uint8_t n) {
  SERIAL_VOZ.write(QUADRO_INICIO);
  SERIAL_VOZ.write((uint8_t)(n + 2));
  SERIAL_VOZ.write(cmd);
  for (uint8_t i = 0; i < n; ++i) SERIAL_VOZ.write(dados[i]);
  SERIAL_VOZ.write(QUADRO_FIM);
  SERIAL_VOZ.flush();
}

// Le um quadro completo. Retorna o tamanho, ou 0 se estourar o tempo.
uint8_t receberQuadro(unsigned long timeout_ms) {
  unsigned long inicio = millis();
  uint8_t n = 0;
  quadro_tam = 0;

  while ((millis() - inicio) < timeout_ms) {
    if (!SERIAL_VOZ.available()) continue;
    uint8_t b = (uint8_t)SERIAL_VOZ.read();

    if (n == 0) {
      if (b != QUADRO_INICIO) continue;
      quadro[n++] = b;
    } else if (n == 1) {
      if (b < 2 || b > (uint8_t)(sizeof(quadro) - 2)) { n = 0; continue; }
      quadro[n++] = b;
    } else {
      quadro[n++] = b;
      if (n >= (uint8_t)(quadro[1] + 2)) {
        if (quadro[quadro[1] + 1] == QUADRO_FIM) { quadro_tam = n; return n; }
        n = 0;
      }
    }
  }
  return 0;
}

void mostrarQuadro() {
  Serial.print(F("    bytes recebidos:"));
  for (uint8_t i = 0; i < quadro_tam; ++i) {
    Serial.print(' ');
    if (quadro[i] < 0x10) Serial.print('0');
    Serial.print(quadro[i], HEX);
  }
  Serial.println();
}

// ---------------------------------------------------------------------------
void treinar(uint8_t registro) {
  if (registro > 6) {
    Serial.println(F("Use um registro de 0 a 6."));
    return;
  }

  Serial.print(F("\nTreinando registro "));
  Serial.print(registro);
  Serial.print(F(" = \""));
  Serial.print(PALAVRAS[registro]);
  Serial.println(F("\""));
  Serial.println(F("Fale a palavra quando o modulo pedir. Ele pede DUAS vezes."));
  Serial.println(F("Fale sempre do mesmo jeito, mesma distancia e mesmo tom."));

  while (SERIAL_VOZ.available()) SERIAL_VOZ.read();
  uint8_t dados[1] = {registro};
  enviarComando(CMD_TREINAR, dados, 1);

  unsigned long inicio = millis();
  while ((millis() - inicio) < 15000) {
    if (receberQuadro(15000) == 0) break;
    mostrarQuadro();
    if (quadro[2] != CMD_TREINAR) continue;
    if (quadro[1] < 5) continue;          // quadro de abertura
    if (quadro[4] != registro) continue;  // resposta de outro registro

    uint8_t status = quadro[5];
    Serial.print(F("  -> "));
    if (status == 0x00) {
      Serial.println(F("GRAVADO com sucesso."));
    } else if (status == 0xFE) {
      Serial.println(F("FALHOU: tempo esgotado, o modulo nao ouviu nada."));
    } else if (status == 0xFF) {
      Serial.println(F("FALHOU: numero de registro invalido."));
    } else {
      Serial.println(F("FALHOU: as duas amostras nao coincidiram. Repita."));
    }
    return;
  }
  Serial.println(F("  -> Sem resposta. Confira RX/TX cruzados e a alimentacao."));
}

// Procura o modulo nas velocidades que o V3 aceita. Retorna o baud em que
// ele respondeu, ou 0. O padrao de fabrica e 9600, mas o baud fica gravado no
// modulo e pode ter sido mudado por outro programa.
long procurarModulo() {
  static const long BAUDS[] = {9600, 19200, 38400, 4800, 2400};
  for (uint8_t i = 0; i < sizeof(BAUDS) / sizeof(BAUDS[0]); ++i) {
    SERIAL_VOZ.begin(BAUDS[i]);
    delay(20);
    for (uint8_t t = 0; t < (i == 0 ? 4 : 1); ++t) {
      while (SERIAL_VOZ.available()) SERIAL_VOZ.read();
      enviarComando(CMD_LIMPAR, NULL, 0);
      unsigned long inicio = millis();
      while ((millis() - inicio) < 400) {
        if (receberQuadro(400 - (millis() - inicio)) == 0) break;
        if (quadro[2] == CMD_LIMPAR) return BAUDS[i];
      }
      delay(250);
    }
  }
  SERIAL_VOZ.begin(9600);
  return 0;
}

void carregar() {
  uint8_t registros[7] = {0, 1, 2, 3, 4, 5, 6};

  while (SERIAL_VOZ.available()) SERIAL_VOZ.read();
  enviarComando(CMD_LIMPAR, NULL, 0);
  if (receberQuadro(1000) == 0) {
    Serial.println(F("Sem resposta ao limpar o reconhecedor."));
    return;
  }
  enviarComando(CMD_CARREGAR, registros, 7);
  bool achou = false;
  unsigned long inicio = millis();
  while (!achou && (millis() - inicio) < 1000) {
    if (receberQuadro(1000) == 0) break;
    achou = (quadro[2] == CMD_CARREGAR);
  }
  if (!achou) {
    Serial.println(F("Sem resposta ao carregar os registros."));
    return;
  }
  mostrarQuadro();

  // Quadro: AA LEN 30 N R0 STA0 R1 STA1 ... 0A
  uint8_t fim = quadro[1] + 1;
  uint8_t carregados = 0;
  for (uint8_t p = 4; p + 1 < fim; p += 2) {
    uint8_t reg = quadro[p];
    uint8_t sta = quadro[p + 1];
    Serial.print(F("  registro "));
    Serial.print(reg);
    if (reg < 7) {
      Serial.print(F(" ("));
      Serial.print(PALAVRAS[reg]);
      Serial.print(')');
    }
    Serial.print(F(": "));
    if (sta == 0x00 || sta == 0xFC) {
      Serial.println(F("carregado"));
      ++carregados;
    } else if (sta == 0xFE) {
      Serial.println(F("NAO TREINADO - grave este registro"));
    } else if (sta == 0xFF) {
      Serial.println(F("fora da faixa"));
    } else {
      Serial.print(F("status 0x"));
      Serial.println(sta, HEX);
    }
  }

  if (carregados == 0) {
    Serial.println(F("Nenhum registro treinado. Grave de 0 a 6 antes."));
    return;
  }
  Serial.println(F("Modo de escuta ligado: fale as palavras e confira se o"));
  Serial.println(F("numero certo aparece."));
  escutando = true;
}

void escutar() {
  if (!escutando) return;
  if (receberQuadro(5) == 0) return;
  if (quadro[2] != CMD_RESULTADO || quadro[1] < 6) return;

  uint8_t registro = quadro[5];
  Serial.print(F("reconhecido: registro "));
  Serial.print(registro);
  if (registro < 7) {
    Serial.print(F("  = \""));
    Serial.print(PALAVRAS[registro]);
    Serial.print(F("\""));
  }
  Serial.println();
}

// ---------------------------------------------------------------------------
void menu() {
  Serial.println();
  Serial.println(F("==================================================="));
  Serial.println(F(" Treinamento do modulo de voz V3"));
  Serial.println(F("==================================================="));
  Serial.println(F("  0..6  treina o registro (fale a palavra 2 vezes)"));
  Serial.println(F("         0 frente   1 tras     2 esquerda"));
  Serial.println(F("         3 direita  4 parar    5 rapido"));
  Serial.println(F("         6 devagar"));
  Serial.println(F("  l     mostra quais registros estao gravados,"));
  Serial.println(F("        carrega os 7 e entra em escuta"));
  Serial.println(F("  ?     mostra este menu"));
  Serial.println(F("---------------------------------------------------"));
}

void setup() {
  Serial.begin(115200);
  delay(300);
  menu();

  Serial.println(F("Procurando o modulo de voz..."));
  long baud = procurarModulo();
  if (baud > 0) {
    Serial.print(F("Modulo encontrado a "));
    Serial.print(baud);
    Serial.println(F(" bps. Digite l para ver quais registros estao gravados."));
  } else {
    Serial.println(F("Modulo NAO respondeu em nenhuma velocidade."));
    Serial.println(F("  - TX do modulo no pino 19 e RX no pino 18 (cruzado)?"));
    Serial.println(F("  - VCC em 5V e GND ligado ao GND do Mega?"));
    Serial.println(F("  - LED do modulo aceso?"));
  }
}

void loop() {
  if (Serial.available()) {
    char c = (char)Serial.read();
    if (c >= '0' && c <= '6') {
      escutando = false;
      treinar((uint8_t)(c - '0'));
    } else if (c == 'l' || c == 'L') {
      carregar();
    } else if (c == '?') {
      menu();
    }
  }
  escutar();
}
