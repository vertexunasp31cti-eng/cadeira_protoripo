# MindMovie — site do projeto

Landing page de apresentação do projeto conceitual **MindMovie**: uma
cadeira de rodas inteligente controlada por ondas cerebrais (BCI). É um site
estático (HTML/CSS/JS puro, sem build), com cinco seções navegáveis em uma
única página:

- **Visão geral** — apresentação do projeto.
- **Como utilizar** — passo a passo de uso e vídeo demonstrativo.
- **Componentes** — tabela de componentes, funções e custo estimado.
- **Referências** — base científica do projeto.
- **Especificações** — a equipe (“Quem somos”) e formulário de contato.

## Como visualizar

Não há dependências nem build. Basta servir a pasta com qualquer servidor
estático, por exemplo:

```bash
cd site
python3 -m http.server 8080
```

Depois acesse `http://localhost:8080`. Também funciona abrindo
`index.html` diretamente no navegador.

## Recursos

- **Responsivo**: layout mobile-first, com menu hambúrguer abaixo de 860px
  e grades que se reorganizam em colunas a partir daí.
- **Tema claro/escuro**: alternável pelo botão de sol/lua no cabeçalho,
  com preferência salva em `localStorage`. O tema padrão é escuro, como no
  design original.
- **Navegação por seções**: os links do menu trocam de seção sem recarregar
  a página, e o hash da URL acompanha a seção ativa.
- **Busca simples**: o ícone de lupa expande um campo de busca por texto
  que pula para a primeira seção onde o termo aparece.
- **Formulário de contato**: validação client-side, sem backend. Para
  ativar o recebimento de mensagens é preciso ligar o formulário a um
  serviço próprio (endpoint HTTP, Formspree, etc.) em `js/app.js`.

## Estrutura

```
site/
  index.html       marcação de todas as seções
  css/styles.css    tema, layout e responsividade
  js/app.js         tema, navegação, menu mobile, busca e formulário
```

## Observações

- A ilustração da cadeira de rodas é uma peça de arte vetorial (SVG)
  original, criada para representar o conceito (sensor neural, cadeira e
  módulo de controle), já que não há fotos reais do protótipo neste
  repositório.
- Os avatares da equipe em “Especificações” são iniciais sobre fundo
  gradiente, como placeholder até que fotos reais estejam disponíveis.
