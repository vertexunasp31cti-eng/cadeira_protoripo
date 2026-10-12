# Treinamento do módulo de voz V3

O módulo não vem com palavras prontas. Ele grava a **sua** voz dizendo cada
comando e depois compara o que ouve com o que foi gravado. Por isso ele
funciona melhor com quem treinou e funciona mal com outra pessoa. Isso é uma
característica do módulo, não um defeito da montagem, e vale registrar no
texto do TCC.

## Ordem dos registros

O firmware espera exatamente esta ordem. Trocar a ordem faz a cadeira andar
para o lado errado.

| Registro | Palavra sugerida | Ação |
|----------|------------------|------|
| 0 | frente   | anda para frente até parar |
| 1 | trás     | anda para trás até parar |
| 2 | esquerda | gira à esquerda por um tempo curto |
| 3 | direita  | gira à direita por um tempo curto |
| 4 | parar    | para na hora |
| 5 | rápido   | sobe um nível de velocidade |
| 6 | devagar  | desce um nível de velocidade |

## Passo a passo

1. Monte a fiação do módulo conforme `docs/ligacoes.md`.
2. Abra `firmware/treinar_voz/treinar_voz.ino` no Arduino IDE, escolha a
   placa **Arduino Mega 2560** e envie.
3. Abra o Monitor Serial em **115200**, com final de linha em "Nova linha".
   O menu aparece na tela.
4. Digite o número do registro, de `0` a `6`, e pressione Enter.
5. O módulo pede a palavra **duas vezes**. Fale, espere ele processar, fale
   de novo. O resultado aparece na tela.
6. Repita para os sete registros.
7. Digite `l` para carregar os sete registros e entrar em modo de escuta.
   Fale cada palavra e confira se aparece o número certo.
8. Só depois disso envie o firmware principal
   `firmware/cadeira_voz/cadeira_voz.ino`.

O treinamento fica gravado na memória do próprio módulo. Você não precisa
repetir a cada vez que ligar, nem ao trocar de sketch.

## Quando o firmware não reconhece o módulo

Ao ligar, o firmware principal procura o módulo e escreve o resultado no
Monitor Serial (115200). A mensagem diz em qual caso você está:

| Mensagem no Monitor Serial | O que significa | O que fazer |
|---|---|---|
| `nenhum byte chegou do modulo` | o Arduino não recebe nada | confira 5V e GND no módulo, GND comum, e o fio TX do módulo no pino **19** (não no 18, não no TX0) |
| `chegaram bytes, mas nenhum quadro valido` | há sinal, mas corrompido | mau contato no jumper, fio longo, ou módulo ligado em 3,3 V |
| `nenhum registro esta treinado` | o módulo responde, mas está vazio | rode a ferramenta `treinar_voz` e grave de 0 a 6 |
| `registro N (...): NAO TREINADO` | aquele comando nunca vai ser reconhecido | grave só esse registro de novo |
| `Modulo de voz respondeu a 19200 bps` (ou outro) | o baud do módulo foi mudado | nada: o firmware se ajusta sozinho |

Depois de `bytes recebidos do modulo:` o firmware imprime os primeiros bytes
em hexadecimal. Um módulo saudável começa sempre com `AA`.

Se aparecer `Modulo de voz pronto` mas as palavras não fazem nada, o
problema é o reconhecimento em si: regrave as palavras no mesmo lugar e na
mesma distância em que vai usar (veja a seção abaixo). Para testar sem os
motores, use o comando `l` da ferramenta de treinamento, que mostra o número
de cada palavra reconhecida.

## Como aumentar a taxa de acerto

O reconhecimento do V3 compara padrões de áudio, então consistência importa
mais do que pronúncia bonita.

- Grave no mesmo ambiente onde vai apresentar, com o mesmo ruído de fundo.
- Fale sempre à mesma distância do microfone, entre 20 cm e 50 cm.
- Mantenha o mesmo ritmo e o mesmo tom nas duas amostras.
- Prefira palavras com sons diferentes entre si. "trás" e "atrás" se
  confundem; "frente" e "parar" não.
- Não grave com os motores ligados. O ruído entra na amostra e depois o
  módulo passa a exigir aquele ruído para reconhecer.
- Se um comando errar muito, regrave só aquele registro.

Palavras curtas demais, como "vai" ou "ré", costumam falhar. Se "trás"
apresentar problema, use "recuar" no lugar. O firmware não muda, porque ele
usa o número do registro, não a palavra.

## Mensagens de erro da ferramenta

| Mensagem | Causa provável |
|----------|----------------|
| `Modulo NAO respondeu em nenhuma velocidade` | TX ligado no TX, módulo sem alimentação ou sem GND comum |
| `Sem resposta. Confira RX/TX cruzados` | TX ligado no TX, ou módulo sem alimentação |
| `NAO TREINADO - grave este registro` | aquele registro está vazio no módulo |
| `tempo esgotado, o modulo nao ouviu nada` | você demorou a falar, ou falou baixo demais |
| `as duas amostras nao coincidiram` | as duas falas saíram diferentes; repita com calma |
| `numero de registro invalido` | registro fora da faixa 0 a 6 |

## Detalhe do protocolo, para o texto do TCC

A comunicação usa quadros no formato:

```
0xAA  LEN  CMD  [dados...]  0x0A
```

`LEN` conta o byte de comando, os dados e o `0x0A` final, então o quadro
inteiro ocupa `LEN + 2` bytes. Os comandos usados no projeto:

| Comando | Código | Função |
|---------|--------|--------|
| treinar | `0x20` | grava um registro |
| carregar | `0x30` | põe registros no reconhecedor (máximo 7) |
| limpar | `0x31` | esvazia o reconhecedor |
| resultado | `0x0D` | enviado pelo módulo quando reconhece uma palavra |

A resposta do comando carregar traz um par por registro pedido:

```
0xAA  LEN  0x30  N  R0  STA0  R1  STA1 ...  0x0A
```

`STA` vale `0x00` quando o registro foi carregado e `0xFE` quando ele nunca
foi treinado. O módulo responde esse quadro mesmo vazio, por isso o firmware
confere o status de cada registro em vez de só esperar a resposta.

O quadro de resultado traz o número do registro na posição 5:

```
índice: 0     1     2     3         4      5         6       7
        0xAA  0x07  0x0D  contagem  grupo  REGISTRO  índice  tam_assinatura  ... 0x0A
```

A ferramenta de treinamento imprime os bytes recebidos em hexadecimal, o que
serve tanto para depurar quanto para ilustrar o protocolo na apresentação.
