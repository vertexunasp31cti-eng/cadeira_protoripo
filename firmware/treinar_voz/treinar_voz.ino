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

void carregar() {
  uint8_t registros[7] = {0, 1, 2, 3, 4, 5, 6};

  while (SERIAL_VOZ.available()) SERIAL_VOZ.read();
  enviarComando(CMD_LIMPAR, NULL, 0);
  if (receberQuadro(1000) == 0) {
    Serial.println(F("Sem resposta ao limpar o reconhecedor."));
    return;
  }
  enviarComando(CMD_CARREGAR, registros, 7);
  if (receberQuadro(1000) == 0) {
    Serial.println(F("Sem resposta ao carregar os registros."));
    return;
  }
  mostrarQuadro();
  Serial.println(F("Registros 0 a 6 carregados. Modo de escuta ligado:"));
  Serial.println(F("fale as palavras e confira se o numero certo aparece."));
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
  Serial.println(F("  l     carrega os 7 registros e entra em escuta"));
  Serial.println(F("  ?     mostra este menu"));
  Serial.println(F("---------------------------------------------------"));
}

void setup() {
  Serial.begin(115200);
  SERIAL_VOZ.begin(9600);
  delay(300);
  menu();
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
