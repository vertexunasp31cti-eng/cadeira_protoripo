# Apoio para a escrita do TCC

Este arquivo reúne o conteúdo técnico do protótipo organizado do jeito que
costuma ser cobrado em um trabalho de conclusão. Não é o texto pronto: é a
matéria-prima, para você escrever com suas palavras.

## Objetivo geral

Desenvolver e validar um protótipo funcional de cadeira de rodas motorizada
controlada por comandos de voz, com travas de segurança que impeçam
movimento indesejado.

## Objetivos específicos

1. Integrar um módulo de reconhecimento de voz embarcado a um
   microcontrolador, sem depender de conexão com a internet.
2. Implementar acionamento de dois motores por ponte H com controle de
   velocidade por modulação por largura de pulso.
3. Implementar detecção de obstáculo frontal com parada automática.
4. Especificar e verificar as travas de segurança do sistema.
5. Levantar experimentalmente a taxa de acerto do reconhecimento e o
   comportamento do sensor em superfícies diferentes.

## Justificativa

O acionamento por voz interessa a pessoas com mobilidade reduzida nos
membros superiores, para quem o joystick convencional é difícil ou
impossível de operar. O reconhecimento embarcado, feito no próprio módulo,
evita a dependência de servidor e de internet, o que reduz o atraso de
resposta e mantém o funcionamento em qualquer ambiente. Em contrapartida,
exige treinamento prévio da voz do usuário e limita o vocabulário.

## Materiais

| Item | Função no protótipo |
|------|--------------------|
| Placa compatível Arduino Mega 2560 + ESP8266 (CH340) | unidade de processamento; quatro portas seriais em hardware atendem voz, depuração e Wi-Fi ao mesmo tempo |
| Módulo de Reconhecimento de Voz V3 | reconhecimento embarcado, até 80 registros gravados e 7 ativos |
| Ponte H dupla L298N | aciona os dois motores nos dois sentidos com controle de velocidade |
| Sensor de obstáculo reflexivo infravermelho | detecção de obstáculo frontal |
| 2 motores DC 3-6 V com roda de 68 mm | tração diferencial |
| Buzzer, LED e botão de emergência | sinalização e parada manual |

## Metodologia

O desenvolvimento seguiu quatro etapas.

**Primeira: separação entre lógica e hardware.** A máquina de estados que
decide o movimento foi escrita sem depender da biblioteca do Arduino, em
`controle.cpp`. As camadas que tocam o hardware ficaram isoladas em módulos
próprios: `motores.cpp` para a ponte H, `sensor_obstaculo.cpp` para o
infravermelho e `vr3.cpp` para o módulo de voz.

**Segunda: verificação automatizada.** Como a lógica não depende do
Arduino, ela é compilada e testada no computador. Foram escritos 40 testes
cobrindo cada regra de segurança. Essa etapa encontrou um defeito real: na
desaceleração, o valor de PWM pulava de positivo para negativo sem passar
por zero, o que invertia a polaridade da ponte H com o motor ainda girando.

**Terceira: integração.** Montagem da fiação, treinamento dos sete comandos
de voz e ajuste dos parâmetros de velocidade mínima e de compensação de
trajetória, com o protótipo montado e no piso de uso.

**Quarta: ensaios.** Medição de taxa de acerto, distância de parada, alcance
do sensor por cor de superfície e verificação de cada trava de segurança,
conforme o roteiro em `docs/testes_e_validacao.md`.

## Arquitetura do firmware

O laço principal executa, em cada volta, sempre na mesma ordem:

1. lê o botão de emergência, que tem prioridade sobre tudo;
2. lê o módulo de voz, sem bloquear;
3. lê comandos digitados, usados em bancada e na demonstração;
4. lê o sensor de obstáculo com filtro de ruído;
5. atualiza a máquina de estados;
6. escreve o resultado na ponte H;
7. atualiza sinalização e telemetria.

Nenhuma dessas etapas espera por evento externo. Essa é a razão de o driver
de voz montar o quadro recebido byte a byte em vez de esperar o quadro
inteiro: se o laço parasse para esperar, o sensor de obstáculo e o tempo
limite de segurança parariam junto.

### Máquina de estados

Estados: `PARADA`, `FRENTE`, `RE`, `GIRO_ESQ`, `GIRO_DIR`.

Avanço e ré são contínuos até parar. Os giros são pulsos temporizados: o
comando "direita" gira por um tempo fixo e para sozinho. Essa escolha evita
que uma falha de reconhecimento deixe a cadeira girando sem controle.

### Travas de segurança

| Trava | O que faz | Por que existe |
|-------|-----------|----------------|
| Sensor de obstáculo | para o avanço e recusa novos comandos de frente | evita colisão frontal |
| Homem-morto, 3 s | para sozinha sem comando novo | protege contra falha de reconhecimento e perda de contato com o usuário |
| "Parar" imediato | corta o PWM sem rampa | é o comando de segurança do usuário |
| Botão de emergência | para e mantém parada | funciona mesmo se o reconhecimento falhar por completo |
| Rampa de aceleração | limita a variação de PWM por ciclo | evita tranco, desconfortável e perigoso |
| Passagem por zero | obriga parada antes de inverter | protege a ponte H e o conjunto mecânico |
| Motores desligados primeiro no `setup()` | garante saídas em zero antes de qualquer outra inicialização | a cadeira não parte sozinha ao ligar |
| Ré liberada com obstáculo | o bloqueio vale só para frente | o sensor aponta para frente, e travar a ré prenderia o usuário |

## Limitações do protótipo

Registre com honestidade. Esta seção costuma render boa discussão na banca.

1. **Reconhecimento dependente do locutor.** O módulo compara padrões de
   áudio da voz gravada. Outra pessoa tem taxa de acerto bem menor.
2. **Vocabulário de sete comandos.** É o limite do reconhecedor ativo.
3. **Sensibilidade a ruído.** O ruído dos próprios motores prejudica o
   reconhecimento, o que motiva treinar com a cadeira parada.
4. **Sensor infravermelho limitado.** Não enxerga bem superfícies pretas
   foscas, vidro e objetos muito inclinados, e não detecta degraus para
   baixo.
5. **Um único sensor frontal.** Não há proteção lateral nem traseira.
6. **Escala reduzida.** Motores de 3-6 V com roda de 68 mm não movem uma
   cadeira real com pessoa. O protótipo demonstra o princípio de controle,
   não a capacidade de carga.
7. **Sem malha fechada.** Não há encoder, então não se mede a velocidade
   real nem o ângulo girado. O giro é por tempo, e o resultado varia com o
   piso e com a carga da bateria.

## Trabalhos futuros

- Encoders nas rodas para controle em malha fechada e giro por ângulo.
- Sensor ultrassônico junto do infravermelho, cobrindo as fraquezas de cada
  um, mais sensores laterais e traseiro.
- Sensor apontado para baixo, para detectar degrau.
- Confirmação por voz nos comandos de avanço, no modelo "frente" seguido de
  "confirma".
- Uso do ESP8266 para registro remoto de telemetria e acompanhamento por
  cuidador, mantendo o Wi-Fi fora do caminho de segurança.
- Estudo com mais de um locutor para medir a queda de desempenho.

## Como citar o funcionamento no texto

Trecho de exemplo, para adaptar:

> O sistema opera em malha aberta com supervisão por sensor. A cada volta do
> laço principal, o microcontrolador consulta o botão de emergência, o
> módulo de reconhecimento e o sensor de obstáculo, atualiza a máquina de
> estados e escreve o resultado na ponte H. O tempo de resposta é
> determinado pelo módulo de reconhecimento, e não pelo laço de controle,
> que executa em ordem de milissegundos.
