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
 *  Os comandos sao numeros falados. Voce digita o numero e fala o mesmo
 *  numero: digita 1 e fala "um", e isso vira o comando frente.
 *      1 um = frente   | 2 dois = tras    | 3 tres  = esquerda
 *      4 quatro = direita | 5 cinco = parar | 6 seis = rapido
 *      7 sete = devagar
 *  Por dentro, o numero 1 fica no registro 0 do modulo, o 2 no registro 1, e
 *  assim por diante, que e a ordem que o firmware da cadeira espera.
 * ===========================================================================
 */

#include <string.h>

#ifndef SERIAL_VOZ
#define SERIAL_VOZ Serial1
#endif

static const uint8_t QUADRO_INICIO = 0xAA;
static const uint8_t QUADRO_FIM = 0x0A;
static const uint8_t CMD_TREINAR = 0x20;
static const uint8_t CMD_CARREGAR = 0x30;
static const uint8_t CMD_LIMPAR = 0x31;
static const uint8_t CMD_RESULTADO = 0x0D;
static const uint8_t CMD_AVISO = 0x0A;  // texto do modulo durante o treino

// O que se fala para cada registro, e o comando que ele vira na cadeira.
static const char* const FALAS[7] = {"um",    "dois", "tres", "quatro",
                                     "cinco", "seis", "sete"};
static const char* const ACOES[7] = {"frente", "tras",   "esquerda", "direita",
                                     "parar",  "rapido", "devagar"};

uint8_t quadro[32];
uint8_t quadro_tam = 0;
bool escutando = false;

// Montagem nao bloqueante usada no modo de escuta (veja montarQuadro).
uint8_t escuta_buf[32];
uint8_t escuta_n = 0;

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

// Durante o treino o modulo manda avisos em texto, em ingles:
//   AA LEN 0A registro "Speak now" 0A
// Mostrar esse texto e o que diz ao usuario a hora certa de falar.
void mostrarAviso() {
  char texto[28];
  uint8_t n = 0;
  for (uint8_t i = 4; i < (uint8_t)(quadro[1] + 1) && n < sizeof(texto) - 1; ++i) {
    texto[n++] = (char)quadro[i];
  }
  texto[n] = '\0';

  Serial.print(F("  modulo: \""));
  Serial.print(texto);
  Serial.print(F("\"  -> "));
  if (strstr(texto, "again")) {
    Serial.println(F("FALE A PALAVRA DE NOVO, igual a primeira vez"));
  } else if (strstr(texto, "now") || strstr(texto, "Speak")) {
    Serial.println(F("FALE A PALAVRA AGORA"));
  } else if (strstr(texto, "ailed")) {
    Serial.println(F("nao gravou (veja o motivo abaixo)"));
  } else if (strstr(texto, "uccess")) {
    Serial.println(F("deu certo"));
  } else if (strstr(texto, "atch")) {
    Serial.println(F("as duas falas sairam diferentes"));
    Serial.println(F("     O modulo vai pedir de novo. Fale igual nas duas vezes:"));
    Serial.println(F("     mesma distancia, mesmo tom, so a palavra, logo apos o aviso."));
  } else {
    Serial.println();
  }
}

// Mostra um registro do jeito que o usuario conhece, por exemplo:
//   numero 1 "um" (frente)
void mostrarNumero(uint8_t registro) {
  Serial.print(F("numero "));
  Serial.print(registro + 1);
  if (registro < 7) {
    Serial.print(F(" \""));
    Serial.print(FALAS[registro]);
    Serial.print(F("\" ("));
    Serial.print(ACOES[registro]);
    Serial.print(')');
  }
}

// ---------------------------------------------------------------------------
void treinar(uint8_t registro) {
  if (registro > 6) {
    Serial.println(F("Use um numero de 1 a 7."));
    return;
  }

  Serial.print(F("\nGravando o "));
  mostrarNumero(registro);
  Serial.println();
  Serial.print(F("Quando o modulo pedir, fale \""));
  Serial.print(FALAS[registro]);
  Serial.println(F("\". Ele pede DUAS vezes."));
  Serial.println(F("Fale sempre do mesmo jeito, mesma distancia e mesmo tom."));

  while (SERIAL_VOZ.available()) SERIAL_VOZ.read();
  uint8_t dados[1] = {registro};
  enviarComando(CMD_TREINAR, dados, 1);

  // Quando as duas falas nao batem, o modulo pede tudo de novo sozinho, quantas
  // vezes for preciso. Por isso nao ha um tempo total curto: a ferramenta so
  // desiste se o modulo ficar 15 s calado, ou depois de 3 minutos tentando.
  unsigned long inicio = millis();
  bool modulo_falou = false;
  while ((millis() - inicio) < 180000UL) {
    if (receberQuadro(15000) == 0) break;
    modulo_falou = true;
    if (quadro[2] == CMD_AVISO) {
      mostrarAviso();
      continue;
    }
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
  if (modulo_falou) {
    // O modulo esta ligado e conversando; so nao terminou a gravacao. Ele pode
    // ter ficado no meio do treino, entao o mais seguro e reinicia-lo.
    Serial.println(F("  -> O modulo nao terminou a gravacao."));
    Serial.println(F("     Tire o USB por 2 segundos, ligue de novo e repita o numero."));
  } else {
    Serial.println(F("  -> Sem resposta. Confira RX/TX cruzados e a alimentacao."));
  }
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
      for (;;) {
        unsigned long passou = millis() - inicio;
        if (passou >= 400) break;
        if (receberQuadro(400 - passou) == 0) break;
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
    Serial.print(F("  "));
    mostrarNumero(reg);
    Serial.print(F(": "));
    if (sta == 0x00 || sta == 0xFC) {
      Serial.println(F("carregado"));
      ++carregados;
    } else if (sta == 0xFE) {
      Serial.println(F("NAO GRAVADO - grave este numero"));
    } else if (sta == 0xFF) {
      Serial.println(F("fora da faixa"));
    } else {
      Serial.print(F("status 0x"));
      Serial.println(sta, HEX);
    }
  }

  if (carregados == 0) {
    Serial.println(F("Nenhum numero gravado. Grave de 1 a 7 antes."));
    return;
  }
  Serial.println(F("Modo de escuta ligado: fale os numeros e confira se o"));
  Serial.println(F("comando certo aparece."));
  escuta_n = 0;
  escutando = true;
}

// Monta um quadro aos poucos, sem bloquear. A 9600 bps um quadro de
// resultado leva cerca de 10 ms para chegar inteiro, entao ele precisa ser
// juntado ao longo de varias voltas do loop(), sem perder o que ja chegou.
bool montarQuadro() {
  while (SERIAL_VOZ.available()) {
    uint8_t b = (uint8_t)SERIAL_VOZ.read();
    if (escuta_n == 0) {
      if (b == QUADRO_INICIO) escuta_buf[escuta_n++] = b;
      continue;
    }
    if (escuta_n == 1) {
      if (b < 2 || b > (uint8_t)(sizeof(escuta_buf) - 2)) {
        escuta_n = 0;
        continue;
      }
      escuta_buf[escuta_n++] = b;
      continue;
    }
    escuta_buf[escuta_n++] = b;
    if (escuta_n >= (uint8_t)(escuta_buf[1] + 2)) {
      bool ok = (escuta_buf[escuta_buf[1] + 1] == QUADRO_FIM);
      if (ok) {
        memcpy(quadro, escuta_buf, escuta_n);
        quadro_tam = escuta_n;
      }
      escuta_n = 0;
      if (ok) return true;
    }
  }
  return false;
}

void escutar() {
  if (!escutando) return;
  if (!montarQuadro()) return;
  if (quadro[2] != CMD_RESULTADO || quadro[1] < 6) return;

  uint8_t registro = quadro[5];
  Serial.print(F("reconhecido: "));
  mostrarNumero(registro);
  Serial.println();
}

// ---------------------------------------------------------------------------
void menu() {
  Serial.println();
  Serial.println(F("==================================================="));
  Serial.println(F(" Treinamento do modulo de voz V3"));
  Serial.println(F("==================================================="));
  Serial.println(F("  1..7  grava o numero (fale o mesmo numero, 2 vezes)"));
  Serial.println(F("          1 um     = frente     2 dois   = tras"));
  Serial.println(F("          3 tres   = esquerda   4 quatro = direita"));
  Serial.println(F("          5 cinco  = parar      6 seis   = rapido"));
  Serial.println(F("          7 sete   = devagar"));
  Serial.println(F("  l     mostra quais numeros estao gravados,"));
  Serial.println(F("        carrega os 7 e entra em escuta"));
  Serial.println(F("  b     procura o modulo de novo (depois de mexer nos fios)"));
  Serial.println(F("  ?     mostra este menu"));
  Serial.println(F("---------------------------------------------------"));
}

void buscarModulo() {
  Serial.println(F("Procurando o modulo de voz..."));
  long baud = procurarModulo();
  if (baud > 0) {
    Serial.print(F("Modulo encontrado a "));
    Serial.print(baud);
    Serial.println(F(" bps. Digite l para ver quais numeros estao gravados."));
  } else {
    Serial.println(F("Modulo NAO respondeu em nenhuma velocidade."));
    Serial.println(F("  - TX do modulo no pino 19 e RX no pino 18 (cruzado)?"));
    Serial.println(F("  - VCC em 5V e GND ligado ao GND do Mega?"));
    Serial.println(F("  - LED do modulo aceso?"));
    Serial.println(F("Arrume e digite b para procurar de novo."));
  }
}

// Pinos da ponte H, os mesmos de firmware/cadeira_voz/config.h. Esta
// ferramenta nao usa os motores, mas deixa esses pinos em nivel baixo: soltos,
// eles flutuam e, com a bateria ligada no L298N, uma roda pode girar sozinha
// durante o treino.
static const uint8_t PINOS_PONTE_H[6] = {5, 35, 33, 6, 34, 32};

void setup() {
  for (uint8_t i = 0; i < 6; ++i) {
    pinMode(PINOS_PONTE_H[i], OUTPUT);
    digitalWrite(PINOS_PONTE_H[i], LOW);
  }
  Serial.begin(115200);
  delay(300);
  menu();
  buscarModulo();
}

void loop() {
  if (Serial.available()) {
    char c = (char)Serial.read();
    if (c >= '1' && c <= '7') {
      escutando = false;
      treinar((uint8_t)(c - '1'));
    } else if (c == '0' || c == '8' || c == '9') {
      Serial.println(F("Use os numeros de 1 a 7."));
    } else if (c == 'l' || c == 'L') {
      carregar();
    } else if (c == 'b' || c == 'B') {
      escutando = false;
      buscarModulo();
    } else if (c == '?') {
      menu();
    }
  }
  escutar();
}
