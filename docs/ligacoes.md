# Ligações do protótipo

Todos os números de pino também estão em `firmware/cadeira_voz/config.h`.
Se você mudar a montagem, mude lá e não espalhe números pelo código.

## 1. Ponte H dupla L298N

| L298N | Arduino Mega | Observação |
|-------|--------------|-----------|
| ENA   | 5            | PWM, velocidade do motor esquerdo |
| IN1   | 22           | sentido do motor esquerdo |
| IN2   | 23           | sentido do motor esquerdo |
| IN3   | 24           | sentido do motor direito |
| IN4   | 25           | sentido do motor direito |
| ENB   | 6            | PWM, velocidade do motor direito |
| GND   | GND          | terra comum, obrigatório |
| OUT1/OUT2 | motor esquerdo | |
| OUT3/OUT4 | motor direito  | |

Retire os jumpers de ENA e ENB. Com eles instalados a placa ignora o PWM e
os motores só funcionam em velocidade máxima.

Por que os pinos 5 e 6: no Mega, os pinos 4 e 13 usam o Timer0, o mesmo
timer da função `millis()`. Usar PWM neles atrapalha a base de tempo, e toda
a segurança do projeto depende de `millis()`.

## 2. Módulo de Reconhecimento de Voz V3

| Módulo V3 | Arduino Mega | Observação |
|-----------|--------------|-----------|
| VCC | 5V  | consome pouco, pode sair da própria placa |
| GND | GND | |
| TX  | 19 (RX1) | **cruzado**: TX do módulo no RX do Arduino |
| RX  | 18 (TX1) | **cruzado**: RX do módulo no TX do Arduino |

O erro mais comum de montagem é ligar TX no TX. O sintoma é a mensagem
`modulo de voz nao respondeu` no Monitor Serial.

O módulo sai de fábrica em 9600 bps. Ele guarda até 80 registros gravados,
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
| Buzzer ativo 5V (+) | 8 | o negativo vai ao GND |
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

## 6. ESP8266 embarcado

A placa Mega 2560 + ESP8266 traz o ESP ligado à `Serial3` por chaves DIP.
O firmware já publica a telemetria em JSON nessa porta, mas o recurso vem
desligado: mude `USAR_ESP8266` para `1` em `config.h` quando for usar.

Deixe o Wi-Fi fora do caminho de segurança. Comando por rede introduz atraso
e perda de pacote, e este protótipo depende de tempo de resposta previsível
para parar.

## 7. Diagrama de blocos

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
            | PWM 5/6  + sentido 22..25
            v
  +---------------------+        +------------------------+
  | Ponte H L298N       |------->| 2 motores DC + rodas   |
  +---------------------+        +------------------------+
            ^
            |
     bateria 7,4 V
```
