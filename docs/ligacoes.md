# Ligações do protótipo

Todos os números de pino também estão em `firmware/cadeira_voz/config.h`.
Se você mudar a montagem, mude lá e não espalhe números pelo código.

## 1. Ponte H dupla L298N

| L298N | Arduino Mega | Observação |
|-------|--------------|-----------|
| ENA   | 5            | PWM, velocidade do motor esquerdo |
| IN1   | 7            | sentido do motor esquerdo |
| IN2   | 8            | sentido do motor esquerdo |
| IN3   | 9            | sentido do motor direito |
| IN4   | 10           | sentido do motor direito |
| ENB   | 6            | PWM, velocidade do motor direito |
| GND   | GND          | terra comum, obrigatório |
| OUT1/OUT2 | motor esquerdo | |
| OUT3/OUT4 | motor direito  | |

Os pinos ENA e ENB controlam a velocidade, e há duas montagens possíveis.
Use uma ou a outra, nunca as duas juntas:

- **Jumpers colocados, nada nos pinos 5 e 6.** Os motores andam sempre na
  velocidade máxima, mas frente, ré, giro e parada funcionam. É a montagem
  mais simples para o primeiro teste.
- **Jumpers retirados, fio 5→ENA e 6→ENB.** Com isso funcionam o controle de
  velocidade (`rápido` e `devagar`) e a rampa de partida.

Cada jumper fica sobre **dois** pinos. Um deles é o ENA (ou ENB) de
verdade, que vai para o chip. O outro é uma saída de +5 V do próprio L298N.
O fio do Arduino tem que ir no pino ENA/ENB. Se ele cair no pino de +5 V, a
saída do Arduino fica em curto com o regulador do L298N e o pino 5 ou 6 pode
queimar. A posição muda de um módulo para outro, então descubra com o
multímetro em vez de chutar: tudo desligado (sem bateria e sem USB),
multímetro em continuidade, uma ponta no parafuso do borne 5V do L298N e a
outra em cada pino do cabeçote. O que apitar é o +5 V. O outro é o ENA.

Nunca recoloque o jumper com o fio do Arduino ainda ligado no cabeçote: o
jumper liga o ENA ao +5 V, e o pino do Arduino fica em curto do mesmo jeito.
Para testar se o problema é o fio, tire o fio do pino 5 ou 6, recoloque o
jumper e digite `m`. Se assim a roda gira e com o fio não, o fio está no
pino errado do cabeçote, solto ou partido.

Os fios de sentido (IN1 a IN4) ficam nos pinos 7, 8, 9 e 10, na fileira
simples de cima do Mega, perto do 5 e do 6. Antes eles ficavam nos pinos 32
a 35, no conector duplo do fim da placa, onde é fácil encaixar um fio um furo
para o lado. Foi o que aconteceu na montagem: só um dos quatro estava no
lugar certo.

Esses quatro pinos só definem o sentido de giro, com nível alto ou baixo.
Quem controla a velocidade é o par ENA e ENB, e esses dois precisam de pino
PWM.

Por que os pinos 5 e 6 para ENA e ENB: no Mega, os pinos 4 e 13 usam o
Timer0, o mesmo timer da função `millis()`. Usar PWM neles atrapalha a base
de tempo, e toda a segurança do projeto depende de `millis()`.

## 2. Módulo de Reconhecimento de Voz V3

| Módulo V3 | Arduino Mega | Observação |
|-----------|--------------|-----------|
| VCC | 5V  | consome pouco, pode sair da própria placa |
| GND | GND | |
| TX  | 19 (RX1) | **cruzado**: TX do módulo no RX do Arduino |
| RX  | 18 (TX1) | **cruzado**: RX do módulo no TX do Arduino |

O erro mais comum de montagem é ligar TX no TX. O sintoma é a mensagem
`nenhum byte chegou do modulo` no Monitor Serial. Veja a tabela de
diagnóstico em `docs/treinamento_vr3.md`.

O módulo sai de fábrica em 9600 bps. Se o baud tiver sido mudado por outro
programa, o firmware encontra sozinho (testa 9600, 19200, 38400, 4800 e
2400) e informa a velocidade no Monitor Serial. Ele guarda até 80 registros gravados,
mas só reconhece 7 ao mesmo tempo, e é por isso que o projeto usa exatamente
7 comandos.

## 3. Sensor de obstáculo infravermelho reflexivo

| Sensor | Arduino Mega |
|--------|--------------|
| VCC | 5V |
| GND | GND |
| OUT | 2 |

Monte o sensor na frente do chassi, apontando para a direção de avanço,
entre 10 cm e 30 cm do chão. Ajuste o potenciômetro do módulo com o
protótipo montado, não na bancada: a altura muda o alcance.

A maioria desses módulos leva a saída para nível **baixo** quando detecta
um objeto. Se o seu faz o contrário, mude `NIVEL_IR_DETECTADO` para `HIGH`
em `config.h`.

Limitação conhecida, que deve constar no texto do TCC: o infravermelho
reflexivo depende da cor e do brilho da superfície. Ele enxerga mal
superfícies pretas foscas, vidro e objetos muito inclinados. Não detecta
buracos nem degraus para baixo.

## 4. Sinalização e emergência

| Componente | Arduino Mega | Observação |
|-----------|--------------|-----------|
| Buzzer ativo 5V (+) | 12 | opcional; o negativo vai ao GND |
| LED de status | 13 | já existe na placa |
| Botão de emergência | 3 e GND | contato normalmente aberto |

O botão usa `INPUT_PULLUP`. Sem botão instalado o pino fica em nível alto,
que é o estado "não acionado", então a ausência do botão nunca dispara uma
parada. Isso é proposital: a falha é para o lado seguro.

Se o buzzer for do tipo passivo, ele não apita com nível fixo. Use um buzzer
ativo, ou troque `digitalWrite` por `tone()` no código.

## 5. Alimentação

Este é o ponto que mais causa problema em protótipo de TCC.

Use **duas fontes com o terra em comum**:

1. Motores: bateria de 7,4 V (duas células de lítio) ou 6 pilhas AA nos
   terminais `VMS`/`12V` e `GND` do L298N.
2. Eletrônica: o Arduino Mega pela USB ou por fonte própria no conector.

Ligue os dois GND juntos. Sem terra comum os sinais de controle não têm
referência e a ponte H se comporta de forma imprevisível.

Por que 7,4 V para motores de 3-6 V: o L298N é feito com transistores
bipolares e perde de 1,5 V a 2,5 V internamente. Com 7,4 V na entrada,
chegam cerca de 5 V ao motor, dentro da faixa nominal. Alimentando com 6 V,
chegariam menos de 4 V e o conjunto fica sem força para vencer o atrito.

Não alimente os motores pelo pino 5V do Arduino. O pico de partida de dois
motores passa do que o regulador da placa aguenta e a placa reinicia sozinha
no meio da operação.

## 6. Chaves DIP da placa

A placa tem um bloco de oito chaves DIP que decide quem conversa pelo cabo
USB. Errar essa posição é a causa mais comum de falha na gravação, e o
sintoma é `avrdude: stk500v2_getsync(): timeout`.

Tabela confirmada em bancada:

| Chaves ligadas | Rótulo na placa | Para que serve |
|---|---|---|
| **3 e 4** | `USB←→MCU` | **gravar o firmware no Mega** |
| 1 e 2 | `MCU←→ESP` | o Mega conversar com o Wi-Fi |
| 5 e 6 | `USB←→ESP` | gravar o ESP8266 |
| 7 | `GND←GPIO0` | modo de gravação do ESP |

A placa costuma sair de fábrica com as chaves 5, 6 e 7 ligadas, que é o modo
de gravar o ESP8266. Nessa posição o computador enxerga a porta serial
normalmente, mas o ATmega2560 nunca responde, porque o cabo está ligado no
outro chip.

Para o uso normal deste projeto: **apenas as chaves 3 e 4 ligadas**.

## 7. ESP8266 embarcado

A placa Mega 2560 + ESP8266 traz o ESP ligado à `Serial3` por chaves DIP.
O firmware já publica a telemetria em JSON nessa porta, mas o recurso vem
desligado: mude `USAR_ESP8266` para `1` em `config.h` quando for usar.

Deixe o Wi-Fi fora do caminho de segurança. Comando por rede introduz atraso
e perda de pacote, e este protótipo depende de tempo de resposta previsível
para parar.

## 8. Diagrama de blocos

```
      voz do usuário
            |
            v
  +---------------------+
  | Módulo de voz V3    |
  +---------------------+
            | Serial1 (9600 bps)
            v
  +-------------------------------+       +--------------------+
  | Arduino Mega 2560             |<------| Sensor IR (pino 2) |
  |                               |       +--------------------+
  |  máquina de estados +         |
  |  travas de segurança          |<------| Botão emergência (3)|
  +-------------------------------+
            | PWM 5/6  + sentido 7..10
            v
  +---------------------+        +------------------------+
  | Ponte H L298N       |------->| 2 motores DC + rodas   |
  +---------------------+        +------------------------+
            ^
            |
     bateria 7,4 V
```
