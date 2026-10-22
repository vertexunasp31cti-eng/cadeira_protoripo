#include "ponte.h"

#include <string.h>

namespace ponte {

namespace {

const char DIGITOS_HEX[] = "0123456789ABCDEF";

// Mesma ordem do enum Comando do Mega. "tras" corresponde a CMD_RE.
const char* const NOMES_COMANDO[] = {"frente", "tras",   "esquerda", "direita",
                                     "parar",  "rapido", "devagar"};
const unsigned int TOTAL_NOMES =
    sizeof(NOMES_COMANDO) / sizeof(NOMES_COMANDO[0]);

const char PREFIXO_TEL[] = "{\"t\":\"tel\",";
const char PREFIXO_EVT[] = "{\"t\":\"evt\",";
const unsigned int TAM_PREFIXO = sizeof(PREFIXO_TEL) - 1;

#define TAM_TEXTO(s) (sizeof(s) - 1)

// Pior caso do JSON de /api/estado no ESP, onde unsigned long tem 32 bits
// (10 digitos) e int com sinal ocupa ate 11 caracteres. Se alguem aumentar
// TAM_EVENTO ou MAX_EVENTOS, a compilacao avisa antes de o JSON passar a
// falhar na placa.
const unsigned int PIOR_EVENTO =
    TAM_TEXTO("{\"seq\":") + 10 + TAM_TEXTO(",\"idade_ms\":") + 10 +
    TAM_TEXTO(",\"dados\":") + (TAM_EVENTO - 1) + TAM_TEXTO("}") +
    TAM_TEXTO(",");
const unsigned int PIOR_ESTADO =
    TAM_TEXTO("{\"conectado\":false") + TAM_TEXTO(",\"idade_ms\":") + 10 +
    TAM_TEXTO(",\"tel\":") + (TAM_LINHA - 4) + TAM_TEXTO(",\"seq\":") + 10 +
    TAM_TEXTO(",\"eventos\":[") + MAX_EVENTOS * PIOR_EVENTO +
    TAM_TEXTO("],\"esp\":{\"uptime_ms\":") + 10 +
    TAM_TEXTO(",\"clientes\":") + 11 + TAM_TEXTO(",\"linhas_ok\":") + 10 +
    TAM_TEXTO(",\"linhas_erro\":") + 10 + TAM_TEXTO("}}") + 1;
static_assert(PIOR_ESTADO <= TAM_ESTADO,
              "o pior caso do /api/estado nao cabe em TAM_ESTADO");

#undef TAM_TEXTO

bool imprimivel(char c) {
  unsigned char u = (unsigned char)c;
  return u >= 0x20 && u <= 0x7E;
}

bool digito(char c) { return c >= '0' && c <= '9'; }

int valorHex(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  return -1;
}

bool alfanumerico(char c) {
  return digito(c) || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

bool nomeValido(const char* nome) {
  if (nome == 0) return false;
  for (unsigned int i = 0; i < TOTAL_NOMES; ++i) {
    if (strcmp(nome, NOMES_COMANDO[i]) == 0) return true;
  }
  return false;
}

bool idValido(const char* id) {
  unsigned int n = 0;
  for (; id[n] != '\0'; ++n) {
    if (n >= TAM_MAX_ID || !alfanumerico(id[n])) return false;
  }
  return n > 0;
}

// Escreve num buffer de tamanho fixo sem nunca passar de cap. Se algo nao
// couber, o resultado inteiro vira vazio: JSON cortado no meio seria pior do
// que nenhum, porque o app nao conseguiria ler nada.
class Escritor {
 public:
  Escritor(char* buf, unsigned int cap)
      : buf_(buf), cap_(cap), n_(0), ok_(buf != 0 && cap > 0) {}

  void bytes(const char* s, unsigned int n) {
    if (!ok_) return;
    if (n >= cap_ - n_) {  // sempre sobra lugar para o '\0'
      ok_ = false;
      return;
    }
    memcpy(buf_ + n_, s, n);
    n_ += n;
  }
  void texto(const char* s) { bytes(s, (unsigned int)strlen(s)); }
  void caractere(char c) { bytes(&c, 1); }

  void numero(unsigned long v) {
    char tmp[24];
    unsigned int i = sizeof(tmp);
    do {
      tmp[--i] = (char)('0' + v % 10);
      v /= 10;
    } while (v > 0);
    bytes(tmp + i, sizeof(tmp) - i);
  }

  void numeroComSinal(long v) {
    if (v < 0) {
      caractere('-');
      numero(0UL - (unsigned long)v);  // vale ate para o menor long
    } else {
      numero((unsigned long)v);
    }
  }

  void hex2(uint8_t v) {
    caractere(DIGITOS_HEX[v >> 4]);
    caractere(DIGITOS_HEX[v & 0x0F]);
  }

  bool ok() const { return ok_; }
  unsigned int tamanho() const { return n_; }

  unsigned int fechar() {
    if (!ok_) {
      if (buf_ != 0 && cap_ > 0) buf_[0] = '\0';
      return 0;
    }
    buf_[n_] = '\0';
    return n_;
  }

 private:
  char* buf_;
  unsigned int cap_;
  unsigned int n_;
  bool ok_;
};

// --- Validacao de JSON plano ------------------------------------------------
// So o necessario para as linhas do Mega: um objeto, sem objeto ou lista
// dentro. Cada funcao avanca *i e retorna false no primeiro erro.

void pularEspacos(const char* s, unsigned int n, unsigned int* i) {
  while (*i < n &&
         (s[*i] == ' ' || s[*i] == '\t' || s[*i] == '\n' || s[*i] == '\r')) {
    ++*i;
  }
}

bool lerTextoJson(const char* s, unsigned int n, unsigned int* i) {
  if (*i >= n || s[*i] != '"') return false;
  ++*i;
  while (*i < n) {
    char c = s[(*i)++];
    if (c == '"') return true;
    // Controle e proibido dentro de texto JSON. Acima de 0x7E tambem
    // recusamos: o protocolo e so ASCII, e assim nao ha UTF-8 quebrado.
    if (!imprimivel(c)) return false;
    if (c == '\\') {
      if (*i >= n) return false;
      char e = s[(*i)++];
      if (e == 'u') {
        for (int k = 0; k < 4; ++k) {
          if (*i >= n || valorHex(s[*i]) < 0) return false;
          ++*i;
        }
      } else if (e == '\0' || strchr("\"\\/bfnrt", e) == 0) {
        return false;
      }
    }
  }
  return false;
}

bool lerNumeroJson(const char* s, unsigned int n, unsigned int* i) {
  unsigned int p = *i;
  if (p < n && s[p] == '-') ++p;
  if (p >= n || !digito(s[p])) return false;
  if (s[p] == '0') {
    ++p;  // JSON nao aceita zero a esquerda
  } else {
    while (p < n && digito(s[p])) ++p;
  }
  if (p < n && s[p] == '.') {
    ++p;
    if (p >= n || !digito(s[p])) return false;
    while (p < n && digito(s[p])) ++p;
  }
  if (p < n && (s[p] == 'e' || s[p] == 'E')) {
    ++p;
    if (p < n && (s[p] == '+' || s[p] == '-')) ++p;
    if (p >= n || !digito(s[p])) return false;
    while (p < n && digito(s[p])) ++p;
  }
  *i = p;
  return true;
}

bool lerLiteralJson(const char* s, unsigned int n, unsigned int* i,
                    const char* literal) {
  unsigned int k = (unsigned int)strlen(literal);
  if (n - *i < k || memcmp(s + *i, literal, k) != 0) return false;
  *i += k;
  return true;
}

bool lerValorJson(const char* s, unsigned int n, unsigned int* i) {
  if (*i >= n) return false;
  switch (s[*i]) {
    case '"': return lerTextoJson(s, n, i);
    case 't': return lerLiteralJson(s, n, i, "true");
    case 'f': return lerLiteralJson(s, n, i, "false");
    case 'n': return lerLiteralJson(s, n, i, "null");
    default: return lerNumeroJson(s, n, i);
  }
}

}  // namespace

// ---------------------------------------------------------------------------
uint8_t checksumXor(const char* s, unsigned int n) {
  uint8_t x = 0;
  for (unsigned int i = 0; i < n; ++i) x ^= (uint8_t)s[i];
  return x;
}

bool objetoJsonPlano(const char* s, unsigned int n) {
  if (s == 0) return false;
  unsigned int i = 0;
  pularEspacos(s, n, &i);
  if (i >= n || s[i] != '{') return false;
  ++i;
  pularEspacos(s, n, &i);
  if (i < n && s[i] == '}') {
    ++i;
    pularEspacos(s, n, &i);
    return i == n;
  }
  for (;;) {
    pularEspacos(s, n, &i);
    if (!lerTextoJson(s, n, &i)) return false;
    pularEspacos(s, n, &i);
    if (i >= n || s[i] != ':') return false;
    ++i;
    pularEspacos(s, n, &i);
    if (!lerValorJson(s, n, &i)) return false;
    pularEspacos(s, n, &i);
    if (i >= n) return false;
    if (s[i] == ',') {
      ++i;
      continue;
    }
    if (s[i] != '}') return false;
    ++i;
    pularEspacos(s, n, &i);
    return i == n;  // nada depois do fecha-chaves
  }
}

TipoLinhaMega validarLinhaMega(const char* linha, unsigned int n,
                               unsigned int* tam_conteudo) {
  if (tam_conteudo != 0) *tam_conteudo = 0;
  // Menor linha possivel: prefixo + "}" + "*HH".
  if (linha == 0 || n < TAM_PREFIXO + 1 + 3) return LINHA_RUIM;

  for (unsigned int i = 0; i < n; ++i) {
    if (!imprimivel(linha[i])) return LINHA_RUIM;
  }

  // O checksum vem depois do ULTIMO '*', com exatamente dois digitos.
  unsigned int fim = n - 3;
  if (linha[fim] != '*') return LINHA_RUIM;
  int alto = valorHex(linha[fim + 1]);
  int baixo = valorHex(linha[fim + 2]);
  if (alto < 0 || baixo < 0) return LINHA_RUIM;
  if (checksumXor(linha, fim) != (uint8_t)(alto * 16 + baixo)) {
    return LINHA_RUIM;
  }

  TipoLinhaMega tipo;
  if (memcmp(linha, PREFIXO_TEL, TAM_PREFIXO) == 0) {
    tipo = LINHA_TELEMETRIA;
  } else if (memcmp(linha, PREFIXO_EVT, TAM_PREFIXO) == 0) {
    tipo = LINHA_EVENTO;
  } else {
    return LINHA_RUIM;
  }
  if (linha[fim - 1] != '}') return LINHA_RUIM;
  if (!objetoJsonPlano(linha, fim)) return LINHA_RUIM;

  if (tam_conteudo != 0) *tam_conteudo = fim;
  return tipo;
}

bool lerInteiro(const char* s, unsigned long limite, unsigned long* valor) {
  if (s == 0 || s[0] == '\0') return false;
  unsigned long v = 0;
  for (const char* p = s; *p != '\0'; ++p) {
    if (!digito(*p)) return false;
    unsigned long d = (unsigned long)(*p - '0');
    if (v > limite / 10 || (v == limite / 10 && d > limite % 10)) {
      return false;
    }
    v = v * 10 + d;
  }
  *valor = v;
  return true;
}

unsigned long interpretarDesde(const char* s) {
  unsigned long v = 0;
  return lerInteiro(s, 0xFFFFFFFFUL, &v) ? v : 0;
}

long interpretarN(const char* s) {
  // Limite de long com 32 bits, que e o tamanho no ESP8266.
  unsigned long v = 0;
  return lerInteiro(s, 2147483647UL, &v) ? (long)v : -1;
}

bool interpretarManter(const char* s, bool* manter) {
  if (s == 0 || s[0] == '\0' || strcmp(s, "0") == 0) {
    *manter = false;
    return true;
  }
  if (strcmp(s, "1") == 0) {
    *manter = true;
    return true;
  }
  // Qualquer outra coisa e erro, e nao "sem manter": um valor mal escrito
  // nunca pode transformar uma renovacao em comando novo de movimento.
  return false;
}

// ---------------------------------------------------------------------------
LeitorSerial::LeitorSerial() { limpar(); }

void LeitorSerial::limpar() {
  buf_[0] = '\0';
  n_ = 0;
  descartando_ = false;
  pronta_ = false;
  descartadas_ = 0;
}

bool LeitorSerial::receber(char c) {
  if (pronta_) {  // a linha anterior ja foi entregue
    pronta_ = false;
    n_ = 0;
    buf_[0] = '\0';
  }
  if (c == '\r') return false;
  if (c == '\n') {
    if (descartando_) {
      descartando_ = false;
      n_ = 0;
      return false;
    }
    if (n_ == 0) return false;
    buf_[n_] = '\0';
    pronta_ = true;
    return true;
  }
  if (descartando_) return false;
  if (n_ >= TAM_LINHA - 1) {
    descartando_ = true;
    ++descartadas_;
    n_ = 0;
    buf_[0] = '\0';
    return false;
  }
  buf_[n_++] = c;
  return false;
}

// ---------------------------------------------------------------------------
PonteCadeira::PonteCadeira() { iniciar(); }

void PonteCadeira::iniciar() {
  leitor_.limpar();
  tel_[0] = '\0';
  tem_tel_ = false;
  t_tel_ = 0;
  tem_linha_ = false;
  t_linha_ = 0;
  ev_inicio_ = 0;
  ev_qtd_ = 0;
  seq_ = 0;
  linhas_ok_ = 0;
  linhas_erro_ = 0;
  for (unsigned int i = 0; i < MAX_IDS; ++i) {
    ids_[i].id[0] = '\0';
    ids_[i].n = -1;
    ids_[i].uso = 0;
  }
  uso_ = 0;
}

void PonteCadeira::receberByte(char c, unsigned long agora_ms) {
  if (leitor_.receber(c)) {
    tratarLinha(leitor_.linha(), leitor_.tamanho(), agora_ms);
  }
}

void PonteCadeira::tratarLinha(const char* linha, unsigned int n,
                               unsigned long agora_ms) {
  unsigned int tam = 0;
  TipoLinhaMega tipo = validarLinhaMega(linha, n, &tam);
  if (tipo == LINHA_RUIM || (tipo == LINHA_EVENTO && tam >= TAM_EVENTO)) {
    ++linhas_erro_;
    return;
  }

  if (tipo == LINHA_TELEMETRIA) {
    memcpy(tel_, linha, tam);
    tel_[tam] = '\0';
    tem_tel_ = true;
    t_tel_ = agora_ms;
  } else {
    guardarEvento(linha, tam, agora_ms);
  }
  ++linhas_ok_;
  tem_linha_ = true;
  t_linha_ = agora_ms;
}

void PonteCadeira::guardarEvento(const char* dados, unsigned int n,
                                 unsigned long agora_ms) {
  unsigned int pos;
  if (ev_qtd_ < MAX_EVENTOS) {
    pos = (ev_inicio_ + ev_qtd_) % MAX_EVENTOS;
    ++ev_qtd_;
  } else {
    pos = ev_inicio_;  // fila cheia: o mais antigo sai
    ev_inicio_ = (ev_inicio_ + 1) % MAX_EVENTOS;
  }
  Evento& ev = eventos_[pos];
  ev.seq = ++seq_;
  ev.chegada_ms = agora_ms;
  memcpy(ev.dados, dados, n);
  ev.dados[n] = '\0';
}

bool PonteCadeira::conectado(unsigned long agora_ms) const {
  // Subtracao sem sinal: continua certa quando o millis() da a volta.
  return tem_linha_ && (agora_ms - t_linha_) < TEMPO_MEGA_OFFLINE_MS;
}

unsigned int PonteCadeira::montarEstado(char* buf, unsigned int cap,
                                        unsigned long agora_ms,
                                        unsigned long desde,
                                        int clientes) const {
  Escritor e(buf, cap);

  e.texto("{\"conectado\":");
  e.texto(conectado(agora_ms) ? "true" : "false");
  e.texto(",\"idade_ms\":");
  if (tem_tel_) {
    e.numero(agora_ms - t_tel_);
  } else {
    e.texto("null");
  }
  // A telemetria e os eventos entram sem mudanca: so foram guardados depois
  // de validados como objeto JSON (validarLinhaMega).
  e.texto(",\"tel\":");
  e.texto(tem_tel_ ? tel_ : "null");
  e.texto(",\"seq\":");
  e.numero(seq_);

  // desde maior que a contagem atual quer dizer que o ESP reiniciou e o app
  // ainda lembra a contagem antiga: manda tudo de novo.
  if (desde > seq_) desde = 0;
  e.texto(",\"eventos\":[");
  bool primeiro = true;
  for (unsigned int k = 0; k < ev_qtd_; ++k) {
    const Evento& ev = eventos_[(ev_inicio_ + k) % MAX_EVENTOS];
    if (ev.seq <= desde) continue;
    if (!primeiro) e.caractere(',');
    primeiro = false;
    e.texto("{\"seq\":");
    e.numero(ev.seq);
    e.texto(",\"idade_ms\":");
    e.numero(agora_ms - ev.chegada_ms);
    e.texto(",\"dados\":");
    e.texto(ev.dados);
    e.caractere('}');
  }

  e.texto("],\"esp\":{\"uptime_ms\":");
  e.numero(agora_ms);
  e.texto(",\"clientes\":");
  e.numeroComSinal(clientes);
  e.texto(",\"linhas_ok\":");
  e.numero(linhas_ok_);
  e.texto(",\"linhas_erro\":");
  e.numero(linhasErro());
  e.texto("}}");
  return e.fechar();
}

int PonteCadeira::acharId(const char* id) const {
  for (unsigned int i = 0; i < MAX_IDS; ++i) {
    if (ids_[i].id[0] != '\0' && strcmp(ids_[i].id, id) == 0) return (int)i;
  }
  return -1;
}

void PonteCadeira::registrarN(const char* id, long n) {
  int k = acharId(id);
  if (k < 0) {
    // Posicao livre ou, sem nenhuma, o id usado ha mais tempo.
    k = 0;
    for (unsigned int i = 0; i < MAX_IDS; ++i) {
      if (ids_[i].id[0] == '\0') {
        k = (int)i;
        break;
      }
      if (ids_[i].uso < ids_[k].uso) k = (int)i;
    }
    strcpy(ids_[k].id, id);  // idValido() ja garantiu o tamanho
    ids_[k].n = n;
  } else if (n > ids_[k].n) {
    ids_[k].n = n;  // um parar atrasado nao faz a contagem voltar
  }
  ids_[k].uso = ++uso_;
}

ResultadoComando PonteCadeira::prepararComando(const char* nome, bool manter,
                                               const char* id, long n,
                                               unsigned long agora_ms,
                                               char* linha, unsigned int cap) {
  if (linha != 0 && cap > 0) linha[0] = '\0';

  if (!nomeValido(nome)) return CMD_INVALIDO;
  // Giro e pulso curto de proposito, e parar/velocidade nao tem o que
  // renovar: so frente e tras aceitam "segure para andar".
  if (manter && strcmp(nome, "frente") != 0 && strcmp(nome, "tras") != 0) {
    return CMD_INVALIDO;
  }
  // Parar nunca e bloqueado, nem sem conexao, nem fora de ordem, nem com id
  // estranho: um parar a mais e sempre seguro, um parar perdido nao.
  bool parar = strcmp(nome, "parar") == 0;
  bool com_id = id != 0 && id[0] != '\0';
  if (com_id && !idValido(id)) {
    if (!parar) return CMD_INVALIDO;
    com_id = false;  // parar sai, so fica sem ordenacao
  }
  bool ordenar = com_id && n >= 0;

  if (!parar) {
    if (ordenar) {
      int k = acharId(id);
      if (k >= 0 && n <= ids_[k].n) return CMD_ATRASADO;
    }
    if (!conectado(agora_ms)) return CMD_SEM_CONEXAO;
  }

  // O '\n' do comeco (ADENDO 2.3 do ESPEC) fecha qualquer lixo que ficou na
  // Serial do Mega sem fim de linha, como o boot do ESP a 74880 bps. Sem ele
  // o Mega juntaria o lixo ao primeiro comando depois de um reinicio do ESP e
  // descartaria os dois, mesmo um parar. O Mega ignora a linha vazia.
  Escritor e(linha, cap);
  e.caractere('\n');
  e.texto(manter ? "MNT:" : "CMD:");
  e.texto(nome);
  // O checksum e so do conteudo, sem esse '\n'.
  uint8_t soma = e.ok() ? checksumXor(linha + 1, e.tamanho() - 1) : 0;
  e.caractere('*');
  e.hex2(soma);
  e.caractere('\n');
  if (e.fechar() == 0) return CMD_INVALIDO;

  // So depois de saber que a linha vai sair: comando recusado nao conta.
  if (ordenar) registrarN(id, n);
  return CMD_OK;
}

}  // namespace ponte
