/*
 * vr3.h - Driver do Modulo de Reconhecimento de Voz V3 (Elechouse).
 *
 * O modulo conversa por serial a 9600 bps usando quadros no formato:
 *
 *   0xAA  LEN  CMD  [dados...]  0x0A
 *
 * LEN conta CMD + dados + o byte final 0x0A, ou seja, o quadro inteiro ocupa
 * LEN + 2 bytes.
 *
 * Quadro devolvido quando uma palavra e reconhecida (CMD = 0x0D):
 *
 *   indice: 0     1     2     3         4      5         6       7
 *           0xAA  0x07  0x0D  contagem  grupo  REGISTRO  indice  tam_assinatura ... 0x0A
 *
 * Usamos o byte REGISTRO (indice 5) como identificador do comando, porque e o
 * numero do registro gravado no modulo durante o treinamento.
 *
 * Resposta ao comando de carregar (CMD = 0x30), um par por registro pedido:
 *
 *   0xAA  LEN  0x30  N  R0  STA0  R1  STA1 ...  0x0A
 *
 * STA = 0x00 carregado, 0xFE registro nunca treinado, 0xFF fora da faixa,
 * 0xFD reconhecedor cheio, 0xFC ja estava no reconhecedor. O modulo responde
 * esse quadro mesmo quando nenhum registro foi treinado, por isso o status de
 * cada registro precisa ser conferido: senao o modulo "conecta" mas nunca
 * reconhece nada.
 *
 * A leitura de reconhecimento e nao bloqueante de proposito: o laco principal
 * precisa continuar girando para atender o sensor de obstaculo e o tempo
 * limite de seguranca. Ja as funcoes de configuracao e treinamento bloqueiam,
 * porque so rodam fora do modo de conducao.
 */
#ifndef VR3_H
#define VR3_H

#include <Arduino.h>

// Resultado do treinamento de um registro.
enum ResultadoTreino {
  TREINO_OK = 0,
  TREINO_SEM_RESPOSTA = -1,
  TREINO_TEMPO_ESGOTADO = -2,  // o modulo nao ouviu a voz a tempo
  TREINO_FORA_DA_FAIXA = -3,   // numero de registro invalido
  TREINO_FALHA = -4            // as duas amostras nao bateram
};

// Status de carga de um registro (resposta do comando 0x30).
enum StatusRegistro {
  REG_CARREGADO = 0x00,
  REG_JA_CARREGADO = 0xFC,
  REG_RECONHECEDOR_CHEIO = 0xFD,
  REG_NAO_TREINADO = 0xFE,
  REG_FORA_DA_FAIXA = 0xFF,
  REG_SEM_RESPOSTA = 0x01  // o modulo nao citou esse registro na resposta
};

class VR3 {
 public:
  explicit VR3(HardwareSerial& porta);

  // Procura o modulo, limpa o reconhecedor e carrega os registros indicados
  // (no maximo 7). Ao ligar, o modulo leva um tempo para responder, entao a
  // primeira velocidade e tentada varias vezes. Se ele nao responder a 9600,
  // as outras velocidades que o V3 aceita sao testadas, porque o baud fica
  // gravado no modulo e pode ter sido mudado por outro programa.
  //
  // Retorna true se o modulo respondeu e pelo menos um registro foi
  // carregado. Registros que nao carregaram (nunca treinados, por exemplo)
  // ficam em statusRegistro(). Use apenas fora da conducao.
  bool iniciar(const uint8_t* registros, uint8_t quantidade);

  // Velocidade em que o modulo respondeu, ou 0 se nao respondeu.
  long baud() const { return baud_; }

  // Quantos registros pedidos foram carregados no reconhecedor.
  uint8_t registrosCarregados() const { return carregados_; }

  // Status do i-esimo registro passado para iniciar().
  StatusRegistro statusRegistro(uint8_t i) const {
    return i < 7 ? (StatusRegistro)status_[i] : REG_SEM_RESPOSTA;
  }

  // Quantos bytes chegaram do modulo na ultima tentativa de iniciar().
  // Zero significa que nada saiu do modulo: e problema de fio ou de energia.
  // Maior que zero com falha significa que ele falou, mas em outro formato.
  uint16_t bytesRecebidos() const { return bytes_vistos_; }

  // Primeiros bytes brutos recebidos em iniciar(), para depurar a fiacao.
  const uint8_t* amostraBytes() const { return amostra_; }
  uint8_t tamanhoAmostra() const { return amostra_tam_; }

  // Grava um registro. O modulo pede a palavra duas vezes e sinaliza sozinho.
  // Bloqueia ate o modulo responder ou esgotar o tempo.
  ResultadoTreino treinar(uint8_t registro, unsigned long timeout_ms = 12000);

  // Retorna o numero do registro reconhecido, ou -1 se nada chegou ainda.
  int lerReconhecimento();

  const char* ultimoErro() const { return erro_; }

  // Ultimo quadro completo recebido, util para depurar o protocolo.
  const uint8_t* ultimoQuadro() const { return quadro_; }
  uint8_t tamanhoUltimoQuadro() const { return quadro_tam_; }

 private:
  bool enviarComando(uint8_t cmd, const uint8_t* dados, uint8_t n);

  // Le um quadro completo. Retorna o numero de bytes ou 0 se estourar o tempo.
  uint8_t receberQuadro(unsigned long timeout_ms);

  // Le quadros ate encontrar um com o comando esperado.
  bool esperarComando(uint8_t cmd_esperado, unsigned long timeout_ms);

  // Muda o baud e pergunta pelo modulo. Retorna true se ele respondeu.
  bool procurar(long baud, uint8_t tentativas);

  HardwareSerial& porta_;
  uint8_t buf_[32];      // montagem incremental (nao bloqueante)
  uint8_t idx_;
  uint8_t quadro_[32];   // ultimo quadro completo
  uint8_t quadro_tam_;
  uint16_t bytes_vistos_;  // diagnostico: bytes lidos da porta
  uint8_t amostra_[12];    // diagnostico: primeiros bytes recebidos
  uint8_t amostra_tam_;
  long baud_;
  uint8_t status_[7];
  uint8_t carregados_;
  const char* erro_;
};

const char* nomeResultadoTreino(ResultadoTreino r);
const char* nomeStatusRegistro(StatusRegistro s);

#endif  // VR3_H
