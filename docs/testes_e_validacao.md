# Testes e validação

O projeto tem duas camadas de verificação: testes automatizados que rodam no
PC e um roteiro de ensaios com o protótipo montado.

## 1. Testes automatizados

A lógica de decisão fica em `firmware/cadeira_voz/controle.cpp`, escrita sem
depender da biblioteca do Arduino. Isso permite compilar e testar no
computador, sem a placa.

```bash
make -C testes
```

O comando roda, nesta ordem:

| Programa | O que verifica |
|----------|----------------|
| `test_controle` | a máquina de estados: cada trava de segurança e a renovação usada pelo app |
| `test_protocolo` | o protocolo do lado do Mega: checksum, leitura de linhas, ruído, texto exato da telemetria e dos eventos, e o que o Mega faz com cada linha do app |
| `test_ponte` | a ponte do ESP8266: validação das linhas do Mega, fila de eventos, JSON do `/api/estado`, regras dos comandos e ordem dos pedidos |
| `test_integracao` | o código real do Mega e do ESP conversando: o que um escreve, o outro entende |
| `compila_firmware` | o firmware do Mega inteiro compila e o laço roda |
| `compila_treinador` | a ferramenta de treinamento compila e roda |
| `compila_esp` | o firmware do ESP8266 inteiro compila e cada rota HTTP responde como combinado |
| `ensaio_firmware` | o sketch inteiro do Mega com bytes chegando pela serial e o sensor e o botão simulados |
| `ensaio_so_acompanha` | o mesmo, com `PERMITIR_COMANDO_APP 0` |
| `ensaio_sem_esp` | o mesmo, com `USAR_ESP8266 0` |
| `gerar_pagina.py --verificar` | o app embutido no ESP é o mesmo de `app/index.html` |

Os substitutos das bibliotecas ficam em `testes/stub_arduino/` (Mega) e
`testes/stub_esp8266/` (ESP8266). Eles não vão para a placa: existem só para
a verificação no PC. Como o compilador real do AVR e o do ESP8266 não entram
nesse processo, a primeira gravação em cada placa continua sendo a prova
final de compilação.

### O que cada grupo de testes cobre

| Grupo | Verifica |
|-------|----------|
| 1 e 2 | a rampa de aceleração nunca dá degrau maior que o passo configurado |
| 3 | "parar" corta o PWM no mesmo instante, sem esperar a rampa |
| 4 | obstáculo para a cadeira, recusa novo avanço e continua liberando a ré |
| 5 | homem-morto: sem comando novo, a cadeira para sozinha |
| 6 | giro é pulso curto e termina sozinho |
| 7 | níveis de velocidade respeitam os limites e valem em movimento |
| 8 | ao inverter o sentido, o PWM passa por zero |
| 9 | parada de emergência corta tudo |
| 10 | o PWM nunca sai da faixa de -255 a 255 |
| 11 e 12 | a renovação do app só prolonga o mesmo movimento e nunca religa a cadeira depois de uma parada |
| 13 | segurar o botão do app mantém o movimento; parar de renovar faz a cadeira parar pelo homem-morto |
| 14 | o código do motivo de parada enviado ao app |

### Ensaios do firmware inteiro

Os ensaios (`testes/ensaio_firmware.cpp`) cobrem o que só existe no laço
principal e por isso não aparece nos testes da máquina de estados:

| Ensaio | Verifica |
|--------|----------|
| ligar | comandos que chegam enquanto o Mega liga não são executados depois; a telemetria sai a cada 250 ms com os campos na ordem combinada |
| comando do app | anda até o PWM do nível 1 nos pinos ENA e ENB, e o homem-morto para em 3 s |
| segure para andar | com renovação a cadeira anda 10 s; sem ela para 3 s depois; renovação depois da parada não religa |
| emergência | o botão corta os motores na mesma volta do laço; comandos do app e do teclado recebidos durante a emergência, e até uma linha que começou nela, não são executados ao soltar |
| obstáculo | para, avisa o app, recusa Frente e libera a Ré |
| reinício do ESP | depois do lixo de boot do ESP na serial, o primeiro "parar" é obedecido |
| ruído | 20 000 bytes aleatórios não geram nenhum comando, e o laço lê no máximo 64 bytes por volta |
| serial cheia | com o buffer de transmissão cheio, nada é escrito e o laço não trava |
| estatísticas | depois dos ensaios acima, os contadores enviados ao app batem um a um com o que aconteceu |

Cada ensaio foi conferido do jeito inverso: tirando do firmware a proteção
correspondente (o descarte na emergência, a limpeza da serial no fim do
`setup()`, a ressincronização do leitor, a recusa no modo só acompanhamento),
o ensaio falha.

### Simulador e teste de ponta a ponta

O simulador (`testes/simulador/`) roda os dois firmwares compilados para o
PC e serve o app de verdade em http://localhost:8000:

```bash
python3 testes/simulador/servidor.py
```

O teste de ponta a ponta usa o app num navegador contra o simulador e
confere, lendo a cadeira direto do ESP simulado, o que ela fez: segurar e
soltar Frente, giro, obstáculo, emergência, queda do Wi-Fi com a cadeira
andando, placa do Mega sem resposta, reinício do ESP e o modo só
acompanhamento. Precisa de Node e do Playwright:

```bash
NODE_PATH=$(npm root -g) node testes/simulador/e2e_app.js
```

Esse teste encontrou um problema real de interface. No celular, com o
alerta de obstáculo na tela, o botão Ré ficava atrás do botão PARAR, que fica
fixo na parte de baixo, justamente quando o usuário precisava dar ré. A
correção leva o direcional para logo depois dos alertas quando o controle
está liberado.

O teste 8 encontrou um defeito real durante o desenvolvimento. Com PWM 130 e
passo de rampa 8, a sequência descendente ia 10, 2, -6: o valor pulava de
positivo para negativo sem passar por zero. Na prática a ponte H invertia a
polaridade com o motor ainda girando, o que gera pico de corrente e um
tranco. A correção foi obrigar a parada em zero por pelo menos um ciclo na
inversão, no método `aproximarPorZero`.

## 2. Ensaios com o protótipo

Faça nesta ordem. Cada etapa só começa quando a anterior passa.

### Etapa 1: motores sem chassi

Suspenda o carrinho com as rodas no ar. Envie o firmware principal e use o
Monitor Serial em 115200 com as teclas `f`, `t`, `e`, `d`, `p`.

- Digite `m` para testar cada roda sozinha: esquerda para frente, esquerda
  para trás, direita para frente e direita para trás, 1,5 s cada. O Monitor
  Serial avisa qual passo está rodando, e qualquer tecla para o teste. Se
  uma roda não gira no passo dela, o problema está no fio ou no motor
  daquele lado.
- As duas rodas giram para o mesmo lado no comando `f`.
- Se uma girar ao contrário, mude `INVERTER_MOTOR_ESQ` ou
  `INVERTER_MOTOR_DIR` em `config.h`, em vez de trocar os fios.
- Se nada girar, confira se os jumpers de ENA e ENB foram retirados.

### Etapa 2: velocidade mínima

Ainda com as rodas no ar, use `-` para chegar ao nível 1 e depois apoie o
carrinho no chão. Se ele não sair do lugar, aumente `PWM_MINIMO` e o primeiro
valor de `NIVEIS_PWM` em `controle.h`, de 10 em 10, até ele andar com
firmeza. O valor certo depende do peso do protótipo montado.

### Etapa 3: andar reto

Marque uma linha de 2 m no chão e deixe a cadeira andar. Se ela puxar para um
lado, reduza o percentual da roda mais rápida em `AJUSTE_ESQ_PCT` ou
`AJUSTE_DIR_PCT`, tipicamente para 90 ou 95.

### Etapa 4: sensor de obstáculo

Com o carrinho andando devagar, ponha uma caixa de papelão no caminho.

- A cadeira precisa parar antes de encostar.
- O buzzer apita de forma intermitente enquanto o obstáculo estiver lá.
- Repetir o comando "frente" não pode fazer a cadeira andar.
- "trás" precisa continuar funcionando.

Teste também com objeto preto fosco. Provavelmente o alcance cai muito. Meça
e registre: é uma limitação legítima do sensor e rende discussão no TCC.

### Etapa 5: giro

Meça o ângulo que a cadeira gira com o valor atual de `TEMPO_GIRO_MS`.
Ajuste esse tempo até chegar perto de 90 graus. O valor depende do piso, do
peso e da carga da bateria, então repita o ensaio com a bateria em dois
estados de carga e registre a diferença.

### Etapa 6: segurança

| Ensaio | Resultado esperado |
|--------|--------------------|
| dizer "frente" e ficar em silêncio | a cadeira para sozinha em 3 s |
| dizer "parar" em movimento | para imediatamente |
| apertar o botão de emergência | para e fica parada enquanto estiver apertado |
| desligar o módulo de voz com a cadeira andando | ela para pelo tempo limite |
| ligar a alimentação com tudo montado | os motores não se movem sozinhos |

### Etapa 7: app no celular

Com as chaves DIP em 1 e 2 e o celular na rede Cadeira-TCC:

| Ensaio | Resultado esperado |
|--------|--------------------|
| abrir o app com a cadeira parada | painel mostra "Parada" e o histórico mostra "Cadeira ligada" |
| segurar Frente e soltar | anda enquanto o dedo está no botão e para ao soltar |
| tocar PARAR com o controle bloqueado | para imediatamente |
| desligar o Wi-Fi do celular com a cadeira andando pelo app | ela para sozinha em até 3 s |
| reiniciar o ESP (botão de reset ou cabo) com a cadeira andando | ela para em até 3 s e o app reconecta sozinho |
| obstáculo à frente com o app aberto | o app mostra o alerta, recusa Frente e libera Ré |
| botão de emergência com o app aberto | o app mostra EMERGÊNCIA; comandos tocados no app nesse tempo não são executados ao soltar |
| dois celulares conectados | os dois veem o mesmo estado; o PARAR de qualquer um funciona |
| firmware com `PERMITIR_COMANDO_APP 0` | o app só acompanha; o PARAR continua funcionando |
| afastar o celular da cadeira | anote a distância em que o app perde a conexão |

## 3. Tabela para os resultados do TCC

Sugestão de medições para a seção de resultados, com pelo menos 10
repetições de cada comando:

| Medida | Como medir |
|--------|-----------|
| taxa de acerto por comando | número de reconhecimentos corretos em 10 tentativas |
| taxa de acerto com ruído | mesma medição com ruído de fundo, por exemplo uma conversa próxima |
| tempo entre falar e a cadeira reagir | cronômetro ou análise de vídeo quadro a quadro |
| distância de parada com obstáculo | régua no chão, em cada nível de velocidade |
| alcance do sensor por cor | papel branco, papelão e papel preto fosco |
| erro de trajetória em 2 m | desvio lateral em relação à linha marcada |
| autonomia | tempo de operação contínua até a bateria cair |
| tempo entre tocar no app e a cadeira reagir | análise de vídeo quadro a quadro, mostrando a tela e a roda |
| alcance do Wi-Fi | distância em que o app perde a conexão, com e sem parede no caminho |

O resumo da aba Histórico do app ajuda nessas medições: depois de um ensaio,
ele mostra quantas vezes o sensor detectou obstáculo, quantas paradas houve
por motivo (comando, obstáculo, homem-morto, emergência, fim do giro), os
comandos recusados e o tempo andando, contados pelo próprio Mega. Os números
zeram quando a cadeira é desligada: anote antes de desligar.

Registre também as falhas. Um TCC de robótica que mostra onde o protótipo
falha e explica por quê vale mais do que um que só mostra o que deu certo.
