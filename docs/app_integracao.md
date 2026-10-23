# App do celular

O app mostra ao vivo o que a cadeira está fazendo e permite comandá-la pelo
celular. Ele fica gravado no ESP8266 da própria placa: o celular entra na
rede Wi-Fi da cadeira e abre o app no navegador. Não precisa de internet, de
roteador nem de instalar nada.

O reconhecimento de voz continua igual, no módulo embarcado. O app é mais
uma forma de acompanhar e de comandar, e passa pelas mesmas travas de
segurança da voz.

## 1. O que o app faz

O app tem três abas.

**Painel.** Mostra o estado da cadeira (parada, andando para frente, de ré,
girando), o motivo da última parada, o nível de velocidade, o PWM de cada
roda numa vista de cima da cadeira, o sensor de obstáculo e o módulo de voz.
Faixas de alerta aparecem para emergência, obstáculo à frente e módulo de
voz indisponível. O botão **PARAR** fica sempre visível e sempre ativo, no
estilo de um botão de emergência de máquina.

**Histórico.** No topo, um resumo do que a cadeira contou desde que foi
ligada: obstáculos detectados, acionamentos do botão de emergência, tempo
andando, comandos recusados, comandos por origem (voz, app e teclado) e
paradas por motivo. Quem conta é o Mega, então os números valem mesmo que o
celular tenha conectado depois; eles zeram quando a cadeira é desligada.
Abaixo vem a lista do que aconteceu, com horário: cadeira ligada, cada
comando com a origem e se foi aceito, cada parada com o motivo, obstáculo
detectado e liberado, emergência acionada e liberada.

Os números do resumo servem para a seção de resultados do TCC: depois de um
ensaio, por exemplo, quantas vezes o sensor parou a cadeira e quantas paradas
foram pelo homem-morto.

**Sobre o TCC.** Apresenta o projeto: objetivo, justificativa, como
funciona, os sete comandos de voz, as oito travas de segurança, componentes,
metodologia, limitações e trabalhos futuros. Serve de apoio na apresentação
para a banca.

### Comandar pelo app

O controle começa **bloqueado**, para um toque acidental no bolso não mover a
cadeira. Toque em "Controle pelo app" para liberar o direcional.

| Botão | Como usar |
|-------|-----------|
| ▲ Frente e ▼ Ré | **segure para andar**; soltou, parou |
| ◀ Esquerda e ▶ Direita | toque; o giro é um pulso curto e para sozinho |
| − Devagar e + Rápido | toque; muda o nível de velocidade |
| PARAR | toque; funciona mesmo com o controle bloqueado |

No computador também valem as setas do teclado (segurar ↑ ou ↓ para andar),
a barra de espaço para parar e as teclas `+` e `-`.

Se o comando for recusado, por exemplo Frente com obstáculo, o app mostra um
aviso como "Frente recusado: obstáculo à frente".

### Modo demonstração

Sem conexão com a cadeira, o app oferece "Ver demonstração". Uma cadeira
simulada roda no próprio navegador, com as mesmas regras de rampa, giro,
homem-morto e obstáculo, e dois botões extras para simular obstáculo e botão
de emergência. Uma faixa fixa avisa que os dados são simulados. Serve de
plano B na apresentação e para mostrar o app sem a placa.

Aberto por um endereço `https`, como a vitrine publicada na internet (seção
3.8), o app vai direto para a demonstração: dali ele não tem como alcançar a
cadeira, e nada sai pela rede.

## 2. Usar no dia a dia

1. Ligue a cadeira. As chaves DIP precisam estar na posição de uso (seção
   3.1).
2. No celular, entre na rede Wi-Fi **Cadeira-TCC**, senha **cadeira123**.
3. Muitos celulares abrem o app sozinhos ao entrar na rede, como numa rede
   de hotel. Se não abrir, digite **http://192.168.4.1** no navegador.
4. Para ter um ícone, use "Adicionar à tela inicial" no menu do navegador.

O celular pode avisar que a rede "não tem internet". Escolha manter a
conexão. Se o app não carregar, desligue os dados móveis: alguns celulares
mandam tudo pela rede móvel quando o Wi-Fi não tem internet.

Até quatro celulares podem ficar conectados ao mesmo tempo, por exemplo o do
usuário e o de um cuidador acompanhando.

## 3. Gravar o firmware do ESP8266

Só precisa ser feito uma vez, e de novo quando o app ou o firmware do ESP
mudar. O ESP8266 é um segundo microcontrolador na placa, com firmware
próprio em `firmware/esp_wifi/`.

### 3.1 Chaves DIP

A tabela confirmada em bancada está em `docs/ligacoes.md`, seção 6.

| Situação | Chaves ligadas |
|----------|----------------|
| Gravar o Mega | 3 e 4 |
| Gravar o ESP8266 | 5, 6 e 7 |
| Usar a cadeira com o app | 1 e 2 (Mega com ESP) |
| Usar e ver o Monitor Serial do Mega | 1, 2, 3 e 4 |

A combinação de uso com o monitor (1, 2, 3 e 4) segue os rótulos da placa,
mas ainda precisa ser confirmada em bancada, como foi feito com a tabela
original. A chave 7 precisa ficar desligada para usar: com ela ligada o ESP
volta ao modo de gravação em vez de rodar o firmware.

Algumas placas têm também uma chave deslizante que escolhe qual serial do
Mega vai para o ESP. Se a sua tiver, deixe em **RXD3/TXD3**: o firmware fala
com o ESP pela Serial3.

### 3.2 Arduino IDE

1. Em Arquivo > Preferências > URLs adicionais para Gerenciadores de Placas,
   acrescente
   `http://arduino.esp8266.com/stable/package_esp8266com_index.json`.
2. Em Ferramentas > Placa > Gerenciador de Placas, instale **esp8266**
   (versão 3.x).
3. Escolha a placa **Generic ESP8266 Module**. Em Flash Size, escolha o
   tamanho da flash do ESP da sua placa. Costuma ser 4MB nessas placas, mas
   confira: o esptool informa o tamanho detectado na gravação.
4. Ponha as chaves em 5, 6 e 7, tire e recoloque o cabo USB, abra
   `firmware/esp_wifi/esp_wifi.ino` e envie.
5. Volte as chaves para a posição de uso (1 e 2).

Nenhuma biblioteca externa é necessária: só as que vêm com o pacote esp8266.

### 3.3 PlatformIO

O ESP tem um projeto PlatformIO próprio. Da raiz do repositório:

```bash
pio run -d firmware/esp_wifi --target upload
```

As chaves seguem o mesmo roteiro do Arduino IDE. Os detalhes estão
comentados em `firmware/esp_wifi/platformio.ini`.

### 3.4 Firmware do Mega

O firmware do Mega já vem com o app ligado (`USAR_ESP8266 1` em
`firmware/cadeira_voz/config.h`). Grave-o normalmente, como no README.

### 3.5 Mudar o app

O app é o arquivo `app/index.html`. Depois de editar, gere de novo o
cabeçalho que vai para o ESP e grave o ESP:

```bash
python3 ferramentas/gerar_pagina.py
```

O script compacta o app com gzip (de uns 100 KB para uns 28 KB) e grava em
`firmware/esp_wifi/pagina.h`. O `make -C testes` acusa se o `pagina.h`
ficou desatualizado.

Para abrir o app no computador sem gravar nada, abra `app/index.html` no
navegador (ele oferece a demonstração) ou use o simulador da seção 7.

### 3.6 Nome e senha da rede

Ficam em `firmware/esp_wifi/config_wifi.h`. **Troque a senha** antes de
apresentar em lugar com muita gente: quem estiver na rede consegue abrir o
app. A senha precisa ter pelo menos 8 caracteres.

### 3.7 Equipe na aba Sobre

Os nomes dos integrantes e do orientador, a instituição e o ano ficam no
objeto `EQUIPE`, no começo do script de `app/index.html`. Para corrigir
algum, edite ali e gere o `pagina.h` de novo. A seção "Equipe" só aparece
quando há integrantes preenchidos.

### 3.8 Vitrine online (Vercel)

O app pode ser publicado na internet como vitrine, para a banca ou qualquer
pessoa ver o app e a aba "Sobre o TCC" num link ou QR code. Online ele abre
direto na demonstração, com o aviso de como comandar a cadeira de verdade:
uma página `https` na internet não consegue falar com a cadeira em
`http://192.168.4.1`, e o celular na rede da cadeira não tem internet.

Na Vercel: Add New > Project, importe o repositório, em **Root Directory**
escolha `app`, em **Framework Preset** deixe **Other**, sem comando de build,
e clique em Deploy. Cada mudança no `app/index.html` que chegar à branch
principal do repositório é publicada de novo sozinha.

## 4. Como funciona

```
  celular (navegador)
          |
          |  Wi-Fi "Cadeira-TCC", HTTP
          v
  +---------------------+   GET /            o app (compactado)
  | ESP8266             |   GET /api/estado  estado e eventos, a cada 300 ms
  | servidor web,       |   POST /api/comando
  | ponte serial        |
  +---------------------+
          ^   |
          |   |  serial 115200 bps (chaves DIP 1 e 2)
          |   v
  +-------------------------------+
  | Arduino Mega 2560 (Serial3)   |  telemetria a cada 250 ms,
  | máquina de estados e travas   |  eventos na hora
  +-------------------------------+
          |
          v
     ponte H e motores
```

O ESP8266 não decide nada sobre movimento. Ele recebe os pedidos do app e os
repassa ao Mega, que os trata exatamente como um comando de voz: mesma
máquina de estados, mesmas travas. No sentido contrário, o Mega manda o
estado da cadeira a cada 250 ms e um aviso no instante de cada evento. O ESP
guarda o último estado e os 20 últimos eventos, e o app consulta a cada
300 ms.

O laço do Mega continua sem esperar nada. Os comandos do app são lidos logo
depois do teclado, no máximo 64 bytes por volta. A saída para o ESP vai
para uma fila e sai aos poucos, só o que cabe no buffer da serial, para uma
linha de 150 bytes não segurar o laço por vários milissegundos com o botão
de emergência e o sensor sem leitura.

## 5. Segurança: por que o Wi-Fi fica fora do caminho de segurança

Comando por rede tem atraso e perda de pacote. A cadeira precisa de tempo de
resposta previsível para parar. Por isso toda trava continua local, no Mega,
e o app só pode pedir o que a voz também pode.

| Proteção | Como funciona |
|----------|---------------|
| Mesma máquina de estados | o comando do app passa por `aplicarComando`, como a voz; obstáculo, homem-morto, rampa e passagem por zero valem igual |
| Segure para andar | Frente e Ré andam só enquanto o dedo está no botão; o app renova o comando a cada 400 ms e manda parar ao soltar, ao trocar de aba, ao bloquear a tela ou ao perder a conexão |
| Homem-morto cobre a queda da rede | se as renovações deixam de chegar, a cadeira para sozinha em até 3 s |
| Renovação nunca religa | a renovação só estende um movimento que já está acontecendo; depois de qualquer parada, andar exige um comando novo |
| Giro não é renovável | o giro continua sendo um pulso curto, como na voz |
| Parar nunca é bloqueado | o ESP repassa "parar" mesmo sem resposta do Mega, e o Mega sempre aceita |
| Pedido atrasado é descartado | cada pedido leva um número crescente; o ESP descarta um pedido que chega depois de um mais novo do mesmo celular |
| Comando sem resposta vira parada | se um comando de movimento não tem resposta em 1,2 s, o app manda parar e avisa "Conexão instável" |
| Ruído não vira comando | toda linha da serial tem checksum e o texto precisa casar exatamente; o lixo que o ESP imprime ao ligar é descartado |
| Emergência descarta tudo | com o botão apertado, o que chega do app, da voz e do teclado é lido e jogado fora, para nada ser executado quando ele for solto |
| Nada do setup é executado | comandos que chegam enquanto o Mega liga são descartados no fim do `setup()` |
| Modo só acompanhamento | com `PERMITIR_COMANDO_APP 0` em `config.h`, o app só acompanha; movimento é recusado e "parar" continua aceito |

Cada uma dessas proteções tem teste automatizado (seção 7 e
`docs/testes_e_validacao.md`).

## 6. Protocolo e API

Esta seção serve para quem for escrever outro cliente, por exemplo um app no
MIT App Inventor, e para o capítulo técnico do TCC.

### 6.1 Serial entre o Mega e o ESP8266

Toda linha, nos dois sentidos, tem o formato `<conteúdo>*HH`, terminada em
`\n`. `HH` é o XOR de todos os bytes do conteúdo em dois dígitos
hexadecimais. Linha com checksum errado, com byte fora do ASCII imprimível
ou grande demais é descartada inteira.

**Mega para ESP: telemetria**, a cada 250 ms, campos sempre nesta ordem:

```
{"t":"tel","ms":15230,"estado":"FRENTE","pwmEsq":130,"pwmDir":130,"nivel":1,"obstaculo":0,"emergencia":0,"motivo":"NENHUMA","voz":1,"app":1}*7F
```

| Campo | Significado |
|-------|-------------|
| `ms` | `millis()` do Mega |
| `estado` | PARADA, FRENTE, RE, GIRO_ESQ ou GIRO_DIR |
| `pwmEsq`, `pwmDir` | PWM de cada roda, de -255 a 255 (negativo é para trás) |
| `nivel` | nível de velocidade, de 1 a 3 |
| `obstaculo` | 1 quando o sensor vê algo à frente |
| `emergencia` | 1 enquanto o botão de emergência está apertado |
| `motivo` | última parada: NENHUMA, COMANDO, OBSTACULO, TEMPO_LIMITE, FIM_DO_GIRO ou EMERGENCIA |
| `voz` | 1 se o módulo de voz iniciou |
| `app` | valor de `PERMITIR_COMANDO_APP` |

**Mega para ESP: eventos**, na hora em que acontecem:

```
{"t":"evt","ms":120,"tipo":"inicio","voz":1}*4F
{"t":"evt","ms":15100,"tipo":"comando","cmd":"FRENTE","origem":"voz","aceito":1}*4D
{"t":"evt","ms":18100,"tipo":"parada","motivo":"TEMPO_LIMITE"}*10
{"t":"evt","ms":19000,"tipo":"obstaculo","ativo":1}*37
{"t":"evt","ms":20000,"tipo":"emergencia","ativa":1}*55
```

O evento `comando` sai para todo comando tratado, de voz, teclado ou app. A
`parada` sai quando a cadeira passa de um movimento para parada.

**Mega para ESP: estatísticas**, uma vez por segundo, com os contadores
desde que a cadeira ligou:

```
{"t":"est","lig":3725,"mov":95,"voz":12,"app":30,"tec":2,"rec":3,"obst":5,"emerg":1,"pcmd":10,"pobst":4,"ptlim":6,"pgiro":8,"pemerg":1}*3B
```

| Campo | Significado |
|-------|-------------|
| `lig`, `mov` | segundos ligada e segundos com alguma roda girando |
| `voz`, `app`, `tec` | comandos tratados, por origem (renovação não conta) |
| `rec` | comandos recusados, de qualquer origem |
| `obst`, `emerg` | vezes que o sensor passou a ver obstáculo e que o botão de emergência foi apertado |
| `pcmd`, `pobst`, `ptlim`, `pgiro`, `pemerg` | paradas por comando, obstáculo, tempo limite (homem-morto), fim do giro e emergência |

Os contadores param em 65535 em vez de voltar a zero. Uma parada só conta
quando a cadeira estava andando: Frente recusada com a cadeira já parada
conta em `rec`, não em `pobst`.

**ESP para Mega: comandos.**

```
\nCMD:frente*7E\n    comando normal (bip e evento)
\nMNT:frente*63\n    renovação do segure para andar (sem bip, sem evento)
```

Nomes válidos: `frente`, `tras`, `esquerda`, `direita`, `parar`, `rapido`,
`devagar`. Só `frente` e `tras` aceitam `MNT`. O `\n` antes da linha fecha
qualquer lixo pendente na serial, como a mensagem que o ESP imprime ao
ligar, para o primeiro comando depois de um reinício nunca se perder.

### 6.2 API HTTP do ESP8266

Endereço: `http://192.168.4.1`. Todas as respostas de `/api/` são JSON, com
`Cache-Control: no-store` e `Access-Control-Allow-Origin: *`.

**`GET /api/estado?desde=N`** devolve o último estado e os eventos com
número de sequência maior que `N`:

```json
{"conectado":true,"idade_ms":120,
 "tel":{"t":"tel","ms":15230,"estado":"FRENTE","pwmEsq":130,"pwmDir":130,"nivel":1,"obstaculo":0,"emergencia":0,"motivo":"NENHUMA","voz":1,"app":1},
 "est":{"t":"est","lig":3725,"mov":95,"voz":12,"app":30,"tec":2,"rec":3,"obst":5,"emerg":1,"pcmd":10,"pobst":4,"ptlim":6,"pgiro":8,"pemerg":1},
 "seq":42,
 "eventos":[{"seq":42,"idade_ms":3000,"dados":{"t":"evt","ms":15100,"tipo":"comando","cmd":"FRENTE","origem":"voz","aceito":1}}],
 "esp":{"uptime_ms":123456,"clientes":1,"linhas_ok":1234,"linhas_erro":2}}
```

- `conectado` fica falso quando o Mega passa 1,5 s sem mandar nada.
- `tel` e `idade_ms` são `null` antes da primeira telemetria, e `est` antes
  da primeira linha de estatísticas.
- `idade_ms` de cada evento é há quanto tempo o ESP o recebeu.
- Sem `desde`, vêm todos os eventos guardados (no máximo 20).

**`POST /api/comando`**, com os parâmetros no corpo (form-urlencoded) ou na
URL:

| Parâmetro | Obrigatório | Significado |
|-----------|-------------|-------------|
| `c` | sim | nome do comando |
| `manter` | não | `1` para renovação (só `frente` e `tras`) |
| `id` | não | texto curto que identifica o celular |
| `n` | não | número crescente a cada pedido do mesmo `id` |

| Resposta | Quando |
|----------|--------|
| 200 `{"ok":true,"conectado":true}` | repassado ao Mega |
| 200 `{"ok":true,"conectado":false}` | "parar" sem resposta do Mega: repassado mesmo assim |
| 400 `{"ok":false,"erro":"comando_invalido"}` | nome ou `manter` inválido |
| 409 `{"ok":false,"erro":"comando_atrasado"}` | pedido mais antigo que outro já recebido do mesmo `id` |
| 503 `{"ok":false,"erro":"sem_conexao"}` | o Mega não está respondendo |

"ok" quer dizer que o ESP repassou o pedido. Se a cadeira aceitou ou recusou
aparece no evento `comando` e no estado.

**Outras rotas:** `GET /` devolve o app, `GET /manifest.webmanifest` e
`GET /icone.svg` servem para "Adicionar à tela inicial", e qualquer outro
endereço redireciona para o app (é isso que faz o celular abrir o app sozinho
ao entrar na rede).

### 6.3 Exemplo com o MIT App Inventor

Com o componente **Web**:

- estado: `Url` = `http://192.168.4.1/api/estado`, chame `Get` num
  `Clock` de 300 ms a 500 ms e leia o JSON com `JsonTextDecode`;
- comando: `Url` = `http://192.168.4.1/api/comando?c=parar` (ou outro
  comando) e chame `PostText` com um texto vazio. Os parâmetros na própria
  URL funcionam qualquer que seja o tipo de conteúdo que o App Inventor
  mandar no corpo.

Sem `id` e `n` o ESP não faz a verificação de ordem. Para andar, siga a mesma
regra do app oficial: mande `c=frente` ao apertar, `c=frente&manter=1` a cada
400 ms enquanto o botão estiver apertado e `c=parar` ao soltar.

## 7. Testar no computador, sem a placa

O simulador roda o firmware do Mega e o do ESP8266 compilados para o PC, com
o app de verdade. Precisa de `g++`, `make` e Python 3, como os testes.

```bash
python3 testes/simulador/servidor.py
```

Depois abra **http://localhost:8000**. O app conversa com o código real das
duas placas; só o hardware é simulado. Para mexer no "hardware", abra estes
endereços em outra aba:

| Endereço | Efeito |
|----------|--------|
| `/sim/obstaculo?v=1` | o sensor vê um obstáculo (`v=0` libera) |
| `/sim/emergencia?v=1` | botão de emergência apertado (`v=0` solta) |
| `/sim/mega_desligado?v=1` | a placa do Mega para de responder (`v=0` volta) |
| `/sim/reiniciar_esp` | o ESP8266 reinicia, com o lixo de boot na serial |

Opções úteis: `--porta 8080` troca a porta, `--rede` deixa um celular na
mesma rede abrir o app pelo IP do computador, `--depuracao` mostra a Serial
de depuração do Mega e `--mega testes/build/mega_sim_so_acompanha` usa o
firmware com `PERMITIR_COMANDO_APP 0`.

Há também um teste de ponta a ponta que usa o app num navegador contra o
simulador: segura Frente, solta, gira, simula obstáculo, emergência, queda
do Wi-Fi e reinício do ESP, e confere o que a cadeira fez. Precisa de Node e
do Playwright:

```bash
NODE_PATH=$(npm root -g) node testes/simulador/e2e_app.js
```

## 8. Problemas comuns

| Sintoma | Causa provável |
|---------|----------------|
| A rede Cadeira-TCC não aparece | ESP sem o firmware gravado, ou chave 7 ligada (ESP em modo de gravação) |
| O app abre, mas diz "Wi-Fi ok, mas a placa não responde" | chaves 1 e 2 desligadas, Mega desligado, firmware do Mega antigo ou com `USAR_ESP8266 0`, ou a chave deslizante fora de RXD3/TXD3 |
| "Sem conexão com a cadeira" | o celular saiu da rede da cadeira, ou está usando os dados móveis |
| O app não abre em 192.168.4.1 | desligue os dados móveis e confira que o celular está na rede Cadeira-TCC |
| O interruptor "Controle pelo app" está cinza | o firmware do Mega está com `PERMITIR_COMANDO_APP 0` |
| Frente recusado | obstáculo à frente; a ré continua liberada |
| "Conexão instável: parada enviada por segurança" | um comando de movimento ficou sem resposta por 1,2 s; aproxime o celular da cadeira |
| O app mostra uma versão antiga | grave o ESP de novo depois de rodar `python3 ferramentas/gerar_pagina.py`, e recarregue a página |

## 9. Limitações do app

- O alcance é o do Wi-Fi do ESP8266, de alguns metros a algumas dezenas de
  metros, conforme as paredes e as interferências.
- A segurança da rede é a senha WPA2 gravada no firmware. Não há login no
  app: quem está na rede pode comandar a cadeira, dentro das mesmas travas.
- O histórico fica na memória do navegador. Recarregar a página traz de
  volta só os 20 últimos eventos guardados no ESP.
- O horário dos eventos é aproximado: nem o Mega nem o ESP têm relógio, e o
  app calcula a hora pelo tempo desde que o ESP recebeu cada evento.
- O app depende do celular estar na rede da cadeira. Acompanhar de longe,
  pela internet, exigiria um servidor externo, fora do escopo deste
  protótipo.
