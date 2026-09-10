# Cadeira de rodas com comando de voz

Protótipo de TCC de robótica: uma cadeira de rodas em escala reduzida que se
movimenta por comandos de voz, com travas de segurança e detecção de
obstáculo.

O reconhecimento é feito no próprio módulo embarcado, sem internet e sem
servidor.

## Componentes

- Placa compatível Arduino Mega 2560 + ESP8266 (CH340)
- Módulo de Reconhecimento de Voz V3
- Ponte H dupla L298N
- Sensor de obstáculo reflexivo infravermelho
- 2 motores DC 3-6 V com roda de 68 mm
- Buzzer ativo, LED e botão de emergência

## Comandos de voz

| Registro | Palavra | O que faz |
|----------|---------|-----------|
| 0 | frente | anda para frente até parar |
| 1 | trás | anda para trás até parar |
| 2 | esquerda | gira à esquerda por um tempo curto |
| 3 | direita | gira à direita por um tempo curto |
| 4 | parar | para imediatamente |
| 5 | rápido | sobe um nível de velocidade |
| 6 | devagar | desce um nível de velocidade |

São sete porque o módulo V3 reconhece no máximo sete registros ao mesmo
tempo, embora guarde até 80 na memória.

## Segurança

O protótipo tem oito travas independentes. Nenhuma depende de outra
funcionar.

1. O sensor de obstáculo para o avanço e recusa novos comandos de frente.
2. Homem-morto: sem comando novo em 3 segundos, a cadeira para sozinha.
3. "Parar" corta o PWM no mesmo instante, sem passar pela rampa.
4. Botão físico de emergência, com prioridade sobre qualquer comando.
5. Rampa de aceleração, que limita a variação de PWM por ciclo.
6. Inversão de sentido obrigada a passar por zero, para não inverter a ponte
   H com o motor girando.
7. Os motores são desligados antes de qualquer outra inicialização, para a
   cadeira nunca partir sozinha ao ligar.
8. A ré continua liberada com obstáculo à frente, porque o sensor só olha
   para frente e travar a ré prenderia o usuário.

Toda trava tem teste automatizado correspondente.

## Estrutura do repositório

```
firmware/cadeira_voz/     firmware principal
  cadeira_voz.ino           laço principal e integração
  config.h                  mapa de pinos e opções
  controle.h / .cpp         máquina de estados, sem dependência do Arduino
  motores.h / .cpp          ponte H L298N
  sensor_obstaculo.h/.cpp   sensor IR com filtro de ruído
  vr3.h / .cpp              driver do módulo de voz

firmware/treinar_voz/     ferramenta de gravação dos comandos
testes/                   testes que rodam no PC, sem hardware
docs/                     ligações, treinamento, ensaios e apoio ao TCC
```

## Como usar

### 1. Montar

Siga `docs/ligacoes.md`. Preste atenção em três pontos que costumam dar
errado: o TX do módulo de voz vai no RX do Arduino, os jumpers de ENA e ENB
do L298N precisam ser retirados, e os dois circuitos precisam de terra
comum.

### 2. Treinar os comandos

Envie `firmware/treinar_voz/treinar_voz.ino`, abra o Monitor Serial em
115200 e siga o menu. O passo a passo está em `docs/treinamento_vr3.md`.

O treinamento fica gravado no módulo e não precisa ser repetido a cada uso.

### 3. Enviar o firmware

Abra `firmware/cadeira_voz/cadeira_voz.ino` no Arduino IDE, selecione a
placa **Arduino Mega 2560** e envie. Nenhuma biblioteca externa é
necessária.

O Monitor Serial em 115200 mostra a telemetria: estado atual, PWM de cada
roda, nível de velocidade, situação do sensor e motivo da última parada.

### 4. Testar sem falar

Com `MODO_SIMULACAO` ligado, que é o padrão, os comandos também aceitam
teclado pelo Monitor Serial:

| Tecla | Comando |
|-------|---------|
| `f` | frente |
| `t` | trás |
| `e` | esquerda |
| `d` | direita |
| `p` ou espaço | parar |
| `+` | mais rápido |
| `-` | mais devagar |

Isso permite ajustar a mecânica antes de treinar a voz, e serve de plano B na
apresentação se o ambiente estiver barulhento demais.

## Testes automatizados

A lógica de decisão não depende da biblioteca do Arduino, então roda no
computador:

```bash
make -C testes
```

Isso executa os 40 testes da máquina de estados e depois compila o firmware
inteiro e a ferramenta de treinamento usando um substituto da API do
Arduino, o que pega erro de compilação antes de abrir o IDE.

Um desses testes encontrou um defeito real durante o desenvolvimento: na
desaceleração, o PWM pulava de +2 para -6 sem passar por zero, invertendo a
ponte H com o motor ainda girando. Está descrito em
`docs/testes_e_validacao.md`.

## Ajustes comuns

Todos os parâmetros ficam em dois arquivos.

Em `firmware/cadeira_voz/config.h`:

| Parâmetro | Para quê |
|-----------|----------|
| `INVERTER_MOTOR_ESQ` / `_DIR` | corrigir roda girando ao contrário sem trocar fio |
| `NIVEL_IR_DETECTADO` | adaptar a sensor que responde em nível alto |
| `MODO_SIMULACAO` | ligar ou desligar o controle por teclado |
| `USAR_ESP8266` | publicar telemetria em JSON na Serial3 |

Em `firmware/cadeira_voz/controle.h`:

| Parâmetro | Para quê |
|-----------|----------|
| `PWM_MINIMO` | menor PWM que faz o motor girar com carga |
| `NIVEIS_PWM` | os três níveis de velocidade |
| `TEMPO_LIMITE_COMANDO_MS` | tempo do homem-morto |
| `TEMPO_GIRO_MS` | duração do giro, ajuste até dar cerca de 90 graus |
| `AJUSTE_ESQ_PCT` / `_DIR_PCT` | corrigir a cadeira que puxa para um lado |
| `RAMPA_PASSO` | suavidade da partida |

## Limitações

Este é um protótipo em escala reduzida, feito para demonstrar o princípio de
controle. Ele não transporta pessoa. O reconhecimento depende da voz de quem
treinou, o vocabulário é de sete palavras e o sensor infravermelho enxerga
mal superfícies pretas foscas e vidro, além de não detectar degraus. A lista
completa está em `docs/tcc_apoio.md`.

## Documentação

- `docs/ligacoes.md` - fiação, alimentação e diagrama de blocos
- `docs/treinamento_vr3.md` - gravação dos comandos e protocolo do módulo
- `docs/testes_e_validacao.md` - testes no PC e roteiro de ensaios
- `docs/tcc_apoio.md` - objetivos, metodologia, limitações e trabalhos futuros
