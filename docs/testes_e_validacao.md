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

O comando faz três coisas:

1. roda os 40 testes da máquina de estados;
2. compila o firmware inteiro com um substituto da API do Arduino, o que
   pega erro de sintaxe e de tipo antes de abrir o IDE;
3. compila a ferramenta de treinamento do mesmo jeito.

O substituto da API fica em `testes/stub_arduino/`. Ele não vai para a
placa: existe só para a verificação no PC.

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

Registre também as falhas. Um TCC de robótica que mostra onde o protótipo
falha e explica por quê vale mais do que um que só mostra o que deu certo.
