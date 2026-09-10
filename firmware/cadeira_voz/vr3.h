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

class VR3 {
 public:
  explicit VR3(Stream& porta);

  // Limpa o reconhecedor e carrega os registros indicados (no maximo 7).
  // Retorna false se o modulo nao responder. Use apenas fora da conducao.
  bool iniciar(const uint8_t* registros, uint8_t quantidade);

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

  Stream& porta_;
  uint8_t buf_[32];      // montagem incremental (nao bloqueante)
  uint8_t idx_;
  uint8_t quadro_[32];   // ultimo quadro completo
  uint8_t quadro_tam_;
  const char* erro_;
};

const char* nomeResultadoTreino(ResultadoTreino r);

#endif  // VR3_H
