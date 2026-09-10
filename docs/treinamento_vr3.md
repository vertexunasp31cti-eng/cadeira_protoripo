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
| `Sem resposta. Confira RX/TX cruzados` | TX ligado no TX, ou módulo sem alimentação |
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

O quadro de resultado traz o número do registro na posição 5:

```
índice: 0     1     2     3         4      5         6       7
        0xAA  0x07  0x0D  contagem  grupo  REGISTRO  índice  tam_assinatura  ... 0x0A
```

A ferramenta de treinamento imprime os bytes recebidos em hexadecimal, o que
serve tanto para depurar quanto para ilustrar o protocolo na apresentação.
