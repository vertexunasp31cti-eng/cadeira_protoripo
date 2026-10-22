/*
 * ponte.h - Ponte entre o Mega (serial) e o app no celular (HTTP), no ESP8266.
 *
 * Este arquivo NAO depende da biblioteca Arduino de proposito, como o
 * controle.h do Mega: validacao das linhas do Mega, fila de eventos, JSON da
 * API e regras dos comandos do app ficam aqui e sao testados no PC.
 * Veja testes/test_ponte.cpp. O esp_wifi.ino so liga isto ao hardware.
 *
 * Tudo fica dentro de namespace ponte para poder ser ligado no mesmo binario
 * de teste que o protocolo_app do Mega sem conflito de nomes.
 *
 * O ESP nao decide nada sobre movimento. Ele so repassa comandos; todas as
 * travas de seguranca continuam no Mega.
 */
#ifndef PONTE_H
#define PONTE_H

#include <stdint.h>

#include "config_wifi.h"

namespace ponte {

// Maior linha aceita do Mega, contando o '\0'. Linha maior e descartada
// inteira, ate o proximo '\n'.
const unsigned int TAM_LINHA = 256;

// Quantos eventos o ESP lembra para o app que ficou um tempo sem consultar.
const unsigned int MAX_EVENTOS = 20;

// Maior evento guardado, contando o '\0'. Os eventos do Mega tem menos de 100
// caracteres. O limite existe para o pior caso do /api/estado (20 eventos
// enormes) ainda caber em TAM_ESTADO; evento maior conta como linha com erro.
const unsigned int TAM_EVENTO = 128;

// Ordenacao de comandos por cliente.
const unsigned int MAX_IDS = 8;
const unsigned int TAM_MAX_ID = 16;

// Buffer do JSON de GET /api/estado (estatico, no esp_wifi.ino).
const unsigned int TAM_ESTADO = 4096;

// Buffer da linha de comando para o Mega. A maior e "\nCMD:esquerda*HH\n",
// 17 bytes mais o '\0'.
const unsigned int TAM_COMANDO = 24;

// XOR de n bytes. E o checksum "*HH" das linhas nos dois sentidos.
uint8_t checksumXor(const char* s, unsigned int n);

// Recebe bytes um a um e monta linhas completas. Ignora '\r' e linha vazia.
// Linha maior que o buffer e descartada ate o proximo '\n', para o resto dela
// nunca ser lido como se fosse uma linha nova.
class LeitorSerial {
 public:
  LeitorSerial();
  void limpar();

  // Retorna true quando uma linha completa (sem o '\n') ficou em linha().
  // A linha vale ate a proxima chamada de receber().
  bool receber(char c);
  const char* linha() const { return buf_; }
  unsigned int tamanho() const { return n_; }

  // Linhas jogadas fora por serem grandes demais.
  unsigned long descartadas() const { return descartadas_; }

 private:
  char buf_[TAM_LINHA];
  unsigned int n_;
  bool descartando_;
  bool pronta_;
  unsigned long descartadas_;
};

enum TipoLinhaMega { LINHA_RUIM, LINHA_TELEMETRIA, LINHA_EVENTO };

// Valida uma linha do Mega (n bytes, sem o '\n'): so ASCII imprimivel,
// "*HH" no fim com o checksum certo (hex maiusculo ou minusculo), conteudo
// comecando com {"t":"tel", ou {"t":"evt", e terminando em }. Em
// *tam_conteudo devolve o tamanho do JSON, sem o "*HH".
//
// Alem do contrato, exige que o conteudo seja um objeto JSON plano valido.
// O conteudo e copiado sem mudanca para dentro do JSON do /api/estado, e um
// unico objeto quebrado faria o app falhar em toda consulta enquanto ele
// estivesse guardado. O checksum de 8 bits deixa passar, por exemplo, dois
// bytes trocados com o mesmo bit errado.
TipoLinhaMega validarLinhaMega(const char* linha, unsigned int n,
                               unsigned int* tam_conteudo);

// true se s[0..n) e um objeto JSON sem aninhamento: {"chave":valor,...} com
// valores texto, numero, true, false ou null.
bool objetoJsonPlano(const char* s, unsigned int n);

// Inteiro decimal sem sinal, so digitos. false se vazio, com outro caractere
// ou maior que limite.
bool lerInteiro(const char* s, unsigned long limite, unsigned long* valor);

// Parametros da API, ja com as regras do contrato.
unsigned long interpretarDesde(const char* s);       // invalido ou ausente -> 0
long interpretarN(const char* s);                    // invalido ou ausente -> -1
bool interpretarManter(const char* s, bool* manter);  // so "", "0" ou "1"

enum ResultadoComando { CMD_OK, CMD_INVALIDO, CMD_SEM_CONEXAO, CMD_ATRASADO };

class PonteCadeira {
 public:
  PonteCadeira();

  // Esquece tudo: telemetria, eventos, contadores e ids de cliente.
  void iniciar();

  // Alimenta um byte vindo do Mega. Linhas validas atualizam a telemetria ou
  // entram na fila de eventos.
  void receberByte(char c, unsigned long agora_ms);

  // Linha valida do Mega ha menos de TEMPO_MEGA_OFFLINE_MS.
  bool conectado(unsigned long agora_ms) const;

  // Monta o JSON de GET /api/estado em buf, terminado em '\0'. Retorna o
  // tamanho sem o '\0', ou 0 se nao coube (buf fica vazio). Nunca escreve
  // alem de cap.
  unsigned int montarEstado(char* buf, unsigned int cap, unsigned long agora_ms,
                            unsigned long desde, int clientes) const;

  // Valida e prepara a linha "\nCMD:..*HH\n" / "\nMNT:..*HH\n" para escrever
  // na Serial (o '\n' do comeco nao entra no checksum; ADENDO 2.3 do ESPEC).
  // id/n: ordenacao por cliente; id vazio ou nulo (ou n < 0) = sem
  // ordenacao. Com CMD_OK a linha completa fica em linha; em qualquer outro
  // resultado linha fica vazia e nada deve ser enviado. id fora do formato
  // (1 a 16 caracteres [A-Za-z0-9]) e cap pequeno demais dao CMD_INVALIDO.
  // "parar" nunca e bloqueado: sai sem conexao, fora de ordem e tambem com
  // id fora do formato (nesse caso sem ordenacao).
  ResultadoComando prepararComando(const char* nome, bool manter,
                                   const char* id, long n,
                                   unsigned long agora_ms, char* linha,
                                   unsigned int cap);

  // Diagnostico.
  unsigned long linhasOk() const { return linhas_ok_; }
  unsigned long linhasErro() const {
    return linhas_erro_ + leitor_.descartadas();
  }
  unsigned long seq() const { return seq_; }
  unsigned int eventosGuardados() const { return ev_qtd_; }
  bool temTelemetria() const { return tem_tel_; }

 private:
  struct Evento {
    unsigned long seq;
    unsigned long chegada_ms;
    char dados[TAM_EVENTO];
  };

  struct UltimoN {
    char id[TAM_MAX_ID + 1];  // "" = posicao livre
    long n;
    unsigned long uso;  // para achar o mais antigo na hora de substituir
  };

  void tratarLinha(const char* linha, unsigned int n, unsigned long agora_ms);
  void guardarEvento(const char* dados, unsigned int n, unsigned long agora_ms);
  int acharId(const char* id) const;
  void registrarN(const char* id, long n);

  LeitorSerial leitor_;

  char tel_[TAM_LINHA];
  bool tem_tel_;
  unsigned long t_tel_;

  bool tem_linha_;
  unsigned long t_linha_;

  Evento eventos_[MAX_EVENTOS];
  unsigned int ev_inicio_;  // posicao do mais antigo
  unsigned int ev_qtd_;
  unsigned long seq_;       // seq do ultimo evento recebido (0 = nenhum)

  unsigned long linhas_ok_;
  unsigned long linhas_erro_;

  UltimoN ids_[MAX_IDS];
  unsigned long uso_;
};

}  // namespace ponte

#endif  // PONTE_H
