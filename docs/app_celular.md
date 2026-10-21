# App de controle pelo celular

A cadeira pode ser comandada pelo celular, além da voz. O app é uma página
que o próprio ESP8266 da placa entrega pelo Wi-Fi: não precisa instalar
nada, não precisa de roteador e não precisa de internet.

```
  celular                 placa Mega 2560 + ESP8266
 +---------+   Wi-Fi    +----------+  Serial3  +-----------+
 |  app    |----------->| ESP8266  |---------->| Mega 2560 |---> motores
 | (página)|<-----------| app_wifi |<----------| cadeira_  |
 +---------+  /estado   +----------+ telemetria|   voz     |
               /cmd                    JSON    +-----------+
```

## Como usar

1. Ligue a cadeira.
2. No celular, conecte na rede Wi-Fi **Cadeira-TCC**, senha **cadeira123**.
   O celular pode avisar que a rede "não tem internet". Mantenha a conexão
   assim mesmo.
3. Abra o navegador em **http://192.168.4.1**.
4. Na primeira vez, o app pede o seu nome (login). A data desse dia fica
   gravada na cadeira, e a partir dela o app conta há quantos dias a cadeira
   está em uso.
5. Confira as duas bolinhas no topo:
   - **Wi-Fi** verde: o celular está falando com o ESP8266.
   - **Cadeira** verde: o ESP8266 está recebendo a telemetria do Mega.
   Os botões só ficam ativos com as duas verdes.

| Botão | O que faz |
|-------|-----------|
| Frente, Ré, Esquerda, Direita | **segure** para andar; soltar para a cadeira |
| PARAR | para na hora, sempre ativo |
| − Devagar / + Rápido | muda o nível de velocidade (1 a 3) |

No computador, as setas do teclado, a barra de espaço e as teclas `+` e `-`
fazem o mesmo.

O app mostra o estado atual, o PWM de cada roda, o nível de velocidade, se o
módulo de voz está pronto, o motivo da última parada, a carga estimada da
bateria e há quantos dias a cadeira está em uso. Com obstáculo à
frente aparece um aviso vermelho e o botão Frente fica desativado.

Voz, teclado do Monitor Serial e app funcionam ao mesmo tempo. Qualquer um
deles pode parar a cadeira.

## Bateria estimada

O Mega mede a tensão da bateria dos motores por um divisor de tensão no pino
A0 (ligação em `docs/ligacoes.md`, seção 5) e converte em porcentagem pela
curva de descarga de uma bateria de lítio de 7,4 V (duas células, 8,4 V
cheia e 6,6 V vazia).

É uma **estimativa**, e o app escreve isso embaixo do número. A tensão de uma
bateria de lítio não cai em linha reta, muda com a temperatura e com o
desgaste das células, e afunda enquanto os motores puxam corrente. Para o
número não despencar a cada partida, o firmware dá peso quase só às leituras
feitas com a cadeira parada há pelo menos 1,5 s.

Com 15% ou menos, o app mostra o aviso "Bateria fraca". Sem o divisor
instalado, a bateria aparece como "não medida" e o resto funciona normalmente.

Se usar 6 pilhas AA em vez da bateria de lítio, a porcentagem não vale: a
curva em `firmware/cadeira_voz/bateria.cpp` é a do lítio.

## Login e dias de uso

O login é feito uma vez: o nome e a data ficam gravados na memória flash do
ESP8266 e não se apagam ao desligar a cadeira. Qualquer celular que abrir o
app depois vê o mesmo login.

O ESP8266 não tem relógio. A data do login vem do celular que fez o login, e
a conta dos dias é feita pelo app com o relógio do celular que está aberto.
Os dias contam por data do calendário: no dia do login aparece "hoje", no dia
seguinte "1 dia", e assim por diante. Por isso o relógio do celular precisa
estar certo, e o ESP recusa login com data anterior a 2024.

"Refazer login", no fim da tela, apaga o registro e zera a contagem. O app
pede confirmação e para a cadeira antes.

## Segurança

O app não tem um caminho próprio até os motores. Cada toque vira o mesmo
comando que a voz produz e passa pela mesma máquina de estados, então todas
as travas do README continuam valendo: obstáculo recusa Frente, homem-morto
de 3 s, botão de emergência com prioridade, rampa de aceleração.

O Wi-Fi nunca é necessário para parar. Se ele falhar, a cadeira para
sozinha. Por isso o app acrescenta três travas, todas no sentido de parar:

1. **Segure para andar.** Enquanto o dedo está no botão, o app repete o
   comando a cada 300 ms. Ao soltar, manda PARAR.
2. **Trava do ESP8266.** Se a repetição parar de chegar por 1 s (celular
   bloqueou a tela, saiu do alcance, travou), o ESP8266 manda PARAR sozinho.
   Por baixo dela continua o homem-morto de 3 s do Mega, que não depende do
   ESP.
3. **Saiu do app, parou.** Trocar de aba, bloquear a tela ou minimizar o
   navegador manda PARAR.

Além disso, o Mega só aceita linhas no formato exato `CMD:x`. O ESP8266
despeja texto de inicialização na serial toda vez que liga, e esse texto tem
letras como `e`, `t` e `d`. Se o Mega aceitasse letras soltas, ligar o Wi-Fi
poderia fazer a cadeira partir sozinha. Comando tocado no app com o botão de
emergência apertado é descartado, e não executado depois que o botão é solto.

## Gravação

São dois firmwares, um em cada chip da placa. As chaves DIP escolhem qual
chip está no cabo USB (tabela em `docs/ligacoes.md`, seção 6).

### 1. Mega 2560: `firmware/cadeira_voz`

`USAR_ESP8266` já vem `1` em `config.h`. Grave como sempre, com as chaves
**3 e 4** ligadas.

### 2. ESP8266: `firmware/app_wifi`

1. Desligue a placa do USB. Ligue as chaves **5, 6 e 7** e desligue as
   demais.
2. **Arduino IDE**: em *Preferências → URLs adicionais*, adicione
   `https://arduino.esp8266.com/stable/package_esp8266com_index.json`,
   instale o pacote **esp8266** no Gerenciador de Placas e escolha a placa
   **Generic ESP8266 Module**. Abra `firmware/app_wifi/app_wifi.ino` e
   envie.

   **PlatformIO**: em `platformio.ini`, troque o `src_dir` para
   `firmware/app_wifi` e rode `pio run -e esp8266 --target upload`.
3. Desligue a chave **7** (senão o ESP volta sempre em modo de gravação).

Essa gravação só precisa ser feita uma vez.

### 3. Uso normal

Chaves **1 e 2** ligadas: é por elas que o Mega e o ESP8266 conversam.
Para ver o Monitor Serial do Mega ao mesmo tempo, ligue também a **3 e 4**.
A combinação 1+2+3+4 é a que o fabricante indica para "USB no Mega e Mega no
ESP" nessas placas, mas confirme em bancada, como foi feito com a tabela da
seção 6.

## Problemas comuns

| Sintoma | Causa provável |
|---------|----------------|
| A rede Cadeira-TCC não aparece | ESP8266 não gravado, ou chave 7 ainda ligada |
| Página abre, bolinha Cadeira vermelha | chaves 1 e 2 desligadas, ou `USAR_ESP8266` em `0` no Mega |
| Página não abre | celular trocou sozinho para outra rede ou para os dados móveis; desligue os dados móveis |
| Monitor Serial mostra `comando ... (app) RECUSADO` | trava agindo, normalmente obstáculo à frente |

## Protocolo, para referência

Celular → ESP8266 (HTTP):

| Requisição | Resposta |
|-----------|----------|
| `GET /` | a página do app |
| `GET /cmd?c=frente` | `ok`; também `re`, `esquerda`, `direita`, `parar`, `rapido`, `devagar` |
| `GET /estado` | `{"mega":1,"clientes":1,"login":{"nome":"Ana","desde":1792540800},"cadeira":{...}}` |
| `GET /login?nome=Ana&t=1792540800` | grava o login; `t` em segundos desde 1970 |
| `GET /login?apagar=1` | apaga o login |

ESP8266 → Mega (Serial3, 115200 bps), uma linha por comando:

```
CMD:F  CMD:T  CMD:E  CMD:D  CMD:P  CMD:+  CMD:-
```

Mega → ESP8266, uma linha a cada 200 ms:

```json
{"estado":"FRENTE","pwmEsq":130,"pwmDir":130,"nivel":1,"obstaculo":0,
 "emergencia":0,"voz":1,"recusado":0,"batMv":7900,"batPct":73,"motivo":"-"}
```

`batPct` vale `-1` quando não há bateria ligada no divisor.

## Testes

`make -C testes` também roda `testes/test_app.cpp`, que compila o firmware
do ESP8266 no PC e liga a saída dele direto no leitor do Mega e na máquina
de estados. Cada toque simulado no app percorre o caminho inteiro até o PWM
das rodas. A verificação do firmware do Mega confere ainda que o texto de
boot do ESP não move a cadeira e que um comando dado durante a emergência é
descartado.
