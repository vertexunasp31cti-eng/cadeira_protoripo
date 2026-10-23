#include "protocolo_app.h"

#include <stdio.h>
#include <string.h>

#if defined(__AVR__)
#include <avr/pgmspace.h>
// No AVR, texto fixo fica na RAM (so 8 KB) se nao for marcado. PSTR deixa o
// formato na flash, a mesma ideia do F() usado no .ino.
#define FORMATAR(buf, cap, formato, ...) \
  snprintf_P((buf), (cap), PSTR(formato), __VA_ARGS__)
#else
#define FORMATAR(buf, cap, formato, ...) \
  snprintf((buf), (cap), formato, __VA_ARGS__)
#endif

// Nomes aceitos do app, na mesma ordem do enum Comando.
static const char* const NOMES_APP[CMD_TOTAL] = {
    "frente", "tras", "esquerda", "direita", "parar", "rapido", "devagar"};

static const char DIGITOS_HEX[] = "0123456789ABCDEF";

static bool imprimivel(char c) {
  uint8_t b = (uint8_t)c;
  return b >= 0x20 && b <= 0x7E;
}

// Valor de um digito hexadecimal, maiusculo ou minusculo; -1 se nao for.
static int valorHex(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  return -1;
}

uint8_t checksumLinha(const char* s, unsigned int n) {
  uint8_t x = 0;
  for (unsigned int i = 0; i < n; ++i) x ^= (uint8_t)s[i];
  return x;
}

// ---------------------------------------------------------------------------
// Entrada: linhas de comando vindas do ESP
// ---------------------------------------------------------------------------
LeitorLinhas::LeitorLinhas() : n_(0), descartando_(false), t_ultimo_byte_(0) {
  buf_[0] = '\0';
  // So o terminador da linha escreve nesta posicao, entao linha() nunca passa
  // do fim do buffer, mesmo depois de lixo.
  buf_[TAM_LINHA_ENTRADA_APP - 1] = '\0';
}

bool LeitorLinhas::receber(char c) {
  if (c == '\r') return false;

  if (c == '\n') {
    bool completa = !descartando_ && n_ > 0;
    if (completa) buf_[n_] = '\0';
    n_ = 0;
    descartando_ = false;
    return completa;
  }

  if (descartando_) return false;

  if (!imprimivel(c) || n_ >= TAM_LINHA_ENTRADA_APP - 1) {
    descartando_ = true;
    n_ = 0;
    return false;
  }

  buf_[n_++] = c;
  return false;
}

bool LeitorLinhas::receber(char c, unsigned long agora_ms) {
  // Subtracao sem sinal: continua certa quando o millis() da a volta.
  if (agora_ms - t_ultimo_byte_ >= SILENCIO_LEITOR_APP_MS) reiniciar();
  t_ultimo_byte_ = agora_ms;
  return receber(c);
}

void LeitorLinhas::descartarLinhaAtual() {
  // No inicio de linha nao ha nada a jogar fora. Marcar descarte ali perderia
  // a proxima linha inteira, que pode ser um "parar".
  if (n_ > 0) {
    n_ = 0;
    descartando_ = true;
  }
}

void LeitorLinhas::reiniciar() {
  n_ = 0;
  descartando_ = false;
}

LinhaApp interpretarLinhaApp(const char* linha) {
  LinhaApp r;
  r.tipo = LINHA_INVALIDA;
  r.cmd = CMD_NENHUM;
  if (linha == NULL) return r;

  // Mede com limite: nenhuma linha valida chega perto do buffer de entrada.
  unsigned int n = 0;
  while (linha[n] != '\0') {
    if (++n >= TAM_LINHA_ENTRADA_APP) return r;
  }

  // "XXX:" + nome com pelo menos 1 letra + "*HH".
  if (n < 8 || linha[n - 3] != '*') return r;
  int alto = valorHex(linha[n - 2]);
  int baixo = valorHex(linha[n - 1]);
  if (alto < 0 || baixo < 0) return r;

  unsigned int tam_conteudo = n - 3;
  for (unsigned int i = 0; i < tam_conteudo; ++i) {
    if (!imprimivel(linha[i])) return r;
  }
  if (checksumLinha(linha, tam_conteudo) != (uint8_t)(alto * 16 + baixo)) {
    return r;
  }

  TipoLinhaApp tipo;
  if (strncmp(linha, "CMD:", 4) == 0) {
    tipo = LINHA_COMANDO;
  } else if (strncmp(linha, "MNT:", 4) == 0) {
    tipo = LINHA_RENOVACAO;
  } else {
    return r;
  }

  const char* nome = linha + 4;
  unsigned int tam_nome = tam_conteudo - 4;
  for (int i = 0; i < CMD_TOTAL; ++i) {
    if (strlen(NOMES_APP[i]) == tam_nome &&
        memcmp(NOMES_APP[i], nome, tam_nome) == 0) {
      Comando c = (Comando)i;
      // Giro, parar e velocidade nao se renovam: so frente e re seguram.
      if (tipo == LINHA_RENOVACAO && c != CMD_FRENTE && c != CMD_RE) return r;
      r.tipo = tipo;
      r.cmd = c;
      return r;
    }
  }
  return r;
}

AcaoApp decidirAcaoApp(const LinhaApp& l, bool permitir, bool emergencia) {
  if (emergencia) return ACAO_IGNORAR;

  if (l.tipo == LINHA_COMANDO) {
    if (l.cmd < CMD_FRENTE || l.cmd >= CMD_TOTAL) return ACAO_IGNORAR;
    // Parar e sempre aceito, mesmo com o app so acompanhando.
    if (permitir || l.cmd == CMD_PARAR) return ACAO_TRATAR;
    return ACAO_RECUSAR;
  }

  if (l.tipo == LINHA_RENOVACAO && permitir &&
      (l.cmd == CMD_FRENTE || l.cmd == CMD_RE)) {
    return ACAO_RENOVAR;
  }
  return ACAO_IGNORAR;
}

// ---------------------------------------------------------------------------
// Saida: telemetria e eventos para o ESP
// ---------------------------------------------------------------------------

// Acrescenta "*HH\n" ao conteudo que o snprintf deixou em buf (n = retorno
// dele). Se nao coube, esvazia buf para nunca sair meia linha.
static unsigned int fecharLinha(char* buf, unsigned int cap, int n) {
  if (n <= 0 || (unsigned int)n + 5 > cap) {  // "*HH\n" + '\0'
    if (cap > 0) buf[0] = '\0';
    return 0;
  }
  unsigned int k = (unsigned int)n;
  uint8_t x = checksumLinha(buf, k);
  buf[k] = '*';
  buf[k + 1] = DIGITOS_HEX[x >> 4];
  buf[k + 2] = DIGITOS_HEX[x & 0x0F];
  buf[k + 3] = '\n';
  buf[k + 4] = '\0';
  return k + 4;
}

static unsigned int linhaVazia(char* buf, unsigned int cap) {
  if (cap > 0) buf[0] = '\0';
  return 0;
}

// Texto que vai entre aspas no JSON.
static bool textoSeguro(const char* s) {
  if (s == NULL || s[0] == '\0') return false;
  for (; *s != '\0'; ++s) {
    if (!imprimivel(*s) || *s == '"' || *s == '\\') return false;
  }
  return true;
}

unsigned int formatarTelemetria(char* buf, unsigned int cap,
                                const DadosTelemetria& d) {
  int n = FORMATAR(buf, cap,
                   "{\"t\":\"tel\",\"ms\":%lu,\"estado\":\"%s\","
                   "\"pwmEsq\":%d,\"pwmDir\":%d,\"nivel\":%d,"
                   "\"obstaculo\":%d,\"emergencia\":%d,\"motivo\":\"%s\","
                   "\"voz\":%d,\"app\":%d}",
                   d.ms, nomeEstado(d.estado), d.pwm_esq, d.pwm_dir, d.nivel,
                   d.obstaculo ? 1 : 0, d.emergencia ? 1 : 0,
                   codigoMotivo(d.motivo), d.voz ? 1 : 0, d.app ? 1 : 0);
  return fecharLinha(buf, cap, n);
}

unsigned int formatarEventoInicio(char* buf, unsigned int cap,
                                  unsigned long ms, bool voz) {
  int n = FORMATAR(buf, cap,
                   "{\"t\":\"evt\",\"ms\":%lu,\"tipo\":\"inicio\",\"voz\":%d}",
                   ms, voz ? 1 : 0);
  return fecharLinha(buf, cap, n);
}

unsigned int formatarEventoComando(char* buf, unsigned int cap,
                                   unsigned long ms, Comando c,
                                   const char* origem, bool aceito) {
  if (!textoSeguro(origem)) return linhaVazia(buf, cap);
  int n = FORMATAR(buf, cap,
                   "{\"t\":\"evt\",\"ms\":%lu,\"tipo\":\"comando\","
                   "\"cmd\":\"%s\",\"origem\":\"%s\",\"aceito\":%d}",
                   ms, nomeComando(c), origem, aceito ? 1 : 0);
  return fecharLinha(buf, cap, n);
}

unsigned int formatarEventoParada(char* buf, unsigned int cap,
                                  unsigned long ms, MotivoParada m) {
  int n = FORMATAR(buf, cap,
                   "{\"t\":\"evt\",\"ms\":%lu,\"tipo\":\"parada\","
                   "\"motivo\":\"%s\"}",
                   ms, codigoMotivo(m));
  return fecharLinha(buf, cap, n);
}

unsigned int formatarEventoObstaculo(char* buf, unsigned int cap,
                                     unsigned long ms, bool ativo) {
  int n = FORMATAR(buf, cap,
                   "{\"t\":\"evt\",\"ms\":%lu,\"tipo\":\"obstaculo\","
                   "\"ativo\":%d}",
                   ms, ativo ? 1 : 0);
  return fecharLinha(buf, cap, n);
}

unsigned int formatarEventoEmergencia(char* buf, unsigned int cap,
                                      unsigned long ms, bool ativa) {
  int n = FORMATAR(buf, cap,
                   "{\"t\":\"evt\",\"ms\":%lu,\"tipo\":\"emergencia\","
                   "\"ativa\":%d}",
                   ms, ativa ? 1 : 0);
  return fecharLinha(buf, cap, n);
}

// ---------------------------------------------------------------------------
// Estatisticas de uso
// ---------------------------------------------------------------------------
static void somar(unsigned int& contador) {
  if (contador < 65535u) ++contador;
}

void zerarEstatisticas(Estatisticas& e) { memset(&e, 0, sizeof(e)); }

void contarComando(Estatisticas& e, const char* origem, bool aceito) {
  if (origem != NULL) {
    if (strcmp(origem, "voz") == 0) {
      somar(e.cmd_voz);
    } else if (strcmp(origem, "app") == 0) {
      somar(e.cmd_app);
    } else if (strcmp(origem, "teclado") == 0) {
      somar(e.cmd_teclado);
    }
  }
  if (!aceito) somar(e.recusados);
}

void contarParada(Estatisticas& e, MotivoParada m) {
  switch (m) {
    case PARADA_COMANDO: somar(e.p_comando); break;
    case PARADA_OBSTACULO: somar(e.p_obstaculo); break;
    case PARADA_TEMPO_LIMITE: somar(e.p_tempo_limite); break;
    case PARADA_FIM_DO_GIRO: somar(e.p_fim_giro); break;
    case PARADA_EMERGENCIA: somar(e.p_emergencia); break;
    default: break;
  }
}

void contarObstaculo(Estatisticas& e) { somar(e.obstaculos); }

void contarEmergencia(Estatisticas& e) { somar(e.emergencias); }

unsigned int formatarEstatisticas(char* buf, unsigned int cap,
                                  unsigned long ms_ligada,
                                  const Estatisticas& e) {
  int n = FORMATAR(buf, cap,
                   "{\"t\":\"est\",\"lig\":%lu,\"mov\":%lu,\"voz\":%u,"
                   "\"app\":%u,\"tec\":%u,\"rec\":%u,\"obst\":%u,"
                   "\"emerg\":%u,\"pcmd\":%u,\"pobst\":%u,\"ptlim\":%u,"
                   "\"pgiro\":%u,\"pemerg\":%u}",
                   ms_ligada / 1000UL, e.ms_movimento / 1000UL, e.cmd_voz,
                   e.cmd_app, e.cmd_teclado, e.recusados, e.obstaculos,
                   e.emergencias, e.p_comando, e.p_obstaculo, e.p_tempo_limite,
                   e.p_fim_giro, e.p_emergencia);
  return fecharLinha(buf, cap, n);
}

// ---------------------------------------------------------------------------
// Fila de saida
// ---------------------------------------------------------------------------
FilaSaida::FilaSaida() : inicio_(0), quantidade_(0) {}

// A volta do indice e feita com comparacao, e nao com %: assim o tamanho da
// fila pode ser qualquer um. No AVR, % por um numero que nao e potencia de 2
// vira divisao em software, cara para fazer a cada byte.
bool FilaSaida::colocar(const char* s, unsigned int n) {
  if (s == NULL || n == 0 || n > livres()) return false;
  unsigned int fim = inicio_ + quantidade_;
  if (fim >= TAM_FILA_APP) fim -= TAM_FILA_APP;
  for (unsigned int i = 0; i < n; ++i) {
    buf_[fim] = s[i];
    if (++fim == TAM_FILA_APP) fim = 0;
  }
  quantidade_ += n;
  return true;
}

char FilaSaida::tirar() {
  if (quantidade_ == 0) return '\0';
  char c = buf_[inicio_];
  if (++inicio_ == TAM_FILA_APP) inicio_ = 0;
  --quantidade_;
  return c;
}
