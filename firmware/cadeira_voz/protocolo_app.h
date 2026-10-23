/*
 * protocolo_app.h - Protocolo serial entre o Mega e o ESP8266, que hospeda o
 * app do celular.
 *
 * Toda linha, nos dois sentidos, tem o formato
 *
 *   <conteudo>*HH\n
 *
 * HH e o XOR de todos os bytes do conteudo, em dois digitos hexadecimais.
 *
 *   Mega -> ESP: telemetria e eventos em JSON, por exemplo
 *                {"t":"evt","ms":120,"tipo":"inicio","voz":1}*4F
 *   ESP -> Mega: CMD:frente*7E   comando normal (bip e evento)
 *                MNT:frente*63   renovacao do "segure para andar"
 *
 * O ESP8266 imprime lixo na serial quando liga (o boot dele e a 74880 bps) e
 * um fio solto tambem gera ruido. Com checksum e casamento exato do texto, so
 * linha integra vira comando: qualquer outra coisa e descartada inteira.
 *
 * Assim como controle.h, este arquivo NAO depende da biblioteca Arduino: e
 * compilado e testado no PC (testes/test_protocolo.cpp).
 */
#ifndef PROTOCOLO_APP_H
#define PROTOCOLO_APP_H

#include <stdint.h>

#include "controle.h"

// Linha de saida para o ESP, ja contando "*HH\n" e o '\0'. A maior linha
// possivel (telemetria com todos os campos no maximo) tem 158 bytes.
const unsigned int TAM_LINHA_APP = 200;

// Linha de comando vinda do ESP. A maior valida, "CMD:esquerda*HH", tem 15.
const uint8_t TAM_LINHA_ENTRADA_APP = 32;

// Silencio na entrada que faz o leitor voltar a sincronizar sozinho. O ESP
// escreve cada linha de uma vez (16 bytes, uns 1,4 ms) e o laco do Mega le a
// serial a cada poucos ms, entao bytes da mesma linha nunca ficam tao longe
// um do outro. Ja entre o lixo do boot do ESP e o primeiro comando passa bem
// mais que isso: o Wi-Fi precisa subir e o celular conectar.
const unsigned long SILENCIO_LEITOR_APP_MS = 50;

// Fila de saida. Cabem juntos a maior telemetria, dois eventos de comando, uma
// parada e um obstaculo: mais do que uma volta do laco costuma gerar.
const unsigned int TAM_FILA_APP = 512;

// XOR de n bytes.
uint8_t checksumLinha(const char* s, unsigned int n);

// Recebe bytes um a um e monta linhas completas, sem bloquear.
class LeitorLinhas {
 public:
  LeitorLinhas();

  // Retorna true quando uma linha completa ficou disponivel em linha().
  // '\r' e ignorado. Linha grande demais ou com byte fora do ASCII imprimivel
  // e descartada inteira, ate o proximo '\n'.
  bool receber(char c);

  // Igual a receber(c), mas se o byte anterior chegou ha
  // SILENCIO_LEITOR_APP_MS ou mais, reinicia antes. Lixo sem '\n' no fim (a
  // mensagem de boot do ESP termina assim) deixaria o leitor descartando, e o
  // primeiro comando depois dele, que pode ser um parar, se perderia.
  bool receber(char c, unsigned long agora_ms);

  // Ultima linha completa, sem o '\n'. Vale ate a proxima chamada de
  // receber(). Sempre termina em '\0' dentro do buffer.
  const char* linha() const { return buf_; }

  // Joga fora a linha que esta pela metade (o resto dela sera descartado ate
  // o '\n'). Usado durante a emergencia: uma linha que comecou com o botao
  // apertado nao pode virar comando quando ele for solto.
  void descartarLinhaAtual();

  // Esquece a linha pela metade e o descarte em andamento: o proximo byte
  // comeca uma linha nova. O resto de uma linha cortada assim nunca e valido
  // sozinho, porque so o comeco da linha tem "CMD:" ou "MNT:".
  void reiniciar();

 private:
  char buf_[TAM_LINHA_ENTRADA_APP];
  uint8_t n_;
  bool descartando_;
  unsigned long t_ultimo_byte_;
};

enum TipoLinhaApp { LINHA_INVALIDA, LINHA_COMANDO, LINHA_RENOVACAO };

struct LinhaApp {
  TipoLinhaApp tipo;
  Comando cmd;  // CMD_NENHUM quando invalida
};

// Interpreta "CMD:frente*HH" / "MNT:tras*HH" (sem o '\n'). Valida checksum,
// prefixo e nome exato (minusculo, sem acento; "tras" e CMD_RE). MNT so vale
// com frente e tras; qualquer outro nome -> LINHA_INVALIDA.
LinhaApp interpretarLinhaApp(const char* linha);

// O que o Mega faz com uma linha do app. A decisao fica aqui, e nao no .ino,
// para as travas do app serem testadas no PC.
enum AcaoApp {
  ACAO_IGNORAR,  // le e joga fora: sem bip e sem evento
  ACAO_TRATAR,   // tratarComando(cmd, agora, "app"), a mesma maquina da voz
  ACAO_RECUSAR,  // dois bips e evento comando com aceito 0
  ACAO_RENOVAR   // controle.renovarComando(cmd, agora), sem bip e sem evento
};

// permitir = PERMITIR_COMANDO_APP; emergencia = botao apertado agora.
//   - linha invalida: ignora;
//   - emergencia: ignora tudo, ate parar (a cadeira ja esta parada pelo
//     botao, e nada pode ficar guardado para quando ele for solto);
//   - CMD: trata; com permitir false recusa, menos parar, sempre aceito;
//   - MNT: renova frente e tras; com permitir false ignora.
AcaoApp decidirAcaoApp(const LinhaApp& l, bool permitir, bool emergencia);

struct DadosTelemetria {
  unsigned long ms;
  Estado estado;
  int pwm_esq;
  int pwm_dir;
  int nivel;  // 1..3
  bool obstaculo;
  bool emergencia;
  MotivoParada motivo;
  bool voz;
  bool app;
};

// Cada formatar* escreve a linha COMPLETA "<json>*HH\n" em buf, termina com
// '\0' e retorna o tamanho sem o '\0'. Se nao couber, retorna 0, deixa buf
// vazio e nunca escreve alem de cap: meia linha nunca deve ser enviada.
unsigned int formatarTelemetria(char* buf, unsigned int cap,
                                const DadosTelemetria& d);
unsigned int formatarEventoInicio(char* buf, unsigned int cap,
                                  unsigned long ms, bool voz);
// origem: "voz", "teclado" ou "app". Texto com aspas, barra invertida ou
// fora do ASCII imprimivel quebraria o JSON: nesse caso retorna 0.
unsigned int formatarEventoComando(char* buf, unsigned int cap,
                                   unsigned long ms, Comando c,
                                   const char* origem, bool aceito);
unsigned int formatarEventoParada(char* buf, unsigned int cap,
                                  unsigned long ms, MotivoParada m);
unsigned int formatarEventoObstaculo(char* buf, unsigned int cap,
                                     unsigned long ms, bool ativo);
unsigned int formatarEventoEmergencia(char* buf, unsigned int cap,
                                      unsigned long ms, bool ativa);

// Contadores de uso desde que a cadeira ligou, para o resumo do app e para a
// secao de resultados do TCC. Nenhuma decisao de seguranca depende deles.
//
// Os contadores param em 65535 em vez de voltar a zero: o int do AVR tem 16
// bits, e um numero que de repente cai para zero confundiria quem le.
struct Estatisticas {
  unsigned long ms_movimento;  // tempo com alguma roda recebendo PWM
  unsigned int cmd_voz;        // comandos tratados, por origem
  unsigned int cmd_app;
  unsigned int cmd_teclado;
  unsigned int recusados;      // comandos recusados, de qualquer origem
  unsigned int obstaculos;     // vezes que o sensor passou a ver obstaculo
  unsigned int emergencias;    // vezes que o botao de emergencia foi apertado
  unsigned int p_comando;      // paradas, por motivo
  unsigned int p_obstaculo;
  unsigned int p_tempo_limite;
  unsigned int p_fim_giro;
  unsigned int p_emergencia;
};

void zerarEstatisticas(Estatisticas& e);

// origem: "voz", "app" ou "teclado".
void contarComando(Estatisticas& e, const char* origem, bool aceito);
void contarParada(Estatisticas& e, MotivoParada m);
void contarObstaculo(Estatisticas& e);
void contarEmergencia(Estatisticas& e);

// Linha completa "{"t":"est",...}*HH\n", como os formatar* acima. Tempos em
// segundos: ms_ligada e o millis() do Mega.
unsigned int formatarEstatisticas(char* buf, unsigned int cap,
                                  unsigned long ms_ligada,
                                  const Estatisticas& e);

// Fila dos bytes que ainda vao sair para o ESP.
//
// Serial.print() fica esperando quando o buffer de transmissao do Mega (64
// bytes) enche: uma telemetria de 150 bytes a 115200 bps seguraria o laco por
// uns 8 ms, com o botao de emergencia e o sensor de obstaculo sem leitura.
// Por isso as linhas entram aqui e saem aos poucos, so o que cabe no buffer
// da serial a cada volta do laco.
class FilaSaida {
 public:
  FilaSaida();

  // Coloca a linha inteira, ou nada se nao couber: meia linha so viraria lixo
  // descartado do outro lado.
  bool colocar(const char* s, unsigned int n);

  // Tira o proximo byte. Com a fila vazia retorna '\0' e nao muda nada.
  char tirar();

  unsigned int pendentes() const { return quantidade_; }
  unsigned int livres() const { return TAM_FILA_APP - quantidade_; }

 private:
  char buf_[TAM_FILA_APP];
  unsigned int inicio_;
  unsigned int quantidade_;
};

#endif  // PROTOCOLO_APP_H
