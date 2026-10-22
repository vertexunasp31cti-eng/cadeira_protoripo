// ARQUIVO GERADO, nao edite; edite app/index.html e rode python3 ferramentas/gerar_pagina.py
//
// App da cadeira embutido no firmware do ESP8266. A pagina fica na flash
// (PROGMEM) e e enviada inteira em GET /.
// Tamanho: 104760 bytes.
#ifndef PAGINA_H
#define PAGINA_H

#include <Arduino.h>

static const char PAGINA_HTML[] PROGMEM = R"PAGINA_HTML(<!doctype html>
<html lang="pt-BR">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover">
<title>Cadeira por Voz</title>
<meta name="claude:viewport-fit-sides" content="cover">
<meta name="description" content="Acompanhe e comande pelo celular a cadeira de rodas por voz do TCC de Robótica.">
<meta name="theme-color" content="#ffffff" media="(prefers-color-scheme: light)">
<meta name="theme-color" content="#151c24" media="(prefers-color-scheme: dark)">
<link rel="manifest" href="/manifest.webmanifest">
<style>
/* Layout: painel de maquina. Leitura da cadeira em cima (a esquerda no PC), comandos embaixo (a direita), PARAR sempre preso a borda da tela. */
:root {
  /* paleta: chapa metalica fria e azul de sinalizacao (o mesmo do icone do app) */
  --fundo: #edf0f4;
  --superficie: #ffffff;
  --superficie-2: #e2e7ee;
  --tinta: #111a24;
  --tinta-2: #4a5868;
  --linha: #c5cfdb;
  --destaque: #1d4ed8;
  --destaque-tinta: #ffffff;
  --destaque-fundo: #dfe7fb;
  /* semanticas, separadas do destaque */
  --ok: #157a3a;
  --ok-fundo: #dbf1e2;
  --alerta: #835300;
  --alerta-fundo: #ffd53d;
  --alerta-tinta: #221900;
  --critico: #c0121a;
  --critico-fundo: #c0121a;
  --critico-tinta: #ffffff;
  /* parada de emergencia: cogumelo vermelho sobre colar amarelo (convencao de maquina) */
  --parar: #d3121b;
  --parar-sombra: #7a070c;
  --parar-brilho: #ff6d72;
  --parar-tinta: #ffffff;
  --colar: #f7c600;
  --colar-borda: #8f7200;
  --sombra: rgba(17, 26, 36, .16);
  --faixa-demo: #26313d;
  /* tipos: DIN de sinalizacao industrial nos titulos, fonte do sistema no texto, mono nos numeros */
  --fonte-titulo: "Bahnschrift", "DIN Alternate", "DIN Next", "D-DIN", "Roboto Condensed", "sans-serif-condensed", "Arial Narrow", system-ui, sans-serif;
  --fonte-texto: system-ui, -apple-system, "Segoe UI", Roboto, "Noto Sans", "Helvetica Neue", Arial, sans-serif;
  --fonte-dados: ui-monospace, "SF Mono", "Cascadia Mono", "Roboto Mono", Menlo, Consolas, "Liberation Mono", monospace;
  --largura: 1180px;
  --topo-h: 58px;
  --abas-h: 60px;
  --dock-h: 94px;
  color-scheme: light;
}
@media (prefers-color-scheme: dark) {
  :root:not([data-theme="light"]) {
    --fundo: #0c1117; --superficie: #151c24; --superficie-2: #1e2731; --tinta: #e8edf2; --tinta-2: #9eacbb; --linha: #2c3846;
    --destaque: #8cb0ff; --destaque-tinta: #07142e; --destaque-fundo: #18284a;
    --ok: #5ad48d; --ok-fundo: #12301f; --alerta: #f4c84a; --alerta-fundo: #ebbb1f; --alerta-tinta: #1c1500;
    --critico: #ff7c7c; --critico-fundo: #b3121a; --critico-tinta: #ffffff;
    --parar: #e11d26; --parar-sombra: #5c0408; --parar-brilho: #ff7a7f; --parar-tinta: #ffffff; --colar: #efbf00; --colar-borda: #6f5800;
    --sombra: rgba(0, 0, 0, .55); --faixa-demo: #c9d3dd;
    color-scheme: dark;
  }
}
:root[data-theme="dark"] {
  --fundo: #0c1117; --superficie: #151c24; --superficie-2: #1e2731; --tinta: #e8edf2; --tinta-2: #9eacbb; --linha: #2c3846;
  --destaque: #8cb0ff; --destaque-tinta: #07142e; --destaque-fundo: #18284a;
  --ok: #5ad48d; --ok-fundo: #12301f; --alerta: #f4c84a; --alerta-fundo: #ebbb1f; --alerta-tinta: #1c1500;
  --critico: #ff7c7c; --critico-fundo: #b3121a; --critico-tinta: #ffffff;
  --parar: #e11d26; --parar-sombra: #5c0408; --parar-brilho: #ff7a7f; --parar-tinta: #ffffff; --colar: #efbf00; --colar-borda: #6f5800;
  --sombra: rgba(0, 0, 0, .55); --faixa-demo: #c9d3dd;
  color-scheme: dark;
}

*, *::before, *::after { box-sizing: border-box; }
[hidden] { display: none !important; }
html { -webkit-text-size-adjust: 100%; text-size-adjust: 100%; scroll-padding-top: calc(var(--cabecalho, 59px) + 12px); scroll-padding-bottom: calc(var(--abas-h) + var(--dock-h) + env(safe-area-inset-bottom, 0px) + 12px); }
body { margin: 0; background: var(--fundo); color: var(--tinta); font: 16px/1.5 var(--fonte-texto); -webkit-font-smoothing: antialiased; }
body::before { content: ""; position: fixed; inset: 0 0 auto; height: env(safe-area-inset-top, 0px); z-index: 40; background: var(--superficie); }
button { font: inherit; color: inherit; cursor: pointer; touch-action: manipulation; -webkit-tap-highlight-color: transparent; }
button:disabled { cursor: not-allowed; }
/* algarismos de largura fixa so onde numeros mudam ou se alinham (na fonte toda, alguns sistemas alargam o hifen) */
.roda-pct, .roda-pwm, .nivel, .dpad-centro, .vel-texto, .diag dd, .ev time, .hist-cab span, .tabela td:first-child, .travas li::before, .seq, .pilula { font-variant-numeric: tabular-nums; }
/* sem menu de toque longo nem selecao de texto nos botoes de conducao */
.tecla, .parar { user-select: none; -webkit-user-select: none; -webkit-touch-callout: none; }
:focus-visible { outline: 3px solid var(--destaque); outline-offset: 2px; }
code, kbd { font-family: var(--fonte-dados); font-size: .92em; }
kbd { display: inline-block; min-width: 1.7em; padding: 0 .35em; border: 1px solid var(--linha); border-bottom-width: 2px; border-radius: 5px; background: var(--superficie); text-align: center; line-height: 1.5; }
.ic { width: 20px; height: 20px; flex: none; fill: none; stroke: currentColor; stroke-width: 2; stroke-linecap: round; stroke-linejoin: round; }
.ic-cheio { fill: currentColor; stroke: none; }
.rotulo { margin: 0; font: 700 12px/1.3 var(--fonte-texto); letter-spacing: .09em; text-transform: uppercase; color: var(--tinta-2); }

/* ---------- topo ---------- */
.topo { position: sticky; top: env(safe-area-inset-top, 0px); z-index: 30; background: var(--superficie); border-bottom: 1px solid var(--linha); }
.topo-interno { max-width: var(--largura); margin: 0 auto; min-height: var(--topo-h); display: flex; align-items: center; gap: 12px; padding-left: max(16px, env(safe-area-inset-left, 0px)); padding-right: max(16px, env(safe-area-inset-right, 0px)); padding-block: 6px; }
.marca { display: flex; align-items: center; gap: 10px; margin-right: auto; min-width: 0; }
.marca svg { width: 32px; height: 32px; flex: none; }
.marca-fundo { fill: var(--destaque); }
.marca-tinta { fill: var(--destaque-tinta); }
.marca h1 { margin: 0; min-width: 0; overflow: hidden; text-overflow: ellipsis; font: 700 19px/1.1 var(--fonte-titulo); letter-spacing: .01em; white-space: nowrap; }
.pilula { display: inline-flex; align-items: center; gap: 8px; min-height: 34px; padding: 4px 12px; border-radius: 999px; border: 1px solid var(--linha); background: var(--superficie-2); color: var(--tinta); font-size: 14px; font-weight: 650; white-space: nowrap; flex: none; }
.pilula::before { content: ""; width: 9px; height: 9px; border-radius: 50%; background: var(--tinta-2); flex: none; }
.pilula.ok { background: var(--ok-fundo); border-color: transparent; color: var(--ok); }
.pilula.ok::before { background: var(--ok); box-shadow: 0 0 0 3px var(--ok-fundo), 0 0 0 4px var(--ok); }
.pilula.alerta { background: var(--alerta-fundo); border-color: transparent; color: var(--alerta-tinta); }
.pilula.alerta::before { background: var(--alerta-tinta); }
.pilula.off { background: var(--superficie); border-color: var(--critico); color: var(--critico); }
.pilula.off::before { background: var(--critico); }
.pilula.demo { background: var(--faixa-demo); border-color: transparent; color: var(--fundo); }
.pilula.demo::before { background: var(--fundo); border-radius: 2px; }

/* abas: no celular ficam presas embaixo; no PC sobem para o topo */
.abas { position: fixed; left: 0; right: 0; bottom: 0; z-index: 26; display: grid; grid-template-columns: repeat(3, minmax(0, 1fr)); background: var(--superficie); border-top: 1px solid var(--linha); padding-left: max(8px, env(safe-area-inset-left, 0px)); padding-right: max(8px, env(safe-area-inset-right, 0px)); padding-bottom: env(safe-area-inset-bottom, 0px); }
.aba { position: relative; min-height: var(--abas-h); display: flex; flex-direction: column; align-items: center; justify-content: center; gap: 3px; border: 0; background: none; color: var(--tinta-2); font-size: 12.5px; font-weight: 650; }
.aba .ic { width: 22px; height: 22px; }
.aba[aria-selected="true"] { color: var(--destaque); }
.aba[aria-selected="true"]::before { content: ""; position: absolute; top: 0; left: 50%; width: 36px; height: 3px; margin-left: -18px; border-radius: 0 0 3px 3px; background: var(--destaque); }

/* faixa da demonstracao: inversa ao tema, listrada, impossivel de confundir com dado real */
.faixa-demo { background: repeating-linear-gradient(-45deg, var(--faixa-demo) 0 16px, var(--tinta-2) 16px 18px); color: var(--fundo); }
.faixa-demo-interno { max-width: var(--largura); margin: 0 auto; display: grid; grid-template-columns: minmax(0, 1fr) auto; align-items: center; gap: 12px; padding-left: max(16px, env(safe-area-inset-left, 0px)); padding-right: max(16px, env(safe-area-inset-right, 0px)); padding-block: 8px; }
.faixa-demo p { margin: 0; font-size: 14px; line-height: 1.35; }
.faixa-demo strong { font: 700 15px/1.2 var(--fonte-titulo); letter-spacing: .04em; text-transform: uppercase; }
.botao-faixa { min-height: 48px; max-width: 9.5em; padding: 4px 12px; border-radius: 12px; border: 2px solid currentColor; background: var(--faixa-demo); color: inherit; font-size: 14px; font-weight: 700; line-height: 1.15; }

/* ---------- pagina ---------- */
.pagina { max-width: var(--largura); margin: 0 auto; display: grid; gap: 16px; padding-left: max(16px, env(safe-area-inset-left, 0px)); padding-right: max(16px, env(safe-area-inset-right, 0px)); padding-block: 16px calc(var(--abas-h) + var(--dock-h) + env(safe-area-inset-bottom, 0px) + 28px); }
.botao { min-height: 48px; padding: 0 18px; border-radius: 12px; border: 0; background: var(--destaque); color: var(--destaque-tinta); font-weight: 700; display: inline-flex; align-items: center; justify-content: center; gap: 8px; }
.botao-sec { min-height: 48px; padding: 0 16px; border-radius: 12px; border: 1.5px solid var(--linha); background: var(--superficie); color: var(--tinta); font-weight: 650; display: inline-flex; align-items: center; gap: 8px; }
.botao-sec:disabled { opacity: .5; }

/* aviso de conexao (vale para todas as abas) */
.aviso { display: flex; gap: 14px; align-items: flex-start; padding: 16px; border-radius: 16px; background: var(--superficie); border: 2px solid var(--linha); }
.aviso.alerta { border-color: var(--alerta); }
.aviso.off { border-color: var(--critico); }
.aviso-ic { width: 44px; height: 44px; border-radius: 12px; display: grid; place-items: center; flex: none; background: var(--superficie-2); color: var(--tinta-2); }
.aviso.alerta .aviso-ic { background: var(--alerta-fundo); color: var(--alerta-tinta); }
.aviso.off .aviso-ic { background: var(--critico-fundo); color: var(--critico-tinta); }
.aviso-corpo { display: grid; gap: 6px; min-width: 0; }
.aviso h2 { margin: 0; font: 700 20px/1.2 var(--fonte-titulo); text-wrap: balance; }
.aviso p { margin: 0; color: var(--tinta-2); }
.aviso p strong, .aviso p code { color: var(--tinta); }
.aviso-acoes { display: flex; flex-wrap: wrap; gap: 10px; align-items: center; margin-top: 4px; }
.tentando { font-size: 13.5px; color: var(--tinta-2); }

/* ---------- painel ---------- */
/* no celular as colunas se desfazem: resumo, comandos e so depois o detalhe,
   para o estado ficar a vista enquanto o dedo segura o direcional */
.painel { display: grid; gap: 16px; }
.col { display: contents; }
.alertas { order: 1; } .cartao-estado { order: 2; } .cartao-controle { order: 3; } .cartao-sim { order: 4; } .cartao-medidas { order: 5; } .diag { order: 6; }
.alertas { display: grid; gap: 10px; }
.alertas.sem-alerta { position: absolute; }
.faixa { display: flex; gap: 12px; align-items: flex-start; padding: 14px 16px; border-radius: 14px; }
.faixa .ic { width: 26px; height: 26px; margin-top: 1px; }
.faixa div { display: grid; gap: 2px; min-width: 0; }
.faixa strong { font: 700 19px/1.2 var(--fonte-titulo); letter-spacing: .05em; }
.faixa span { font-size: 15px; line-height: 1.4; }
.faixa-critico { background: var(--critico-fundo); color: var(--critico-tinta); }
.faixa-alerta { background: var(--alerta-fundo); color: var(--alerta-tinta); }
.faixa-info { background: var(--superficie); border: 1px solid var(--linha); }
.faixa-info strong { font-size: 16px; letter-spacing: .02em; }
.faixa-info span { color: var(--tinta-2); }

.cartao-estado { background: var(--superficie); border: 1px solid var(--linha); border-radius: 18px; padding: 16px; display: grid; gap: 10px; transition: border-color .2s; }
.cartao-estado.movendo { border-color: var(--destaque); }
.cartao-estado.emergencia { border-color: var(--critico); }
.cartao-estado.desatualizado { border-color: var(--linha); }
.cartao-estado.desatualizado .estado-ic { background: var(--superficie-2); color: var(--tinta-2); }
.cartao-estado.desatualizado .estado-nome { color: var(--tinta-2); }
.estado-topo { display: flex; gap: 14px; align-items: center; }
.estado-ic { width: 54px; height: 54px; border-radius: 15px; display: grid; place-items: center; flex: none; background: var(--superficie-2); color: var(--tinta-2); transition: background .2s, color .2s; }
.estado-ic .ic { width: 28px; height: 28px; stroke-width: 2.4; }
.cartao-estado.movendo .estado-ic { background: var(--destaque); color: var(--destaque-tinta); }
.cartao-estado.emergencia .estado-ic { background: var(--critico-fundo); color: var(--critico-tinta); }
.estado-txt { min-width: 0; display: grid; gap: 2px; }
.estado-nome { margin: 0; font: 700 clamp(25px, 7.2vw, 34px)/1.08 var(--fonte-titulo); letter-spacing: .005em; text-wrap: balance; }
.ultima { margin: 2px 0 0; color: var(--tinta-2); font-size: 15px; line-height: 1.35; }
.velho { justify-self: start; display: inline-flex; align-items: center; gap: 6px; padding: 3px 10px; border-radius: 999px; background: var(--superficie-2); color: var(--tinta-2); font-size: 13px; font-weight: 650; }
.cartao-medidas.desatualizado > * { opacity: .45; }
.cartao-medidas { background: var(--superficie); border: 1px solid var(--linha); border-radius: 18px; padding: 16px; display: grid; gap: 18px; container-type: inline-size; }
.cartao-medidas > * { transition: opacity .2s; }
.tracao { display: grid; grid-template-columns: minmax(0, 1fr) auto minmax(0, 1fr); align-items: center; gap: 10px; padding: 6px 0; }
.roda { display: flex; align-items: center; gap: 10px; min-width: 0; }
.roda-esq { justify-content: flex-end; text-align: right; }
.roda-txt { display: grid; gap: 1px; min-width: 0; }
.roda-pct { font: 700 27px/1 var(--fonte-titulo); }
.roda-sent { font-size: 14.5px; font-weight: 600; }
.roda-pwm { font: 500 12px/1.3 var(--fonte-dados); color: var(--tinta-2); white-space: nowrap; }
.trilho { position: relative; width: 14px; height: 116px; flex: none; border-radius: 7px; background: var(--superficie-2); border: 1px solid var(--linha); overflow: hidden; }
.trilho::after { content: ""; position: absolute; left: 0; right: 0; top: 50%; height: 2px; margin-top: -1px; background: var(--tinta-2); }
.barra { position: absolute; left: 0; right: 0; bottom: 50%; height: 0; background: var(--destaque); transition: height .15s linear; }
.barra.re { bottom: auto; top: 50%; }
.cadeira { width: clamp(84px, 25vw, 118px); height: auto; display: block; overflow: visible; }
.cadeira .cone { fill: var(--linha); opacity: .7; transition: fill .2s; }
.cadeira .sensor { fill: var(--tinta-2); }
.cadeira.obst .cone { fill: var(--alerta-fundo); opacity: 1; }
.cadeira.obst .sensor { fill: var(--alerta); }
.cadeira .pe, .cadeira .rodizio { fill: var(--superficie-2); stroke: var(--tinta-2); stroke-width: 1.5; }
.cadeira .assento { fill: var(--superficie-2); stroke: var(--tinta-2); stroke-width: 2; }
.cadeira .encosto { fill: var(--tinta-2); }
.cadeira .eixo { stroke: var(--tinta-2); stroke-width: 1.5; stroke-dasharray: 3 3; }
.cadeira .roda-svg { fill: var(--tinta); transition: fill .2s; }
.cadeira .roda-svg.gira { fill: var(--destaque); }
.cadeira .chev { fill: none; stroke: var(--destaque-tinta); stroke-width: 2.4; stroke-linecap: round; stroke-linejoin: round; transform-box: fill-box; transform-origin: center; }
.cadeira .chev.re { transform: rotate(180deg); }
.cadeira .seta { display: none; fill: none; stroke: var(--destaque); stroke-width: 3.2; stroke-linecap: round; }
.cadeira[data-estado="FRENTE"] .seta-FRENTE, .cadeira[data-estado="RE"] .seta-RE, .cadeira[data-estado="GIRO_ESQ"] .seta-GIRO_ESQ, .cadeira[data-estado="GIRO_DIR"] .seta-GIRO_DIR { display: inline; }
.ponta-cadeira { fill: var(--destaque); }
.legenda { margin: -8px 0 0; font-size: 13.5px; color: var(--tinta-2); text-align: center; text-wrap: balance; }

.vel { display: grid; gap: 8px; }
.vel-texto { font: 700 18px/1.2 var(--fonte-titulo); }
.vel-niveis { display: grid; grid-template-columns: repeat(3, minmax(0, 1fr)); gap: 6px; }
.nivel { display: flex; justify-content: space-between; align-items: baseline; gap: 4px; padding: 8px 10px; border-radius: 10px; border: 1px solid var(--linha); background: var(--superficie-2); color: var(--tinta-2); }
.nivel b { font: 700 18px/1 var(--fonte-titulo); color: var(--tinta); }
.nivel small { font: 500 12px/1 var(--fonte-dados); }
.nivel.on { background: var(--destaque-fundo); border-color: var(--destaque); color: var(--destaque); }
.nivel.on b { color: var(--destaque); }
.nivel.atual { background: var(--destaque); border-color: var(--destaque); color: var(--destaque-tinta); }
.nivel.atual b { color: var(--destaque-tinta); }

.sensores { margin: 0; display: grid; border-top: 1px solid var(--linha); }
.sensores div { display: flex; align-items: center; justify-content: space-between; gap: 12px; min-height: 48px; padding-top: 8px; border-bottom: 1px solid var(--linha); padding-bottom: 8px; }
.sensores div:last-child { border-bottom: 0; padding-bottom: 0; }
.sensores dt { font-weight: 600; }
.sensores dd { margin: 0; }
.chip { display: inline-flex; align-items: center; gap: 7px; min-height: 34px; padding: 5px 12px 5px 10px; border-radius: 999px; background: var(--superficie-2); color: var(--tinta); font-size: 14px; font-weight: 650; }
.chip .ic { width: 18px; height: 18px; }
.chip.ok { background: var(--ok-fundo); color: var(--ok); }
.chip.alerta { background: var(--alerta-fundo); color: var(--alerta-tinta); }

/* comandos */
.cartao-controle { background: var(--superficie-2); border-radius: 18px; padding: 16px; display: grid; gap: 14px; }
.trava { width: 100%; min-height: 62px; display: flex; align-items: center; gap: 12px; padding: 10px 14px; border-radius: 14px; border: 1px solid var(--linha); background: var(--superficie); text-align: left; }
.trava > .ic { width: 24px; height: 24px; color: var(--tinta-2); }
.trava[aria-checked="true"] > .ic { color: var(--destaque); }
.trava-txt { flex: 1; display: grid; min-width: 0; }
.trava-rot { font-weight: 700; font-size: 17px; line-height: 1.25; }
.trava-est { font-size: 14px; color: var(--tinta-2); }
.chave { position: relative; width: 54px; height: 32px; flex: none; border-radius: 16px; background: var(--linha); transition: background .15s; }
.chave::after { content: ""; position: absolute; top: 4px; left: 4px; width: 24px; height: 24px; border-radius: 50%; background: var(--tinta-2); transition: transform .15s, background .15s; }
.trava[aria-checked="true"] .chave { background: var(--destaque); }
.trava[aria-checked="true"] .chave::after { transform: translateX(22px); background: var(--destaque-tinta); }
.trava:disabled { opacity: .6; }
.nota { margin: 0; font-size: 14.5px; color: var(--tinta-2); }
.nota-app { display: flex; gap: 10px; align-items: flex-start; margin: 0; padding: 12px 14px; border-radius: 12px; background: var(--superficie); border: 1px solid var(--linha); font-size: 14.5px; }
.nota-app .ic { color: var(--alerta); margin-top: 1px; }
.nota-app code { font-size: 13px; overflow-wrap: anywhere; }
.comandos { display: grid; gap: 14px; }
.dica { margin: 0; font-weight: 650; text-align: center; text-wrap: balance; }
.dpad { display: grid; grid-template-columns: repeat(3, minmax(0, 1fr)); grid-template-areas: ". f ." "e c d" ". r ."; gap: 10px; width: 100%; max-width: 330px; margin: 0 auto; }
.tecla { min-height: 74px; min-width: 0; padding: 6px 4px; border-radius: 16px; border: 2px solid var(--linha); background: var(--superficie); color: var(--tinta); display: flex; flex-direction: column; align-items: center; justify-content: center; gap: 4px; font-size: 15px; font-weight: 650; line-height: 1.1; touch-action: none; }
.tecla .ic { width: 30px; height: 30px; stroke-width: 2.4; pointer-events: none; }
.tecla span { pointer-events: none; }
.tecla.seg { border-color: var(--destaque); color: var(--destaque); }
.tecla.ativo { background: var(--destaque); border-color: var(--destaque); color: var(--destaque-tinta); animation: renova .4s linear infinite; }
.tecla:disabled { opacity: .4; }
.t-f { grid-area: f; } .t-e { grid-area: e; } .t-d { grid-area: d; } .t-r { grid-area: r; }
.dpad-centro { grid-area: c; display: grid; place-content: center; text-align: center; border-radius: 50%; color: var(--tinta-2); font-size: 12px; line-height: 1.2; }
.dpad-centro b { font: 700 26px/1 var(--fonte-titulo); color: var(--tinta); }
.velocidades { display: grid; grid-template-columns: 1fr 1fr; gap: 10px; width: 100%; max-width: 330px; margin: 0 auto; }
.velocidades .tecla { min-height: 54px; flex-direction: row; gap: 8px; }
.velocidades .tecla .ic { width: 22px; height: 22px; }
.atalhos { margin: 0 auto; padding: 0; list-style: none; display: grid; grid-template-columns: auto auto; justify-content: center; gap: 6px 22px; font-size: 13.5px; color: var(--tinta-2); }
.atalhos li { display: flex; align-items: center; gap: 4px; }
.atalhos kbd { margin-right: 2px; }
@keyframes renova { 0% { box-shadow: 0 0 0 0 var(--destaque); } 100% { box-shadow: 0 0 0 9px transparent; } }

.cartao-sim { border: 2px dashed var(--tinta-2); border-radius: 18px; padding: 16px; display: grid; gap: 12px; }
.cartao-sim h2 { margin: 0; font: 700 17px/1.2 var(--fonte-titulo); }
.cartao-sim p { margin: 0; font-size: 14.5px; color: var(--tinta-2); }
.sim-botoes { display: grid; grid-template-columns: 1fr 1fr; gap: 10px; }
.sim { min-height: 56px; padding: 8px 10px; border-radius: 12px; border: 2px solid var(--linha); background: var(--superficie); display: flex; align-items: center; justify-content: center; gap: 8px; font-weight: 650; line-height: 1.2; text-align: left; }
#sim-obstaculo[aria-pressed="true"] { background: var(--alerta-fundo); border-color: var(--alerta-fundo); color: var(--alerta-tinta); }
#sim-emergencia[aria-pressed="true"] { background: var(--critico-fundo); border-color: var(--critico-fundo); color: var(--critico-tinta); }

.diag { border-top: 1px solid var(--linha); }
.diag summary { min-height: 48px; display: flex; align-items: center; gap: 8px; cursor: pointer; font-weight: 650; color: var(--tinta-2); list-style: none; }
.diag summary::-webkit-details-marker { display: none; }
.diag summary .ic { transition: transform .15s; }
.diag[open] summary .ic { transform: rotate(90deg); }
.diag dl { display: grid; grid-template-columns: minmax(0, 1fr) auto; gap: 6px 16px; margin: 0 0 8px; font-size: 14px; }
.diag dt { color: var(--tinta-2); }
.diag dd { margin: 0; text-align: right; font-family: var(--fonte-dados); font-size: 13px; }

/* ---------- PARAR ---------- */
.dock { position: fixed; left: 0; right: 0; bottom: calc(var(--abas-h) + env(safe-area-inset-bottom, 0px)); z-index: 25; background: var(--superficie); border-top: 1px solid var(--linha); box-shadow: 0 -8px 24px var(--sombra); }
.dock-interno { max-width: var(--largura); margin: 0 auto; padding-left: max(16px, env(safe-area-inset-left, 0px)); padding-right: max(16px, env(safe-area-inset-right, 0px)); padding-block: 9px 10px; }
.colar { padding: 6px; border-radius: 20px; background: var(--colar); border: 2px solid var(--colar-borda); }
.parar { width: 100%; min-height: 58px; border: 0; border-radius: 14px; display: flex; align-items: center; justify-content: center; gap: 12px; color: var(--parar-tinta); background: radial-gradient(120% 130% at 50% 0%, var(--parar-brilho) 0%, var(--parar) 28%, var(--parar) 70%, var(--parar-sombra) 100%); box-shadow: inset 0 -4px 0 var(--parar-sombra), 0 2px 0 var(--colar-borda); text-shadow: 0 1px 1px var(--parar-sombra); font: 700 27px/1 var(--fonte-titulo); letter-spacing: .16em; }
.parar .ic { width: 26px; height: 26px; }
.parar:active, .parar.acionado { transform: translateY(2px); box-shadow: inset 0 -1px 0 var(--parar-sombra); filter: brightness(.92); }
.parar:focus-visible { outline-color: var(--tinta); outline-offset: 4px; }
.parar-tecla { display: none; }

/* avisos curtos */
.toasts { position: fixed; left: 0; right: 0; bottom: calc(var(--abas-h) + var(--dock-h) + env(safe-area-inset-bottom, 0px) + 10px); z-index: 35; display: flex; justify-content: center; pointer-events: none; padding-left: max(16px, env(safe-area-inset-left, 0px)); padding-right: max(16px, env(safe-area-inset-right, 0px)); }
.toast { display: flex; align-items: flex-start; gap: 10px; max-width: 520px; padding: 12px 16px; border-radius: 14px; background: var(--tinta); color: var(--fundo); font-weight: 600; line-height: 1.35; box-shadow: 0 10px 30px var(--sombra); }
.toast.alerta { background: var(--alerta-fundo); color: var(--alerta-tinta); }
.toast.critico { background: var(--critico-fundo); color: var(--critico-tinta); }

/* ---------- historico ---------- */
.hist-cab { display: flex; align-items: center; gap: 8px 12px; flex-wrap: wrap; }
.hist-cab h2 { margin: 0 auto 0 0; font: 700 26px/1.1 var(--fonte-titulo); }
.hist-nota { margin: 0; font-size: 14px; color: var(--tinta-2); }
.hist { list-style: none; margin: 0; padding: 0; background: var(--superficie); border: 1px solid var(--linha); border-radius: 16px; overflow: hidden; }
.ev { display: grid; grid-template-columns: auto auto minmax(0, 1fr); align-items: center; gap: 12px; min-height: 54px; padding: 9px 14px; border-top: 1px solid var(--linha); }
.ev:first-child { border-top: 0; }
.ev time { font: 500 13.5px/1 var(--fonte-dados); color: var(--tinta-2); }
.ev-ic { width: 34px; height: 34px; border-radius: 10px; display: grid; place-items: center; background: var(--superficie-2); color: var(--tinta-2); }
.ev-ic .ic { width: 19px; height: 19px; }
.ev-txt { overflow-wrap: anywhere; }
.ev-alerta .ev-ic { background: var(--alerta-fundo); color: var(--alerta-tinta); }
.ev-alerta .ev-txt { font-weight: 650; }
.ev-critico .ev-ic { background: var(--critico-fundo); color: var(--critico-tinta); }
.ev-critico .ev-txt { font-weight: 700; color: var(--critico); }
.ev-ok .ev-ic { background: var(--ok-fundo); color: var(--ok); }
.ev-info .ev-ic { background: var(--destaque-fundo); color: var(--destaque); }
.vazio { margin: 0; padding: 28px 18px; border: 1.5px dashed var(--linha); border-radius: 16px; color: var(--tinta-2); text-align: center; }
.historico { display: grid; gap: 14px; max-width: 760px; width: 100%; margin: 0 auto; }

/* ---------- sobre ---------- */
.sobre { max-width: 760px; width: 100%; margin: 0 auto; display: grid; gap: 34px; }
.sobre section { display: grid; gap: 12px; min-width: 0; }
.sobre h2 { margin: 0; font: 700 23px/1.2 var(--fonte-titulo); text-wrap: balance; }
.sobre h3 { margin: 6px 0 0; font: 700 18px/1.25 var(--fonte-titulo); }
.sobre p, .sobre li { max-width: 68ch; }
.sobre p { margin: 0; }
.sobre ul, .sobre ol { margin: 0; padding-left: 1.3em; display: grid; gap: 8px; }
.capa { gap: 14px; padding-bottom: 6px; border-bottom: 1px solid var(--linha); }
.capa h2 { font-size: clamp(30px, 8vw, 44px); line-height: 1.05; }
.capa .lead { font-size: 18px; line-height: 1.5; color: var(--tinta-2); }
.diagrama { margin: 0; padding: 12px; border-radius: 16px; background: var(--superficie); border: 1px solid var(--linha); overflow-x: auto; }
.diagrama svg { display: block; width: 100%; min-width: 300px; max-width: 540px; height: auto; margin: 0 auto; }
.diagrama figcaption { margin-top: 8px; font-size: 14px; color: var(--tinta-2); text-align: center; text-wrap: balance; }
.d-caixa { fill: var(--superficie); stroke: var(--tinta-2); stroke-width: 1.5; }
.d-mega { fill: var(--destaque-fundo); stroke: var(--destaque); stroke-width: 2; }
.d-rede { fill: var(--superficie); stroke: var(--tinta-2); stroke-width: 1.5; stroke-dasharray: 5 4; }
.d-regiao { fill: none; stroke: var(--linha); stroke-width: 1.5; stroke-dasharray: 2 4; }
.d-seta { fill: none; stroke: var(--tinta-2); stroke-width: 1.6; }
.d-ponta { fill: var(--tinta-2); }
.d-tit { fill: var(--tinta); font: 650 12.5px var(--fonte-texto); }
.d-sub { fill: var(--tinta-2); font: 400 10.5px var(--fonte-texto); }
.d-rot { fill: var(--tinta-2); font: 500 10px var(--fonte-dados); }
.d-mega-tit { fill: var(--tinta); font: 700 15px var(--fonte-texto); }
.rolagem { overflow-x: auto; border: 1px solid var(--linha); border-radius: 14px; background: var(--superficie); }
.tabela { width: 100%; border-collapse: collapse; font-size: 15px; }
.tabela th, .tabela td { padding: 10px 14px; text-align: left; border-top: 1px solid var(--linha); vertical-align: top; }
.tabela thead th { border-top: 0; font-size: 12px; letter-spacing: .08em; text-transform: uppercase; color: var(--tinta-2); background: var(--superficie-2); }
.tabela td:first-child { color: var(--tinta-2); width: 3.5em; }
.tabela td:nth-child(2) { font-weight: 700; white-space: nowrap; }
.travas { list-style: none; padding: 0 !important; counter-reset: trava; gap: 0 !important; border-top: 1px solid var(--linha); }
.travas li { counter-increment: trava; display: grid; grid-template-columns: 1.8em minmax(0, 1fr); gap: 10px; padding: 11px 0; border-bottom: 1px solid var(--linha); max-width: none; }
.travas li::before { content: counter(trava); font: 700 20px/1.15 var(--fonte-titulo); color: var(--critico); }
.pecas { display: grid; gap: 0; margin: 0; border-top: 1px solid var(--linha); }
.pecas div { display: grid; gap: 2px; padding: 10px 0; border-bottom: 1px solid var(--linha); }
.pecas dt { font-weight: 700; }
.pecas dd { margin: 0; color: var(--tinta-2); }
.etapas { counter-reset: etapa; list-style: none; padding: 0 !important; gap: 14px !important; }
.etapas li { counter-increment: etapa; display: grid; grid-template-columns: 2.4em minmax(0, 1fr); gap: 10px; max-width: none; }
.etapas li::before { content: counter(etapa); width: 1.9em; height: 1.9em; border-radius: 50%; display: grid; place-items: center; background: var(--destaque); color: var(--destaque-tinta); font: 700 15px/1 var(--fonte-titulo); }
.etapas b { display: block; }
.defeito { display: grid; gap: 10px; padding: 16px; border-radius: 16px; background: var(--superficie); border: 1px solid var(--linha); }
.defeito h3 { margin: 0; }
.seq { display: flex; flex-wrap: wrap; align-items: center; gap: 6px; margin: 0; font-family: var(--fonte-dados); font-size: 14px; }
.seq span { padding: 3px 8px; border-radius: 7px; background: var(--superficie-2); }
.seq .ruim { background: var(--critico-fundo); color: var(--critico-tinta); }
.seq .bom { background: var(--ok-fundo); color: var(--ok); font-weight: 700; }
.seq em { font-style: normal; font-family: var(--fonte-texto); color: var(--tinta-2); min-width: 4.5em; }
.equipe dl { display: grid; gap: 0; margin: 0; border-top: 1px solid var(--linha); }
.equipe dl div { display: grid; grid-template-columns: minmax(7em, auto) minmax(0, 1fr); gap: 12px; padding: 10px 0; border-bottom: 1px solid var(--linha); }
.equipe dt { color: var(--tinta-2); }
.equipe dd { margin: 0; font-weight: 600; }
.equipe ul { list-style: none; padding: 0; gap: 2px; }

/* ---------- PC ---------- */
@media (hover: hover) and (pointer: fine) {
  .tecla:not(:disabled):hover, .botao-sec:not(:disabled):hover, .sim:hover { border-color: var(--destaque); }
  .aba:hover { color: var(--tinta); }
}
@media (min-width: 700px) and (max-width: 1099px) {
  .pagina > * { width: 100%; max-width: 680px; margin-inline: auto; }
  .dock-interno { max-width: 712px; }
  .abas { padding-left: max(8px, env(safe-area-inset-left, 0px), calc((100% - 712px) / 2)); padding-right: max(8px, env(safe-area-inset-right, 0px), calc((100% - 712px) / 2)); }
}
/* PC: abas no topo e o PARAR num trilho fixo no alto a direita, ao lado dos comandos */
@media (min-width: 1100px) {
  :root { --dock-h: 0px; }
  .abas { position: static; display: flex; gap: 4px; padding: 0; border: 0; background: none; margin-right: 8px; }
  .aba { min-height: 48px; flex-direction: row; gap: 8px; padding: 0 14px; border-radius: 10px; font-size: 15px; }
  .aba .ic { width: 20px; height: 20px; }
  .aba[aria-selected="true"] { background: var(--destaque-fundo); }
  .aba[aria-selected="true"]::before { display: none; }
  .marca { margin-right: 20px; }
  .pilula { margin-left: auto; }
  .pagina { padding-right: calc(max(16px, env(safe-area-inset-right, 0px)) + 184px); padding-block: 24px 48px; gap: 20px; }
  .painel { grid-template-columns: minmax(0, 1.3fr) minmax(340px, 1fr); align-items: start; gap: 24px; }
  .col { display: grid; gap: 20px; align-content: start; min-width: 0; }
  .dock { left: auto; bottom: auto; top: calc(env(safe-area-inset-top, 0px) + var(--cabecalho, 59px) + 24px); right: max(calc(16px + env(safe-area-inset-right, 0px)), calc((100% - var(--largura)) / 2 + 16px)); background: none; border: 0; box-shadow: none; }
  .dock-interno { padding: 0; }
  .colar { width: 156px; height: 156px; padding: 0; display: grid; place-items: center; border-radius: 30px; box-shadow: 0 12px 32px var(--sombra); }
  .parar { width: 122px; height: 122px; min-height: 0; border-radius: 50%; flex-direction: column; gap: 6px; font-size: 21px; letter-spacing: .1em; box-shadow: inset 0 -6px 0 var(--parar-sombra), 0 3px 0 var(--colar-borda); }
  .parar .ic { width: 22px; height: 22px; }
  .parar-tecla { display: block; font: 600 11px/1 var(--fonte-texto); letter-spacing: .06em; opacity: .85; }
  .toasts { bottom: 32px; }
  .botao-faixa { max-width: none; padding-inline: 16px; }
  .cadeira { width: 138px; }
  .trilho { height: 138px; }
  .roda-pct { font-size: 32px; }
}
/* celular deitado: sobra pouca altura. Abas sobem para o topo, o PARAR vira
   coluna na borda direita (sob o polegar) e o painel ganha duas colunas, para
   o estado ficar a vista enquanto o dedo segura Frente ou Re */
@media (orientation: landscape) and (max-height: 500px) and (max-width: 1099px) {
  :root { --dock-h: 0px; --abas-h: 0px; --trilho: 92px; }
  .topo-interno { min-height: 0; padding-block: 4px; }
  .marca { margin-right: 12px; }
  .marca svg, .aba .ic { display: none; }
  .marca h1 { font-size: 17px; }
  .abas { position: static; display: flex; gap: 2px; padding: 0; border: 0; background: none; }
  .aba { min-height: 48px; padding: 0 10px; border-radius: 10px; font-size: 14.5px; white-space: nowrap; }
  .aba[aria-selected="true"] { background: var(--destaque-fundo); }
  .aba[aria-selected="true"]::before { display: none; }
  .pilula { margin-left: auto; }
  .faixa-demo-interno { padding-block: 2px; }
  .faixa-demo p { font-size: 13px; }
  .botao-faixa { max-width: none; }
  .pagina { padding-right: calc(var(--trilho) + env(safe-area-inset-right, 0px) + 16px); padding-block: 12px calc(16px + env(safe-area-inset-bottom, 0px)); gap: 12px; }
  .pagina > * { max-width: none; }
  .pagina > .historico, .pagina > .sobre { max-width: 760px; }
  .cartao-controle { padding: 12px; gap: 12px; }
  .estado-nome { font-size: 26px; }
  .trava { min-height: 50px; padding: 4px 10px; }
  .trava > .ic { display: none; }
  .trava-rot { font-size: 16px; }
  .comandos { gap: 10px; }
  /* a dica desce para baixo do direcional: Frente e Re sobem e cabem na tela */
  .dpad { order: -1; gap: 8px; }
  .dica { font-size: 14.5px; line-height: 1.3; }
  .tecla { min-height: 56px; padding-block: 3px; gap: 2px; font-size: 14px; }
  .tecla .ic { width: 24px; height: 24px; }
  .velocidades .tecla { min-height: 48px; }
  .dock { left: auto; right: 0; top: calc(env(safe-area-inset-top, 0px) + var(--cabecalho, 56px)); bottom: 0; width: calc(var(--trilho) + env(safe-area-inset-right, 0px)); border-top: 0; border-left: 1px solid var(--linha); box-shadow: -8px 0 24px var(--sombra); }
  .dock-interno { height: 100%; max-width: none; padding: 10px max(10px, env(safe-area-inset-right, 0px)) calc(10px + env(safe-area-inset-bottom, 0px)) 10px; }
  .colar { height: 100%; }
  .parar { height: 100%; flex-direction: column; gap: 10px; font-size: 22px; letter-spacing: 0; }
  .parar span:first-of-type { writing-mode: vertical-lr; text-orientation: upright; }
  .toasts { bottom: calc(12px + env(safe-area-inset-bottom, 0px)); justify-content: flex-start; padding-right: calc(50% + var(--trilho) / 2); }
}
@media (orientation: landscape) and (max-height: 500px) and (min-width: 640px) and (max-width: 1099px) {
  .painel { grid-template-columns: repeat(2, minmax(0, 1fr)); align-items: start; gap: 14px; }
  .col { display: grid; gap: 12px; align-content: start; min-width: 0; }
  .cadeira { width: 84px; }
  .trilho { height: 100px; }
  .roda-pct { font-size: 23px; }
}
/* cartao estreito (celular deitado pequeno, tela de 320 px): o desenho sobe e
   as duas rodas ficam lado a lado embaixo, sem o texto do PWM transbordar */
@container (max-width: 279px) {
  .tracao { grid-template-columns: repeat(2, minmax(0, 1fr)); row-gap: 4px; }
  .cadeira { order: -1; grid-column: 1 / -1; justify-self: center; }
}
@media (max-width: 420px) {
  .marca svg { display: none; }
  .marca h1 { font-size: 18px; }
}
@media (max-width: 380px) {
  .marca h1 { font-size: 17px; }
  .pilula { font-size: 13px; padding: 4px 10px; }
  .cartao-estado { padding: 16px 14px; }
  .roda-pct { font-size: 23px; }
}
@media (prefers-reduced-motion: reduce) {
  *, *::before, *::after { transition: none !important; animation: none !important; scroll-behavior: auto !important; }
}
</style>
</head>
<body data-aba="painel">
<svg width="0" height="0" style="position:absolute" aria-hidden="true" focusable="false">
  <defs>
    <marker id="ponta-cadeira" viewBox="0 0 8 8" refX="5" refY="4" markerWidth="4" markerHeight="4" orient="auto"><path class="ponta-cadeira" d="M0 0 8 4 0 8z"/></marker>
    <marker id="ponta-diag" viewBox="0 0 8 8" refX="8" refY="4" markerWidth="8" markerHeight="8" markerUnits="userSpaceOnUse" orient="auto-start-reverse"><path class="d-ponta" d="M0 0 8 4 0 8z"/></marker>
  </defs>
  <symbol id="i-cima" viewBox="0 0 24 24"><path d="M12 20V5M5 11.5 12 4.5l7 7"/></symbol>
  <symbol id="i-baixo" viewBox="0 0 24 24"><path d="M12 4v15M19 12.5l-7 7-7-7"/></symbol>
  <symbol id="i-esq" viewBox="0 0 24 24"><path d="M9 4 4 9l5 5"/><path d="M4 9h9.5a6.5 6.5 0 0 1 0 13H10"/></symbol>
  <symbol id="i-dir" viewBox="0 0 24 24"><path d="m15 4 5 5-5 5"/><path d="M20 9h-9.5a6.5 6.5 0 0 0 0 13H14"/></symbol>
  <symbol id="i-menos" viewBox="0 0 24 24"><path d="M5 12h14"/></symbol>
  <symbol id="i-mais" viewBox="0 0 24 24"><path d="M12 5v14M5 12h14"/></symbol>
  <symbol id="i-fechado" viewBox="0 0 24 24"><rect x="5" y="11" width="14" height="10" rx="2"/><path d="M8 11V7.5a4 4 0 0 1 8 0V11"/></symbol>
  <symbol id="i-aberto" viewBox="0 0 24 24"><rect x="5" y="11" width="14" height="10" rx="2"/><path d="M8 11V7.5a4 4 0 0 1 7.6-1.8"/></symbol>
  <symbol id="i-mic" viewBox="0 0 24 24"><rect x="9" y="3" width="6" height="11" rx="3"/><path d="M5.5 11a6.5 6.5 0 0 0 13 0M12 17.5V21"/></symbol>
  <symbol id="i-celular" viewBox="0 0 24 24"><rect x="7" y="2.5" width="10" height="19" rx="2.5"/><path d="M11 18h2"/></symbol>
  <symbol id="i-teclado" viewBox="0 0 24 24"><rect x="2.5" y="6" width="19" height="12" rx="2"/><path d="M6.5 10h.01M10 10h.01M14 10h.01M17.5 10h.01M7.5 14h9"/></symbol>
  <symbol id="i-ligar" viewBox="0 0 24 24"><path d="M12 3v8"/><path d="M17.7 6.8a8 8 0 1 1-11.4 0"/></symbol>
  <symbol id="i-parada" viewBox="0 0 24 24"><rect x="6" y="6" width="12" height="12" rx="2"/></symbol>
  <symbol id="i-triangulo" viewBox="0 0 24 24"><path d="M10.3 4.2 2.6 18a2 2 0 0 0 1.7 3h15.4a2 2 0 0 0 1.7-3L13.7 4.2a2 2 0 0 0-3.4 0z"/><path d="M12 10v4M12 17.3v.01"/></symbol>
  <symbol id="i-octogono" viewBox="0 0 24 24"><path d="M8.1 2.5h7.8l5.6 5.6v7.8l-5.6 5.6H8.1l-5.6-5.6V8.1z"/><path d="M12 7.5v5.5M12 16.4v.01"/></symbol>
  <symbol id="i-pare" viewBox="0 0 24 24"><path d="M8.1 2.5h7.8l5.6 5.6v7.8l-5.6 5.6H8.1l-5.6-5.6V8.1z"/></symbol>
  <symbol id="i-livre" viewBox="0 0 24 24"><path d="m5 12.5 4.5 4.5L19 7.5"/></symbol>
  <symbol id="i-painel" viewBox="0 0 24 24"><path d="M3.5 17a8.5 8.5 0 1 1 17 0"/><path d="m12 17 4.5-5.5"/><path d="M3.5 20.5h17"/></symbol>
  <symbol id="i-hist" viewBox="0 0 24 24"><circle cx="12" cy="12" r="9"/><path d="M12 7v5l3.2 2"/></symbol>
  <symbol id="i-sobre" viewBox="0 0 24 24"><path d="m2 9 10-5 10 5-10 5z"/><path d="M6 11v5c3.5 2.6 8.5 2.6 12 0v-5M22 9v6"/></symbol>
  <symbol id="i-wifi" viewBox="0 0 24 24"><path d="M2.5 9a14 14 0 0 1 19 0M5.8 12.6a9 9 0 0 1 12.4 0M9.2 16.2a4 4 0 0 1 5.6 0"/><path d="M12 19.6v.01"/></symbol>
  <symbol id="i-semwifi" viewBox="0 0 24 24"><path d="M3 3l18 18"/><path d="M9.2 16.2a4 4 0 0 1 5.6 0M5.8 12.6a9 9 0 0 1 4.4-2.4M2.5 9a14 14 0 0 1 4.2-2.7M13.8 6.2a14 14 0 0 1 7.7 2.8M16.7 10.3a9 9 0 0 1 1.5 1.3"/><path d="M12 19.6v.01"/></symbol>
  <symbol id="i-play" viewBox="0 0 24 24"><path d="M7 4.5v15l12-7.5z"/></symbol>
  <symbol id="i-sair" viewBox="0 0 24 24"><path d="M6 6l12 12M18 6 6 18"/></symbol>
  <symbol id="i-seta" viewBox="0 0 24 24"><path d="m9 5 7 7-7 7"/></symbol>
  <symbol id="i-info" viewBox="0 0 24 24"><circle cx="12" cy="12" r="9"/><path d="M12 11v6M12 7.6v.01"/></symbol>
</svg>

<header class="topo">
  <div class="topo-interno">
    <div class="marca">
      <svg viewBox="0 0 32 32" aria-hidden="true"><rect class="marca-fundo" width="32" height="32" rx="8"/><rect class="marca-tinta" x="11.5" y="9" width="9" height="13" rx="2.5"/><rect class="marca-tinta" x="6" y="13" width="3.6" height="11" rx="1.6"/><rect class="marca-tinta" x="22.4" y="13" width="3.6" height="11" rx="1.6"/><rect class="marca-tinta" x="11.5" y="23.5" width="9" height="3" rx="1.2"/><rect class="marca-tinta" x="13" y="5" width="6" height="2.4" rx="1.2"/></svg>
      <h1>Cadeira por Voz</h1>
    </div>
    <nav class="abas" role="tablist" aria-label="Seções">
      <button type="button" class="aba" role="tab" id="tab-painel" aria-controls="aba-painel" aria-selected="true"><svg class="ic" aria-hidden="true"><use href="#i-painel"/></svg><span>Painel</span></button>
      <button type="button" class="aba" role="tab" id="tab-historico" aria-controls="aba-historico" aria-selected="false" tabindex="-1"><svg class="ic" aria-hidden="true"><use href="#i-hist"/></svg><span>Histórico</span></button>
      <button type="button" class="aba" role="tab" id="tab-sobre" aria-controls="aba-sobre" aria-selected="false" tabindex="-1"><svg class="ic" aria-hidden="true"><use href="#i-sobre"/></svg><span>Sobre o TCC</span></button>
    </nav>
    <div id="pilula" class="pilula" role="status" aria-live="polite">Conectando…</div>
  </div>
  <div id="faixa-demo" class="faixa-demo" hidden>
    <div class="faixa-demo-interno">
      <p id="faixa-demo-texto"><strong>Modo demonstração</strong> — dados simulados, a cadeira não está conectada</p>
      <button type="button" id="btn-sair-demo" class="botao-faixa">Sair da demonstração</button>
    </div>
  </div>
</header>

<main class="pagina">
  <section id="aviso-conexao" class="aviso" hidden aria-live="polite">
    <div class="aviso-ic"><svg class="ic" aria-hidden="true"><use id="aviso-icone" href="#i-semwifi"/></svg></div>
    <div class="aviso-corpo">
      <h2 id="aviso-titulo">Sem conexão com a cadeira</h2>
      <p id="dica-sem">Entre na rede Wi-Fi <strong>Cadeira-TCC</strong> e abra <code>http://192.168.4.1</code> no navegador. O app se reconecta sozinho quando a cadeira responder.</p>
      <p id="dica-placa" hidden>O celular chegou ao ESP8266, mas o Mega não está mandando dados. Confira se as chaves DIP 1 e 2 (MCU↔ESP) estão ligadas e se o Mega está ligado.</p>
      <div class="aviso-acoes" id="aviso-acoes">
        <button type="button" id="btn-demo" class="botao"><svg class="ic" aria-hidden="true"><use href="#i-play"/></svg>Ver demonstração</button>
        <span class="tentando">Tentando de novo…</span>
      </div>
    </div>
  </section>

  <!-- PAINEL -->
  <section id="aba-painel" class="painel" role="tabpanel" aria-labelledby="tab-painel">
    <div class="col">
      <div id="alertas" class="alertas sem-alerta" aria-live="assertive">
        <div id="alerta-emergencia" class="faixa faixa-critico" hidden><svg class="ic" aria-hidden="true"><use href="#i-octogono"/></svg><div><strong>EMERGÊNCIA</strong><span>Botão de emergência acionado. A cadeira fica parada e ignora comandos enquanto ele estiver apertado.</span></div></div>
        <div id="alerta-obstaculo" class="faixa faixa-alerta" hidden><svg class="ic" aria-hidden="true"><use href="#i-triangulo"/></svg><div><strong>OBSTÁCULO À FRENTE</strong><span>A cadeira não avança. A ré continua liberada.</span></div></div>
        <div id="alerta-voz" class="faixa faixa-info" hidden><svg class="ic" aria-hidden="true"><use href="#i-mic"/></svg><div><strong>Módulo de voz indisponível</strong><span>A cadeira não está ouvindo comandos de voz. Confira se o TX do módulo está no pino 19 e o RX no 18.</span></div></div>
      </div>

      <article id="cartao-estado" class="cartao-estado" aria-labelledby="estado-nome">
        <div class="estado-topo">
          <div class="estado-ic"><svg class="ic" aria-hidden="true"><use id="estado-icone" href="#i-parada"/></svg></div>
          <div class="estado-txt">
            <p class="rotulo">Agora</p>
            <h2 id="estado-nome" class="estado-nome">Sem leitura</h2>
            <p id="ultima-parada" class="ultima" hidden></p>
          </div>
        </div>
        <span id="estado-velho" class="velho" hidden>Última leitura conhecida</span>
      </article>

      <section id="cartao-medidas" class="cartao-medidas" aria-label="Rodas, velocidade e sensores">
        <div class="tracao">
          <div class="roda roda-esq">
            <div class="roda-txt"><span class="rotulo">Roda esq.</span><span id="pct-e" class="roda-pct">0%</span><span id="sent-e" class="roda-sent">parada</span><span id="pwm-e" class="roda-pwm">PWM 0</span></div>
            <div class="trilho" aria-hidden="true"><div id="barra-e" class="barra"></div></div>
          </div>
          <svg id="cadeira" class="cadeira" viewBox="0 0 120 170" role="img" aria-label="Vista de cima da cadeira, com o sensor à frente e uma roda motriz de cada lado">
            <path class="cone" d="M60 33 25 1h70z"/>
            <rect class="sensor" x="52" y="30" width="16" height="5" rx="2"/>
            <rect class="pe" x="44" y="37" width="32" height="9" rx="3"/>
            <circle class="rodizio" cx="38" cy="57" r="5"/>
            <circle class="rodizio" cx="82" cy="57" r="5"/>
            <rect class="assento" x="33" y="48" width="54" height="76" rx="10"/>
            <rect class="encosto" x="33" y="126" width="54" height="13" rx="4"/>
            <path class="eixo" d="M26 108h68"/>
            <rect id="svg-roda-e" class="roda-svg" x="11" y="80" width="15" height="58" rx="5"/>
            <rect id="svg-roda-d" class="roda-svg" x="94" y="80" width="15" height="58" rx="5"/>
            <path id="chev-e" class="chev" d="m13.5 112 5-6 5 6" visibility="hidden"/>
            <path id="chev-d" class="chev" d="m96.5 112 5-6 5 6" visibility="hidden"/>
            <path class="seta seta-FRENTE" d="M60 104V72" marker-end="url(#ponta-cadeira)"/>
            <path class="seta seta-RE" d="M60 68v32" marker-end="url(#ponta-cadeira)"/>
            <path class="seta seta-GIRO_ESQ" d="M73 86a13 13 0 1 0-3.8 9.2" marker-end="url(#ponta-cadeira)"/>
            <path class="seta seta-GIRO_DIR" d="M47 86a13 13 0 1 1 3.8 9.2" marker-end="url(#ponta-cadeira)"/>
          </svg>
          <div class="roda roda-dir">
            <div class="trilho" aria-hidden="true"><div id="barra-d" class="barra"></div></div>
            <div class="roda-txt"><span class="rotulo">Roda dir.</span><span id="pct-d" class="roda-pct">0%</span><span id="sent-d" class="roda-sent">parada</span><span id="pwm-d" class="roda-pwm">PWM 0</span></div>
          </div>
        </div>
        <p class="legenda">Tração diferencial: rodas em sentidos opostos fazem a cadeira girar no lugar.</p>
        <div class="vel">
          <strong id="vel-texto" class="vel-texto">Velocidade – de 3</strong>
          <div class="vel-niveis" aria-hidden="true">
            <div class="nivel" data-n="1"><b>1</b><small>PWM 130</small></div>
            <div class="nivel" data-n="2"><b>2</b><small>PWM 175</small></div>
            <div class="nivel" data-n="3"><b>3</b><small>PWM 220</small></div>
          </div>
        </div>
        <dl class="sensores">
          <div><dt>Sensor IR</dt><dd id="chip-ir" class="chip"><svg class="ic" aria-hidden="true"><use href="#i-livre"/></svg><span>–</span></dd></div>
          <div><dt>Módulo de voz</dt><dd id="chip-voz" class="chip"><svg class="ic" aria-hidden="true"><use href="#i-mic"/></svg><span>–</span></dd></div>
        </dl>
      </section>
    </div>

    <div class="col">
      <section class="cartao-controle" aria-labelledby="trava-rotulo">
        <button type="button" id="trava" class="trava" role="switch" aria-checked="false" aria-labelledby="trava-rotulo" aria-describedby="trava-estado">
          <svg class="ic" aria-hidden="true"><use id="trava-icone" href="#i-fechado"/></svg>
          <span class="trava-txt"><span id="trava-rotulo" class="trava-rot">Controle pelo app</span><span id="trava-estado" class="trava-est">Bloqueado</span></span>
          <span class="chave" aria-hidden="true"></span>
        </button>
        <p id="nota-app" class="nota-app" hidden><svg class="ic" aria-hidden="true"><use href="#i-info"/></svg><span>O firmware está em modo só acompanhamento (<code>PERMITIR_COMANDO_APP = 0</code>)</span></p>
        <p id="nota-controle" class="nota">O app só acompanha. Ligue a chave para comandar a cadeira pelo celular.</p>
        <div id="comandos" class="comandos" hidden>
          <p class="dica">Segure ▲ ou ▼ para andar. Soltou, parou.</p>
          <div class="dpad">
            <button type="button" id="btn-frente" class="tecla seg t-f" data-segurar="frente" disabled><svg class="ic" aria-hidden="true"><use href="#i-cima"/></svg><span>Frente</span></button>
            <button type="button" id="btn-esquerda" class="tecla t-e" data-toque="esquerda" disabled><svg class="ic" aria-hidden="true"><use href="#i-esq"/></svg><span>Esquerda</span></button>
            <div class="dpad-centro" aria-hidden="true"><span>nível</span><b id="dpad-nivel">–</b><span>de 3</span></div>
            <button type="button" id="btn-direita" class="tecla t-d" data-toque="direita" disabled><svg class="ic" aria-hidden="true"><use href="#i-dir"/></svg><span>Direita</span></button>
            <button type="button" id="btn-re" class="tecla seg t-r" data-segurar="tras" disabled><svg class="ic" aria-hidden="true"><use href="#i-baixo"/></svg><span>Ré</span></button>
          </div>
          <div class="velocidades">
            <button type="button" id="btn-devagar" class="tecla" data-toque="devagar" disabled><svg class="ic" aria-hidden="true"><use href="#i-menos"/></svg><span>Devagar</span></button>
            <button type="button" id="btn-rapido" class="tecla" data-toque="rapido" disabled><svg class="ic" aria-hidden="true"><use href="#i-mais"/></svg><span>Rápido</span></button>
          </div>
          <ul id="atalhos" class="atalhos" aria-label="Atalhos de teclado" hidden><li><kbd>↑</kbd><kbd>↓</kbd> segurar</li><li><kbd>←</kbd><kbd>→</kbd> girar</li><li><kbd>espaço</kbd> parar</li><li><kbd>+</kbd><kbd>−</kbd> velocidade</li></ul>
        </div>
      </section>

      <section id="cartao-sim" class="cartao-sim" hidden aria-labelledby="sim-titulo">
        <h2 id="sim-titulo">Simular na demonstração</h2>
        <p>Liga e desliga o que os sensores da cadeira veriam.</p>
        <div class="sim-botoes">
          <button type="button" id="sim-obstaculo" class="sim" aria-pressed="false"><svg class="ic" aria-hidden="true"><use href="#i-triangulo"/></svg>Obstáculo</button>
          <button type="button" id="sim-emergencia" class="sim" aria-pressed="false"><svg class="ic" aria-hidden="true"><use href="#i-octogono"/></svg>Botão de emergência</button>
        </div>
      </section>

      <details class="diag">
        <summary><svg class="ic" aria-hidden="true"><use href="#i-seta"/></svg>Diagnóstico da conexão</summary>
        <dl>
          <dt>Telemetria recebida</dt><dd id="dg-idade">–</dd>
          <dt>Relógio do Mega</dt><dd id="dg-mega">–</dd>
          <dt>ESP8266 ligado há</dt><dd id="dg-esp">–</dd>
          <dt>Celulares na rede</dt><dd id="dg-clientes">–</dd>
          <dt>Linhas válidas / descartadas</dt><dd id="dg-linhas">–</dd>
          <dt>Este app (id · envios)</dt><dd id="dg-id">–</dd>
        </dl>
      </details>
    </div>
  </section>

  <!-- HISTORICO -->
  <section id="aba-historico" class="historico" role="tabpanel" aria-labelledby="tab-historico" hidden>
    <div class="hist-cab">
      <h2>Histórico</h2>
      <span id="hist-conta" class="hist-nota">0 eventos</span>
      <button type="button" id="btn-limpar" class="botao-sec" disabled><svg class="ic" aria-hidden="true"><use href="#i-sair"/></svg>Limpar</button>
    </div>
    <p class="hist-nota">Mais recente em cima. O horário é aproximado, calculado pela idade de cada evento no ESP8266. Ficam guardados os últimos 200, só neste aparelho.</p>
    <ol id="hist-lista" class="hist" hidden></ol>
    <p id="hist-vazio" class="vazio">Nenhum evento ainda. Comandos, paradas, obstáculos e emergências aparecem aqui assim que acontecem.</p>
  </section>

  <!-- SOBRE -->
  <article id="aba-sobre" class="sobre" role="tabpanel" aria-labelledby="tab-sobre" hidden>
    <section class="capa">
      <p id="sobre-curso" class="rotulo">Trabalho de Conclusão de Curso · Robótica</p>
      <h2>Cadeira de rodas com comando de voz</h2>
      <p class="lead">Protótipo em escala reduzida de uma cadeira de rodas que se movimenta por comandos de voz, com travas de segurança e detecção de obstáculo. O reconhecimento é feito no próprio módulo embarcado, sem internet e sem servidor.</p>
    </section>

    <section>
      <h2>Objetivo geral</h2>
      <p>Desenvolver e validar um protótipo funcional de cadeira de rodas motorizada controlada por comandos de voz, com travas de segurança que impeçam movimento indesejado.</p>
    </section>

    <section>
      <h2>Justificativa</h2>
      <p>O acionamento por voz interessa a pessoas com mobilidade reduzida nos membros superiores, para quem o joystick convencional é difícil ou impossível de operar.</p>
      <p>O reconhecimento feito no próprio módulo evita depender de servidor e de internet. Isso reduz o atraso de resposta e mantém o funcionamento em qualquer ambiente. Em troca, exige treinar antes a voz do usuário e limita o vocabulário.</p>
    </section>

    <section>
      <h2>Como funciona</h2>
      <figure class="diagrama">
        <svg viewBox="0 0 360 424" role="img" aria-labelledby="diag-tit">
          <title id="diag-tit">Diagrama de blocos: voz, módulo V3, sensor IR e botão de emergência entram no Arduino Mega 2560, que comanda a ponte H L298N e os motores. O Mega conversa com o ESP8266, que conversa com o app no celular.</title>
          <rect class="d-regiao" x="240" y="280" width="118" height="142" rx="12"/>
          <path class="d-seta" d="M60 56v38" marker-end="url(#ponta-diag)"/>
          <path class="d-seta" d="M60 144v38" marker-end="url(#ponta-diag)"/>
          <path class="d-seta" d="M180 56v126" marker-end="url(#ponta-diag)"/>
          <path class="d-seta" d="M300 56v126" marker-end="url(#ponta-diag)"/>
          <path class="d-seta" d="M60 248v38" marker-end="url(#ponta-diag)"/>
          <path class="d-seta" d="M112 312h14" marker-end="url(#ponta-diag)"/>
          <path class="d-seta" d="M300 250v36" marker-start="url(#ponta-diag)" marker-end="url(#ponta-diag)"/>
          <path class="d-seta" d="M300 338v36" marker-start="url(#ponta-diag)" marker-end="url(#ponta-diag)"/>
          <rect class="d-caixa" x="8" y="8" width="104" height="48" rx="10"/>
          <text class="d-tit" x="60" y="29" text-anchor="middle">Usuário</text><text class="d-sub" x="60" y="45" text-anchor="middle">fala o comando</text>
          <rect class="d-caixa" x="128" y="8" width="104" height="48" rx="10"/>
          <text class="d-tit" x="180" y="29" text-anchor="middle">Sensor IR</text><text class="d-sub" x="180" y="45" text-anchor="middle">obstáculo frontal</text>
          <rect class="d-caixa" x="248" y="8" width="104" height="48" rx="10"/>
          <text class="d-tit" x="300" y="29" text-anchor="middle">Emergência</text><text class="d-sub" x="300" y="45" text-anchor="middle">botão físico</text>
          <rect class="d-caixa" x="8" y="96" width="104" height="48" rx="10"/>
          <text class="d-tit" x="60" y="117" text-anchor="middle">Módulo V3</text><text class="d-sub" x="60" y="133" text-anchor="middle">reconhece a voz</text>
          <text class="d-rot" x="66" y="168">Serial1</text>
          <text class="d-rot" x="186" y="124">pino 2</text>
          <text class="d-rot" x="306" y="124">pino 3</text>
          <rect class="d-mega" x="8" y="184" width="344" height="64" rx="12"/>
          <text class="d-mega-tit" x="180" y="211" text-anchor="middle">Arduino Mega 2560</text>
          <text class="d-sub" x="180" y="231" text-anchor="middle">máquina de estados e travas de segurança</text>
          <text class="d-rot" x="66" y="272">PWM</text>
          <text class="d-rot" x="294" y="272" text-anchor="end">Serial3</text>
          <rect class="d-caixa" x="8" y="288" width="104" height="48" rx="10"/>
          <text class="d-tit" x="60" y="309" text-anchor="middle">Ponte H</text><text class="d-sub" x="60" y="325" text-anchor="middle">L298N</text>
          <rect class="d-caixa" x="128" y="288" width="104" height="48" rx="10"/>
          <text class="d-tit" x="180" y="309" text-anchor="middle">2 motores DC</text><text class="d-sub" x="180" y="325" text-anchor="middle">rodas de 68 mm</text>
          <rect class="d-rede" x="248" y="288" width="104" height="48" rx="10"/>
          <text class="d-tit" x="300" y="309" text-anchor="middle">ESP8266</text><text class="d-sub" x="300" y="325" text-anchor="middle">Wi-Fi da placa</text>
          <text class="d-rot" x="294" y="360" text-anchor="end">Wi-Fi</text>
          <rect class="d-rede" x="248" y="376" width="104" height="40" rx="10"/>
          <text class="d-tit" x="300" y="401" text-anchor="middle">App no celular</text>
          <text class="d-sub" x="8" y="384">Em tracejado, a parte que fica fora</text>
          <text class="d-sub" x="8" y="399">do caminho de segurança: o ESP8266</text>
          <text class="d-sub" x="8" y="414">só repassa pedidos ao Mega.</text>
        </svg>
        <figcaption>A voz, o sensor e o botão de emergência entram no Mega, que decide e comanda a ponte H. O app fala com o Mega através do ESP8266.</figcaption>
      </figure>
      <p>A cada volta do laço principal, o Mega lê, nesta ordem, o botão de emergência, o módulo de voz, os comandos digitados, os comandos do app e o sensor de obstáculo. Depois atualiza a máquina de estados, escreve o resultado na ponte H e atualiza a sinalização e a telemetria. Nenhuma etapa espera por evento externo: se o laço parasse para esperar, o sensor e o tempo limite de segurança parariam junto.</p>
      <p>A cadeira tem cinco estados: parada, frente, ré, giro à esquerda e giro à direita. Avanço e ré continuam até uma parada. O giro é um pulso de 700 ms a 70% da velocidade e termina sozinho, para que uma falha de reconhecimento não deixe a cadeira girando sem controle.</p>
      <p>São três níveis de velocidade, com PWM 130, 175 e 220 numa escala que vai até 255. A rampa muda o PWM em no máximo 8 a cada 20 ms.</p>
    </section>

    <section>
      <h2>Os 7 comandos de voz</h2>
      <div class="rolagem">
        <table class="tabela">
          <thead><tr><th scope="col">Reg.</th><th scope="col">Palavra</th><th scope="col">O que faz</th></tr></thead>
          <tbody>
            <tr><td>0</td><td>frente</td><td>anda para frente até parar</td></tr>
            <tr><td>1</td><td>trás</td><td>anda para trás até parar</td></tr>
            <tr><td>2</td><td>esquerda</td><td>gira à esquerda por um tempo curto</td></tr>
            <tr><td>3</td><td>direita</td><td>gira à direita por um tempo curto</td></tr>
            <tr><td>4</td><td>parar</td><td>para imediatamente</td></tr>
            <tr><td>5</td><td>rápido</td><td>sobe um nível de velocidade</td></tr>
            <tr><td>6</td><td>devagar</td><td>desce um nível de velocidade</td></tr>
          </tbody>
        </table>
      </div>
      <p>São sete porque o Módulo de Voz V3 reconhece no máximo sete registros ao mesmo tempo, embora guarde até 80 na memória.</p>
    </section>

    <section>
      <h2>As 8 travas de segurança</h2>
      <p>Nenhuma depende de outra funcionar, e toda trava tem teste automatizado correspondente.</p>
      <ol class="travas">
        <li>O sensor de obstáculo para o avanço e recusa novos comandos de frente.</li>
        <li>Homem-morto: sem comando novo em 3 segundos, a cadeira para sozinha.</li>
        <li>“Parar” corta o PWM no mesmo instante, sem passar pela rampa.</li>
        <li>Botão físico de emergência, com prioridade sobre qualquer comando.</li>
        <li>Rampa de aceleração, que limita a variação de PWM por ciclo.</li>
        <li>Inversão de sentido obrigada a passar por zero, para não inverter a ponte H com o motor girando.</li>
        <li>Os motores são desligados antes de qualquer outra inicialização, para a cadeira nunca partir sozinha ao ligar.</li>
        <li>A ré continua liberada com obstáculo à frente, porque o sensor só olha para frente e travar a ré prenderia o usuário.</li>
      </ol>
    </section>

    <section>
      <h2>Componentes</h2>
      <dl class="pecas">
        <div><dt>Placa compatível Arduino Mega 2560 + ESP8266 (CH340)</dt><dd>Processamento. Quatro portas seriais em hardware atendem voz, depuração e Wi-Fi ao mesmo tempo.</dd></div>
        <div><dt>Módulo de Reconhecimento de Voz V3</dt><dd>Reconhecimento embarcado, até 80 registros gravados e 7 ativos.</dd></div>
        <div><dt>Ponte H dupla L298N</dt><dd>Aciona os dois motores nos dois sentidos, com controle de velocidade.</dd></div>
        <div><dt>Sensor de obstáculo reflexivo infravermelho</dt><dd>Detecção de obstáculo frontal.</dd></div>
        <div><dt>2 motores DC 3-6 V com roda de 68 mm</dt><dd>Tração diferencial.</dd></div>
        <div><dt>Buzzer, LED e botão de emergência</dt><dd>Sinalização e parada manual.</dd></div>
      </dl>
    </section>

    <section>
      <h2>Metodologia</h2>
      <ol class="etapas">
        <li><div><b>Separação entre lógica e hardware.</b> A máquina de estados que decide o movimento foi escrita sem depender da biblioteca do Arduino. O que toca o hardware ficou em módulos próprios: ponte H, sensor infravermelho e módulo de voz.</div></li>
        <li><div><b>Verificação automatizada.</b> Como a lógica não depende do Arduino, ela é compilada e testada no computador, sem a placa. Os testes cobrem cada regra de segurança e também a conversa entre o Mega, o ESP8266 e o app. Os firmwares inteiros são compilados no PC com substitutos das bibliotecas, o que pega erro antes de abrir a IDE.</div></li>
        <li><div><b>Integração.</b> Montagem da fiação, treinamento dos sete comandos de voz e ajuste da velocidade mínima e da compensação de trajetória, com o protótipo montado e no piso de uso.</div></li>
        <li><div><b>Ensaios.</b> Medição da taxa de acerto, da distância de parada e do alcance do sensor por cor de superfície, e verificação de cada trava de segurança.</div></li>
      </ol>
      <div class="defeito">
        <h3>O defeito real que um teste encontrou</h3>
        <p>Na desaceleração, com PWM 130 e passo de rampa 8, o valor pulava de positivo para negativo sem passar por zero. A ponte H invertia a polaridade com o motor ainda girando, o que gera pico de corrente e um tranco. A correção obriga a roda a ficar em zero por pelo menos um ciclo antes de inverter.</p>
        <p class="seq"><em>Antes</em><span>10</span><span>2</span><span class="ruim">−6</span></p>
        <p class="seq"><em>Depois</em><span>10</span><span>2</span><span class="bom">0</span><span>−8</span></p>
      </div>
    </section>

    <section>
      <h2>Como o app se integra</h2>
      <p>O ESP8266 da própria placa cria a rede Wi-Fi <strong>Cadeira-TCC</strong> e serve este app em <code>http://192.168.4.1</code>. Não precisa de roteador, de internet nem de instalar nada: o app abre no navegador do celular.</p>
      <p>O Mega manda ao ESP8266 o estado da cadeira a cada 250 ms e um aviso no instante de cada evento: comando, parada, obstáculo e emergência. O app consulta o ESP8266 a cada 300 ms. Cada linha da serial leva um checksum, porque o ESP8266 imprime lixo na serial quando liga e ruído não pode virar comando.</p>
      <p>Para andar pelo app, o dedo fica em Frente ou Ré. Enquanto ele está lá, o app renova o comando a cada 400 ms. Soltou, o app manda parar. Se a rede cair, as renovações deixam de chegar e o homem-morto para a cadeira em até 3 s.</p>
      <h3>Por que o Wi-Fi fica fora do caminho de segurança</h3>
      <p>Comando por rede tem atraso e perda de pacote, e a cadeira precisa de tempo de resposta previsível para parar. Por isso o ESP8266 só repassa pedidos. Quem decide é o Mega, pela mesma máquina de estados da voz, com todas as travas valendo. Parar é sempre aceito. Com o botão de emergência apertado, tudo o que chega do app, da voz e do teclado é descartado. Com <code>PERMITIR_COMANDO_APP = 0</code> no firmware, o app só acompanha.</p>
    </section>

    <section>
      <h2>Limitações</h2>
      <ul>
        <li>O reconhecimento depende de quem gravou a voz. Outra pessoa tem taxa de acerto bem menor.</li>
        <li>O vocabulário é de sete comandos, o limite do reconhecedor ativo.</li>
        <li>O ruído atrapalha, inclusive o dos próprios motores. Por isso o treino é feito com a cadeira parada.</li>
        <li>O sensor infravermelho enxerga mal superfícies pretas foscas, vidro e objetos muito inclinados, e não detecta degraus para baixo.</li>
        <li>Há um único sensor, frontal. Não existe proteção lateral nem traseira.</li>
        <li>A escala é reduzida: motores de 3-6 V com roda de 68 mm não movem uma cadeira real com pessoa. O protótipo demonstra o princípio de controle, não a capacidade de carga.</li>
        <li>Não há malha fechada. Sem encoder, o giro é por tempo e varia com o piso e com a carga da bateria.</li>
      </ul>
    </section>

    <section>
      <h2>Trabalhos futuros</h2>
      <ul>
        <li>Encoders nas rodas, para controle em malha fechada e giro por ângulo.</li>
        <li>Sensor ultrassônico junto do infravermelho, cobrindo as fraquezas de cada um, mais sensores laterais e traseiro.</li>
        <li>Sensor apontado para baixo, para detectar degrau.</li>
        <li>Confirmação por voz nos comandos de avanço, como “frente” seguido de “confirma”.</li>
        <li>Registro da telemetria fora do celular. Hoje o app acompanha ao vivo, mas o histórico fica só na memória do navegador.</li>
        <li>Estudo com mais de um locutor, para medir a queda de desempenho.</li>
      </ul>
    </section>

    <section id="sobre-equipe" class="equipe" hidden>
      <h2>Equipe</h2>
      <dl id="equipe-dados"></dl>
    </section>
  </article>
</main>

<div class="dock">
  <div class="dock-interno">
    <div class="colar">
      <button type="button" id="btn-parar" class="parar" aria-label="PARAR"><svg class="ic ic-cheio" aria-hidden="true"><use href="#i-pare"/></svg><span>PARAR</span><span class="parar-tecla" aria-hidden="true">espaço</span></button>
    </div>
  </div>
</div>

<div class="toasts" aria-live="assertive"><div id="toast" class="toast" hidden><svg class="ic" aria-hidden="true"><use id="toast-icone" href="#i-info"/></svg><span id="toast-texto"></span></div></div>

<script>
// Equipe do TCC: preencha aqui. A secao "Equipe" da aba Sobre so aparece com integrantes.
const EQUIPE = { integrantes: [], orientador: "", instituicao: "", curso: "Robótica", ano: "" };

(function () {
"use strict";

// Servido pelo ESP o caminho e relativo. Aberto como arquivo no celular,
// vai direto ao IP fixo do ponto de acesso da cadeira.
const API = location.protocol === "file:" ? "http://192.168.4.1" : "";
const INTERVALO_ESTADO_MS = 300;
const LIMITE_ESTADO_MS = 1500;
const FALHAS_SEM_CONEXAO = 3;
// Renovacao bem abaixo dos 3 s do homem-morto: perder uma ou duas nao para a cadeira.
const RENOVAR_MS = 400;
const LIMITE_MOVIMENTO_MS = 1200;
const LIMITE_OUTROS_MS = 4000;
const MAX_HISTORICO = 200;
const SVG = "http://www.w3.org/2000/svg";

const NOME_ESTADO = { PARADA: "Parada", FRENTE: "Andando para frente", RE: "Andando de ré", GIRO_ESQ: "Girando à esquerda", GIRO_DIR: "Girando à direita" };
const ICONE_ESTADO = { PARADA: "i-parada", FRENTE: "i-cima", RE: "i-baixo", GIRO_ESQ: "i-esq", GIRO_DIR: "i-dir" };
const MOTIVO = { COMANDO: "comando do usuário", OBSTACULO: "obstáculo à frente", TEMPO_LIMITE: "tempo limite sem comando (homem-morto)", FIM_DO_GIRO: "fim do giro", EMERGENCIA: "botão de emergência" };
const NOME_CMD = { FRENTE: "Frente", RE: "Ré", ESQUERDA: "Esquerda", DIREITA: "Direita", PARAR: "Parar", RAPIDO: "Rápido", DEVAGAR: "Devagar" };
const ORIGEM = { voz: "pela voz", teclado: "pelo teclado", app: "pelo app" };
const ICONE_ORIGEM = { voz: "i-mic", teclado: "i-teclado", app: "i-celular" };
const MOVIMENTO = { frente: true, tras: true, esquerda: true, direita: true };
const CONEXAO = {
  iniciando: ["Conectando…", ""],
  conectado: ["Conectado", "ok"],
  placa: ["Sem resposta", "alerta"],
  sem: ["Sem conexão", "off"],
  demo: ["Demonstração", "demo"]
};
const ABAS = ["painel", "historico", "sobre"];

const $ = (id) => document.getElementById(id);

const E = {
  conexao: "iniciando",
  falhas: 0,
  seq: 0,
  uptime: null,
  primeiro: true,
  tel: null,
  idadeTel: null,
  esp: null,
  travado: true,
  historico: [],
  histSujo: true,
  demo: null,
  aba: "painel"
};

const ID_CLIENTE = gerarId(8);
let contadorN = 0;

function gerarId(tam) {
  const abc = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
  let v = null;
  try { v = crypto.getRandomValues(new Uint8Array(tam)); } catch (e) { v = null; }
  let s = "";
  for (let i = 0; i < tam; i++) s += abc[(v ? v[i] : Math.floor(Math.random() * 256)) % abc.length];
  return s;
}

function texto(el, t) { if (el.textContent !== t) el.textContent = t; }
function usar(el, id) { if (el.getAttribute("href") !== "#" + id) el.setAttribute("href", "#" + id); }
function icone(id) {
  const s = document.createElementNS(SVG, "svg");
  s.setAttribute("class", "ic");
  s.setAttribute("aria-hidden", "true");
  const u = document.createElementNS(SVG, "use");
  u.setAttribute("href", "#" + id);
  s.appendChild(u);
  return s;
}
// Antes do primeiro toque o Chrome recusa vibrar e reclama no console.
function vibrar(p) {
  try {
    const ativo = !navigator.userActivation || navigator.userActivation.hasBeenActive;
    if (navigator.vibrate && ativo) navigator.vibrate(p);
  } catch (e) { /* sem vibracao */ }
}
function dois(n) { return String(n).padStart(2, "0"); }
function hora(ms) { const d = new Date(ms); return dois(d.getHours()) + ":" + dois(d.getMinutes()) + ":" + dois(d.getSeconds()); }
function duracao(ms) {
  if (typeof ms !== "number" || !isFinite(ms) || ms < 0) return "–";
  if (ms < 1000) return Math.round(ms) + " ms";
  const s = Math.floor(ms / 1000);
  if (s < 60) return (ms / 1000).toFixed(1).replace(".", ",") + " s";
  const m = Math.floor(s / 60);
  if (m < 60) return m + " min " + dois(s % 60) + " s";
  return Math.floor(m / 60) + " h " + dois(m % 60) + " min";
}
function comSinal(v) { return v > 0 ? "+" + v : v < 0 ? "−" + Math.abs(v) : "0"; }

// ---------------------------------------------------------------------
// Avisos curtos
// ---------------------------------------------------------------------
let timerAviso = 0;
function avisar(msg, tipo) {
  const el = $("toast");
  el.className = "toast" + (tipo ? " " + tipo : "");
  usar($("toast-icone"), tipo === "critico" ? "i-octogono" : tipo === "alerta" ? "i-triangulo" : "i-info");
  $("toast-texto").textContent = msg;
  el.hidden = false;
  clearTimeout(timerAviso);
  timerAviso = setTimeout(() => { el.hidden = true; }, 4500);
}

// ---------------------------------------------------------------------
// Envio de comandos
// ---------------------------------------------------------------------
function podeMover() {
  const t = E.tel;
  if (E.travado || !t || t.emergencia === 1) return false;
  if (E.demo) return true;
  return E.conexao === "conectado" && t.app === 1;
}

let ultimoAvisoInstavel = 0;
// Comando de movimento sem resposta: nao da para saber se chegou, entao o
// lado seguro e mandar parar. Um aviso por episodio, mas o parar sempre sai.
function pararPorSeguranca() {
  encerrarSegurar(false);
  enviarComando("parar", false);
  const agora = Date.now();
  if (agora - ultimoAvisoInstavel > 1500) {
    ultimoAvisoInstavel = agora;
    avisar("Conexão instável: parada enviada por segurança", "alerta");
  }
}

function enviarComando(c, manter) {
  // Ultima barreira: movimento so sai com o controle liberado e a cadeira
  // conectada. Na demo, nada sai pela rede: vai so para a simulacao.
  if (c !== "parar" && !podeMover()) return;
  if (E.demo) {
    E.demo.comando(c, !!manter);
    atualizarDemo();
    return;
  }
  contadorN += 1;
  const corpo = new URLSearchParams();
  corpo.set("c", c);
  if (manter) corpo.set("manter", "1");
  corpo.set("id", ID_CLIENTE);
  corpo.set("n", String(contadorN));
  const vigiar = !!manter || MOVIMENTO[c] === true;
  const ctl = new AbortController();
  const timer = setTimeout(() => ctl.abort(), vigiar ? LIMITE_MOVIMENTO_MS : LIMITE_OUTROS_MS);
  // keepalive no parar: se a pagina esta sendo recarregada ou fechada (o
  // parar do pagehide), o navegador termina o envio em vez de corta-lo no meio.
  fetch(API + "/api/comando", { method: "POST", body: corpo, cache: "no-store", signal: ctl.signal, keepalive: c === "parar" })
    .then((r) => r.json().catch(() => null).then((j) => ({ status: r.status, j: j })))
    .then((res) => { clearTimeout(timer); tratarResposta(c, vigiar, res.status, res.j); })
    .catch(() => { clearTimeout(timer); falhaComando(c, vigiar); });
}

function falhaComando(c, vigiar) {
  if (vigiar) pararPorSeguranca();
  else if (c === "parar") avisar("A parada não chegou à cadeira. Sem comando novo, ela para sozinha em até 3 s.", "critico");
}

function tratarResposta(c, vigiar, status, j) {
  if (!j || typeof j !== "object") { falhaComando(c, vigiar); return; }
  if (j.ok === true) return;
  if (j.erro === "comando_atrasado") return;  // um pedido mais novo ja passou na frente
  if (j.erro === "sem_conexao") {
    if (vigiar) encerrarSegurar(true);
    avisar("A placa não respondeu. O comando não foi enviado.", "alerta");
    return;
  }
  if (j.erro === "comando_invalido") { avisar("O ESP8266 não reconheceu o comando “" + c + "”.", "alerta"); return; }
  falhaComando(c, vigiar);
}

// ---------------------------------------------------------------------
// Segure para andar
// ---------------------------------------------------------------------
let segurando = null;  // { cmd, el, ponteiro, tecla, timer }

function iniciarSegurar(cmd, el, ponteiro, tecla) {
  if (segurando || !podeMover()) return false;
  // O relogio da renovacao nasce antes do envio: se o envio ja encerrar o
  // segurar (demo em emergencia, por exemplo), o encerrar tambem o desliga.
  segurando = { cmd: cmd, el: el, ponteiro: ponteiro, tecla: tecla, timer: setInterval(renovar, RENOVAR_MS) };
  el.classList.add("ativo");
  enviarComando(cmd, false);
  return true;
}

function renovar() {
  if (!segurando) return;
  if (!podeMover()) { encerrarSegurar(true); return; }
  enviarComando(segurando.cmd, true);
}

function encerrarSegurar(mandarParar) {
  const s = segurando;
  if (!s) return;
  segurando = null;  // antes de soltar a captura: o lostpointercapture chega aqui de novo
  clearInterval(s.timer);
  s.el.classList.remove("ativo");
  if (s.ponteiro !== null) {
    try { if (s.el.hasPointerCapture(s.ponteiro)) s.el.releasePointerCapture(s.ponteiro); } catch (e) { /* ja solto */ }
  }
  if (mandarParar) enviarComando("parar", false);
}

// Dois dedos em dois botoes, ou outra tecla de movimento no meio: nao ha
// como saber o que a pessoa quer, entao para tudo.
function conflito() {
  if (!segurando) return;
  encerrarSegurar(true);
  avisar("Um movimento por vez. Parada enviada.", "alerta");
}

function tocar(cmd) {
  if (!podeMover()) return;
  if (segurando && MOVIMENTO[cmd]) { conflito(); return; }
  vibrar(15);
  enviarComando(cmd, false);
}

function acionarParar() {
  encerrarSegurar(false);
  vibrar(40);
  enviarComando("parar", false);
  const b = $("btn-parar");
  b.classList.add("acionado");
  setTimeout(() => b.classList.remove("acionado"), 180);
}

// ---------------------------------------------------------------------
// Estado vindo do ESP (ou da simulacao, no mesmo formato)
// ---------------------------------------------------------------------
let pedindo = false;
let timerEstado = 0;

function agendarEstado(ms) {
  clearTimeout(timerEstado);
  timerEstado = setTimeout(consultarEstado, ms);
}

async function consultarEstado() {
  if (E.demo || pedindo) return;
  pedindo = true;
  const inicio = Date.now();
  const ctl = new AbortController();
  const timer = setTimeout(() => ctl.abort(), LIMITE_ESTADO_MS);
  let r = null;
  try {
    const resp = await fetch(API + "/api/estado?desde=" + E.seq, { cache: "no-store", signal: ctl.signal });
    if (resp.ok) r = await resp.json();
  } catch (e) {
    r = null;
  }
  clearTimeout(timer);
  pedindo = false;
  if (E.demo) return;
  // Resposta fora do formato conta como falha; o laco de consulta nunca para.
  let ok = false;
  try {
    if (r && typeof r === "object" && typeof r.conectado === "boolean") {
      E.falhas = 0;
      receberEstado(r, r.conectado ? "conectado" : "placa");
      ok = true;
    }
  } catch (e) {
    ok = false;
  }
  if (!ok) {
    E.falhas += 1;
    if (E.falhas >= FALHAS_SEM_CONEXAO) mudarConexao("sem");
  }
  agendarEstado(Math.max(0, INTERVALO_ESTADO_MS - (Date.now() - inicio)));
}

function receberEstado(r, conexao) {
  const agora = Date.now();
  const seq = typeof r.seq === "number" ? r.seq : null;
  const up = r.esp && typeof r.esp.uptime_ms === "number" ? r.esp.uptime_ms : null;
  E.tel = r.tel && typeof r.tel === "object" ? r.tel : null;
  E.idadeTel = typeof r.idade_ms === "number" ? r.idade_ms : null;
  E.esp = r.esp && typeof r.esp === "object" ? r.esp : null;

  let usarEventos = true;
  if (seq !== null && seq < E.seq) {
    // O ESP reiniciou e mandou tudo desde o comeco: o historico recomeca daqui.
    E.historico = [];
    E.histSujo = true;
    E.seq = 0;
  } else if (up !== null && E.uptime !== null && up < E.uptime) {
    // Reiniciou e a contagem nova ja passou da antiga: pede tudo de novo.
    E.historico = [];
    E.histSujo = true;
    E.seq = 0;
    usarEventos = false;
  }
  E.uptime = up;

  if (usarEventos) {
    const avisarRecusa = !E.primeiro;
    const lista = Array.isArray(r.eventos) ? r.eventos : [];
    for (const ev of lista) {
      if (!ev || typeof ev.seq !== "number" || ev.seq <= E.seq) continue;
      E.seq = ev.seq;
      const idade = typeof ev.idade_ms === "number" ? ev.idade_ms : 0;
      adicionarEvento(ev.dados, agora - idade, avisarRecusa && idade < 5000);
    }
    if (seq !== null && seq > E.seq) E.seq = seq;
    E.primeiro = false;
  }
  mudarConexao(conexao);
  renderizar();
}

function mudarConexao(c) {
  if (E.conexao === c) return;
  E.conexao = c;
  renderizar();
}

// ---------------------------------------------------------------------
// Historico
// ---------------------------------------------------------------------
function descreverEvento(d) {
  if (!d || typeof d !== "object") return null;
  switch (d.tipo) {
    case "inicio":
      return { icone: "i-ligar", classe: "ev-info", texto: "Cadeira ligada " + (Number(d.voz) === 1 ? "(módulo de voz pronto)" : "(sem módulo de voz)") };
    case "comando": {
      const ok = Number(d.aceito) === 1;
      const nome = NOME_CMD[d.cmd] || String(d.cmd);
      const origem = ORIGEM[d.origem] || "(" + String(d.origem) + ")";
      return { icone: ICONE_ORIGEM[d.origem] || "i-info", classe: ok ? "" : "ev-alerta", texto: nome + " " + origem + " — " + (ok ? "aceito" : "recusado") };
    }
    case "parada":
      return { icone: "i-parada", classe: d.motivo === "EMERGENCIA" ? "ev-critico" : d.motivo === "OBSTACULO" ? "ev-alerta" : "", texto: "Parou: " + (MOTIVO[d.motivo] || "motivo não informado") };
    case "obstaculo":
      return Number(d.ativo) === 1 ? { icone: "i-triangulo", classe: "ev-alerta", texto: "Obstáculo detectado" } : { icone: "i-livre", classe: "ev-ok", texto: "Caminho livre" };
    case "emergencia":
      return Number(d.ativa) === 1 ? { icone: "i-octogono", classe: "ev-critico", texto: "Emergência acionada" } : { icone: "i-livre", classe: "ev-ok", texto: "Emergência liberada" };
    default:
      return null;
  }
}

function textoRecusa(d) {
  const nome = NOME_CMD[d.cmd] || String(d.cmd);
  const t = E.tel;
  if (d.cmd === "FRENTE" && t && t.obstaculo === 1) return nome + " recusado: obstáculo à frente";
  if (d.origem === "app" && t && t.app === 0) return nome + " recusado: o firmware só aceita parar pelo app";
  return nome + " recusado pela cadeira";
}

function adicionarEvento(dados, quando, avisarRecusa) {
  const desc = descreverEvento(dados);
  if (!desc) return;
  desc.hora = quando;
  E.historico.unshift(desc);
  if (E.historico.length > MAX_HISTORICO) E.historico.length = MAX_HISTORICO;
  E.histSujo = true;
  if (avisarRecusa && dados.tipo === "comando" && Number(dados.aceito) === 0) avisar(textoRecusa(dados), "alerta");
}

function renderHistorico() {
  if (!E.histSujo || E.aba !== "historico") return;
  E.histSujo = false;
  const lista = $("hist-lista");
  const frag = document.createDocumentFragment();
  for (const h of E.historico) {
    const li = document.createElement("li");
    li.className = "ev" + (h.classe ? " " + h.classe : "");
    const tm = document.createElement("time");
    tm.textContent = hora(h.hora);
    try { tm.dateTime = new Date(h.hora).toISOString(); } catch (e) { /* hora invalida */ }
    const ic = document.createElement("span");
    ic.className = "ev-ic";
    ic.appendChild(icone(h.icone));
    const tx = document.createElement("span");
    tx.className = "ev-txt";
    tx.textContent = h.texto;
    li.append(tm, ic, tx);
    frag.appendChild(li);
  }
  lista.textContent = "";
  lista.appendChild(frag);
  const n = E.historico.length;
  lista.hidden = n === 0;
  $("hist-vazio").hidden = n > 0;
  $("hist-conta").textContent = n === 1 ? "1 evento" : n + " eventos";
  $("btn-limpar").disabled = n === 0;
}

// ---------------------------------------------------------------------
// Desenho do painel
// ---------------------------------------------------------------------
function roda(lado, pwm) {
  const v = typeof pwm === "number" ? Math.max(-255, Math.min(255, pwm)) : 0;
  const pct = Math.round(Math.abs(v) / 255 * 100);
  texto($("pct-" + lado), pct + "%");
  texto($("sent-" + lado), v > 0 ? "frente" : v < 0 ? "ré" : "parada");
  texto($("pwm-" + lado), "PWM " + comSinal(v));
  const barra = $("barra-" + lado);
  barra.style.height = (pct / 2) + "%";
  barra.classList.toggle("re", v < 0);
  $("svg-roda-" + lado).classList.toggle("gira", v !== 0);
  const chev = $("chev-" + lado);
  chev.setAttribute("visibility", v === 0 ? "hidden" : "visible");
  chev.classList.toggle("re", v < 0);
}

function renderizar() {
  const t = E.tel;
  const vivo = !!t && (E.conexao === "conectado" || E.conexao === "demo");
  const c = CONEXAO[E.conexao];

  const pil = $("pilula");
  texto(pil, c[0]);
  pil.className = "pilula" + (c[1] ? " " + c[1] : "");

  // Aviso de conexao
  const aviso = $("aviso-conexao");
  const semConexao = E.conexao === "sem";
  aviso.hidden = !(semConexao || E.conexao === "placa");
  if (!aviso.hidden) {
    aviso.className = "aviso " + (semConexao ? "off" : "alerta");
    texto($("aviso-titulo"), semConexao ? "Sem conexão com a cadeira" : "Wi-Fi ok, mas a placa não responde");
    usar($("aviso-icone"), semConexao ? "i-semwifi" : "i-triangulo");
    $("dica-sem").hidden = !semConexao;
    $("dica-placa").hidden = semConexao;
    $("btn-demo").hidden = !semConexao;
  }

  if ($("faixa-demo").hidden !== !E.demo) {
    $("faixa-demo").hidden = !E.demo;
    document.documentElement.style.setProperty("--cabecalho", document.querySelector(".topo").offsetHeight + "px");
  }
  $("cartao-sim").hidden = !E.demo;

  // Faixas de alerta: so com dado ao vivo, para nao afirmar coisa velha.
  $("alerta-emergencia").hidden = !(vivo && t.emergencia === 1);
  $("alerta-obstaculo").hidden = !(vivo && t.obstaculo === 1);
  $("alerta-voz").hidden = !(vivo && t.voz === 0);
  // A regiao aria-live nunca some (leitor de tela so anuncia mudanca numa
  // regiao que ja existia); vazia, sai do fluxo para nao abrir espaco na grade.
  $("alertas").classList.toggle("sem-alerta", !vivo || !(t.emergencia === 1 || t.obstaculo === 1 || t.voz === 0));

  // Cartao de estado
  const cartao = $("cartao-estado");
  const est = t && NOME_ESTADO[t.estado] ? t.estado : null;
  texto($("estado-nome"), est ? NOME_ESTADO[est] : "Sem leitura");
  usar($("estado-icone"), est ? ICONE_ESTADO[est] : "i-parada");
  // Leitura velha nao ganha cor de movimento nem de emergencia: pode nao valer mais.
  cartao.classList.toggle("movendo", vivo && !!est && est !== "PARADA");
  cartao.classList.toggle("emergencia", vivo && t.emergencia === 1);
  $("cartao-medidas").classList.toggle("desatualizado", !vivo);
  cartao.classList.toggle("desatualizado", !vivo && !!t);
  $("estado-velho").hidden = vivo || !t;
  const ult = $("ultima-parada");
  const motivo = t && est === "PARADA" ? MOTIVO[t.motivo] : null;
  ult.hidden = !motivo;
  if (motivo) texto(ult, "Última parada: " + motivo);

  roda("e", t ? t.pwmEsq : 0);
  roda("d", t ? t.pwmDir : 0);
  const svg = $("cadeira");
  svg.setAttribute("data-estado", est || "PARADA");
  svg.classList.toggle("obst", !!t && t.obstaculo === 1);

  const nivel = t && t.nivel >= 1 && t.nivel <= 3 ? t.nivel : 0;
  texto($("vel-texto"), nivel ? "Velocidade " + nivel + " de 3" : "Velocidade – de 3");
  texto($("dpad-nivel"), nivel ? String(nivel) : "–");
  document.querySelectorAll(".nivel").forEach((n) => {
    const k = Number(n.dataset.n);
    n.classList.toggle("on", k < nivel);
    n.classList.toggle("atual", k === nivel);
  });

  const chipIr = $("chip-ir");
  chipIr.className = "chip" + (t ? (t.obstaculo === 1 ? " alerta" : " ok") : "");
  usar(chipIr.querySelector("use"), t && t.obstaculo === 1 ? "i-triangulo" : "i-livre");
  texto(chipIr.querySelector("span"), t ? (t.obstaculo === 1 ? "obstáculo" : "livre") : "–");
  const chipVoz = $("chip-voz");
  chipVoz.className = "chip" + (t && t.voz === 1 ? " ok" : "");
  texto(chipVoz.querySelector("span"), t ? (t.voz === 1 ? "pronto" : "indisponível") : "–");

  // Diagnostico
  texto($("dg-idade"), E.idadeTel === null ? "–" : "há " + duracao(E.idadeTel));
  texto($("dg-mega"), t ? duracao(t.ms) : "–");
  const esp = E.esp;
  texto($("dg-esp"), esp ? duracao(esp.uptime_ms) : "–");
  texto($("dg-clientes"), esp && typeof esp.clientes === "number" ? String(esp.clientes) : "–");
  texto($("dg-linhas"), esp ? (esp.linhas_ok ?? "–") + " / " + (esp.linhas_erro ?? "–") : "–");
  texto($("dg-id"), ID_CLIENTE + " · " + contadorN);

  renderControles();
  renderHistorico();
}

function renderControles() {
  const t = E.tel;
  const soAcompanha = !E.demo && !!t && t.app === 0;
  if (soAcompanha) E.travado = true;
  const trava = $("trava");
  trava.disabled = soAcompanha;
  trava.setAttribute("aria-checked", String(!E.travado));
  texto($("trava-estado"), E.travado ? "Bloqueado" : "Liberado");
  usar($("trava-icone"), E.travado ? "i-fechado" : "i-aberto");
  $("nota-app").hidden = !soAcompanha;
  $("comandos").hidden = E.travado;

  const pode = podeMover();
  document.querySelectorAll("[data-segurar], [data-toque]").forEach((b) => { b.disabled = !pode; });
  // Botao desabilitado no meio de um "segurar" nao recebe mais o soltar.
  if (!pode) encerrarSegurar(true);

  const nota = $("nota-controle");
  let msg = "";
  if (soAcompanha) msg = "";
  else if (E.travado) msg = "O app só acompanha. Ligue a chave para comandar a cadeira pelo celular.";
  else if (!(E.demo || E.conexao === "conectado") || !t) msg = "Sem conexão com a cadeira: os comandos ficam desligados até ela responder.";
  else if (t.emergencia === 1) msg = "Botão de emergência acionado: a cadeira ignora comandos até ele ser solto.";
  nota.hidden = !msg;
  if (msg) texto(nota, msg);
}

// ---------------------------------------------------------------------
// Abas
// ---------------------------------------------------------------------
function mostrarAba(nome, lembrar) {
  if (ABAS.indexOf(nome) < 0) nome = "painel";
  E.aba = nome;
  document.body.setAttribute("data-aba", nome);
  for (const a of ABAS) {
    const tab = $("tab-" + a);
    const sel = a === nome;
    tab.setAttribute("aria-selected", String(sel));
    tab.tabIndex = sel ? 0 : -1;
    $("aba-" + a).hidden = !sel;
  }
  if (nome !== "painel") encerrarSegurar(true);
  renderHistorico();
  if (lembrar) {
    try { history.replaceState(null, "", "#" + nome); } catch (e) { /* moldura sem historico */ }
  }
}

// ---------------------------------------------------------------------
// Demonstracao: porte simplificado do controle.cpp, rodando no navegador
// e respondendo no mesmo formato da API do ESP.
// ---------------------------------------------------------------------
function criarSimulador() {
  const NIVEIS_PWM = [130, 175, 220];
  const PWM_MINIMO = 90, FATOR_GIRO_PCT = 70, RAMPA_PASSO = 8, RAMPA_MS = 20;
  const LIMITE_MS = 3000, GIRO_MS = 700, TEL_MS = 250, MAX_EVENTOS = 20;
  const CODIGO = { frente: "FRENTE", tras: "RE", esquerda: "ESQUERDA", direita: "DIREITA", parar: "PARAR", rapido: "RAPIDO", devagar: "DEVAGAR" };
  const t0 = performance.now();
  const agora = () => Math.floor(performance.now() - t0);
  const c = { estado: "PARADA", nivel: 0, alvoE: 0, alvoD: 0, pwmE: 0, pwmD: 0, obst: false, motivo: "NENHUMA", tCmd: 0, tGiro: 0, tRampa: 0 };
  const s = { sensor: false, emergencia: false, seq: 0, eventos: [], pubEstado: "PARADA", pubObst: false, pubEmerg: false, tTel: 0, tel: null, linhas: 0 };

  const lim = (v, a, b) => (v < a ? a : v > b ? b : v);
  const aproximar = (a, alvo, p) => (a < alvo ? Math.min(a + p, alvo) : a > alvo ? Math.max(a - p, alvo) : a);
  // Inversao de sentido passa por zero, como no aproximarPorZero do Mega.
  const porZero = (a, alvo, p) => { const n = aproximar(a, alvo, p); return (a > 0 && n < 0) || (a < 0 && n > 0) ? 0 : n; };
  const base = () => lim(NIVEIS_PWM[lim(c.nivel, 0, 2)], PWM_MINIMO, 255);
  const giro = () => lim(Math.trunc(base() * FATOR_GIRO_PCT / 100), PWM_MINIMO, 255);
  const alvo = (e, d) => { c.alvoE = lim(e, -255, 255); c.alvoD = lim(d, -255, 255); };
  const pararSuave = (t, m) => { c.estado = "PARADA"; alvo(0, 0); c.motivo = m; c.tCmd = t; };
  const pararJa = (t, m) => { pararSuave(t, m); c.pwmE = 0; c.pwmD = 0; };
  const alvoDoEstado = () => {
    if (c.estado === "FRENTE") alvo(base(), base());
    else if (c.estado === "RE") alvo(-base(), -base());
    else if (c.estado === "GIRO_ESQ") alvo(-giro(), giro());
    else if (c.estado === "GIRO_DIR") alvo(giro(), -giro());
  };

  function aplicar(cmd, t) {
    switch (cmd) {
      case "parar": pararJa(t, "COMANDO"); return true;
      case "frente":
        if (c.obst) { pararJa(t, "OBSTACULO"); return false; }
        c.estado = "FRENTE"; c.motivo = "NENHUMA"; c.tCmd = t; alvoDoEstado(); return true;
      case "tras": c.estado = "RE"; c.motivo = "NENHUMA"; c.tCmd = t; alvoDoEstado(); return true;
      case "esquerda": c.estado = "GIRO_ESQ"; c.motivo = "NENHUMA"; c.tGiro = c.tCmd = t; alvoDoEstado(); return true;
      case "direita": c.estado = "GIRO_DIR"; c.motivo = "NENHUMA"; c.tGiro = c.tCmd = t; alvoDoEstado(); return true;
      case "rapido": case "devagar":
        c.nivel = lim(c.nivel + (cmd === "rapido" ? 1 : -1), 0, 2); c.tCmd = t; alvoDoEstado(); return true;
      default: return false;
    }
  }
  function renovar(cmd, t) {
    if (t - c.tCmd >= LIMITE_MS) return;
    if ((cmd === "frente" && c.estado === "FRENTE" && !c.obst) || (cmd === "tras" && c.estado === "RE")) c.tCmd = t;
  }
  function evento(d) {
    const t = agora();
    s.seq += 1;
    s.linhas += 1;
    s.eventos.push({ seq: s.seq, chegada: t, dados: Object.assign({ t: "evt", ms: t }, d) });
    if (s.eventos.length > MAX_EVENTOS) s.eventos.shift();
  }
  function publicar(t, emerg) {
    if (emerg !== s.pubEmerg) { s.pubEmerg = emerg; evento({ tipo: "emergencia", ativa: emerg ? 1 : 0 }); }
    if (c.obst !== s.pubObst) { s.pubObst = c.obst; evento({ tipo: "obstaculo", ativo: c.obst ? 1 : 0 }); }
    if (c.estado === "PARADA" && s.pubEstado !== "PARADA") evento({ tipo: "parada", motivo: c.motivo });
    s.pubEstado = c.estado;
    if (!s.tel || t - s.tTel >= TEL_MS) {
      s.tTel = t;
      s.linhas += 1;
      s.tel = { t: "tel", ms: t, estado: c.estado, pwmEsq: c.pwmE, pwmDir: c.pwmD, nivel: c.nivel + 1, obstaculo: c.obst ? 1 : 0, emergencia: emerg ? 1 : 0, motivo: c.motivo, voz: 1, app: 1 };
    }
  }
  function passo() {
    const t = agora();
    if (s.emergencia) { pararJa(t, "EMERGENCIA"); publicar(t, true); return; }
    c.obst = s.sensor;
    if (c.obst && c.estado === "FRENTE") pararJa(t, "OBSTACULO");
    if ((c.estado === "GIRO_ESQ" || c.estado === "GIRO_DIR") && t - c.tGiro >= GIRO_MS) pararSuave(t, "FIM_DO_GIRO");
    if (c.estado !== "PARADA" && t - c.tCmd >= LIMITE_MS) pararSuave(t, "TEMPO_LIMITE");
    if (t - c.tRampa >= RAMPA_MS) {
      c.tRampa = t;
      c.pwmE = porZero(c.pwmE, c.alvoE, RAMPA_PASSO);
      c.pwmD = porZero(c.pwmD, c.alvoD, RAMPA_PASSO);
    }
    publicar(t, false);
  }

  evento({ tipo: "inicio", voz: 1 });
  passo();
  const timer = setInterval(passo, RAMPA_MS);

  return {
    comando(nome, manter) {
      if (s.emergencia) return;  // como no Mega: com o botao apertado tudo e descartado
      const t = agora();
      if (manter) { renovar(nome, t); return; }
      if (!CODIGO[nome]) return;
      const aceito = aplicar(nome, t);
      evento({ tipo: "comando", cmd: CODIGO[nome], origem: "app", aceito: aceito ? 1 : 0 });
      passo();
      s.tTel = -TEL_MS;  // telemetria nova ja na proxima leitura
      passo();
    },
    estado(desde) {
      const t = agora();
      if (desde > s.seq) desde = 0;
      return {
        conectado: true,
        idade_ms: s.tel ? t - s.tTel : null,
        tel: s.tel,
        seq: s.seq,
        eventos: s.eventos.filter((e) => e.seq > desde).map((e) => ({ seq: e.seq, idade_ms: t - e.chegada, dados: e.dados })),
        esp: { uptime_ms: t, clientes: 1, linhas_ok: s.linhas, linhas_erro: 0 }
      };
    },
    alternar(qual) {
      s[qual] = !s[qual];
      passo();
      s.tTel = -TEL_MS;
      passo();
      return s[qual];
    },
    parar() { clearInterval(timer); }
  };
}

let timerDemo = 0;
function atualizarDemo() {
  if (E.demo) receberEstado(E.demo.estado(E.seq), "demo");
}

function reiniciarDados() {
  encerrarSegurar(true);
  E.travado = true;
  E.tel = null;
  E.idadeTel = null;
  E.esp = null;
  E.seq = 0;
  E.uptime = null;
  E.primeiro = true;
  E.falhas = 0;
  E.historico = [];
  E.histSujo = true;
}

function entrarDemo() {
  if (E.demo) return;
  reiniciarDados();
  clearTimeout(timerEstado);
  E.demo = criarSimulador();
  E.conexao = "demo";
  $("sim-obstaculo").setAttribute("aria-pressed", "false");
  $("sim-emergencia").setAttribute("aria-pressed", "false");
  timerDemo = setInterval(atualizarDemo, 200);
  atualizarDemo();
  mostrarAba("painel");
  window.scrollTo(0, 0);
}

function sairDemo() {
  if (!E.demo) return;
  encerrarSegurar(false);
  clearInterval(timerDemo);
  E.demo.parar();
  E.demo = null;
  reiniciarDados();
  E.conexao = "iniciando";
  renderizar();
  agendarEstado(0);
}

// ---------------------------------------------------------------------
// Equipe
// ---------------------------------------------------------------------
function renderEquipe() {
  const curso = String(EQUIPE.curso || "").trim();
  $("sobre-curso").textContent = "Trabalho de Conclusão de Curso" + (curso ? " · " + curso : "");
  const nomes = (Array.isArray(EQUIPE.integrantes) ? EQUIPE.integrantes : []).map((n) => String(n).trim()).filter(Boolean);
  const sec = $("sobre-equipe");
  sec.hidden = nomes.length === 0;
  if (!nomes.length) return;
  const dl = $("equipe-dados");
  dl.textContent = "";
  const linha = (rotulo, valor) => {
    const div = document.createElement("div");
    const dt = document.createElement("dt");
    dt.textContent = rotulo;
    const dd = document.createElement("dd");
    if (typeof valor === "string") dd.textContent = valor; else dd.appendChild(valor);
    div.append(dt, dd);
    dl.appendChild(div);
  };
  const ul = document.createElement("ul");
  for (const n of nomes) { const li = document.createElement("li"); li.textContent = n; ul.appendChild(li); }
  linha(nomes.length === 1 ? "Integrante" : "Integrantes", ul);
  const extra = [["Orientação", EQUIPE.orientador], ["Instituição", EQUIPE.instituicao], ["Curso", EQUIPE.curso], ["Ano", EQUIPE.ano]];
  for (const [r, v] of extra) { const s = String(v || "").trim(); if (s) linha(r, s); }
}

// ---------------------------------------------------------------------
// Ligacoes
// ---------------------------------------------------------------------
document.querySelectorAll("[data-segurar]").forEach((b) => {
  const cmd = b.dataset.segurar;
  b.addEventListener("pointerdown", (e) => {
    if (e.pointerType === "mouse" && e.button !== 0) return;
    e.preventDefault();
    if (segurando) { conflito(); return; }
    if (!podeMover()) return;
    try { b.setPointerCapture(e.pointerId); } catch (x) { /* ponteiro sem captura */ }
    if (iniciarSegurar(cmd, b, e.pointerId, null)) vibrar(15);
  });
  const soltar = (e) => { if (segurando && segurando.ponteiro === e.pointerId) encerrarSegurar(true); };
  b.addEventListener("pointerup", soltar);
  b.addEventListener("pointercancel", soltar);
  b.addEventListener("lostpointercapture", soltar);
});
// Rede de seguranca para um pointerup que nao chegou ao botao.
const soltarGlobal = (e) => { if (segurando && segurando.ponteiro !== null && segurando.ponteiro === e.pointerId) encerrarSegurar(true); };
document.addEventListener("pointerup", soltarGlobal, true);
document.addEventListener("pointercancel", soltarGlobal, true);
const soltarToque = (e) => { if (segurando && segurando.ponteiro !== null && e.touches.length === 0) encerrarSegurar(true); };
document.addEventListener("touchend", soltarToque, true);
document.addEventListener("touchcancel", soltarToque, true);

document.querySelectorAll("[data-toque]").forEach((b) => {
  b.addEventListener("click", () => tocar(b.dataset.toque));
});

const btnParar = $("btn-parar");
btnParar.addEventListener("pointerdown", (e) => {
  if (e.pointerType === "mouse" && e.button !== 0) return;
  e.preventDefault();
  acionarParar();
});
// Enter com o foco no botao (o clique de teclado tem detail 0).
btnParar.addEventListener("click", (e) => { if (e.detail === 0) acionarParar(); });

document.addEventListener("contextmenu", (e) => {
  if (e.target instanceof Element && e.target.closest(".dpad, .velocidades, .dock")) e.preventDefault();
});

$("trava").addEventListener("click", () => {
  if ($("trava").disabled) return;
  E.travado = !E.travado;
  if (E.travado) encerrarSegurar(true);
  vibrar(10);
  renderControles();
});

for (const a of ABAS) $("tab-" + a).addEventListener("click", () => mostrarAba(a, true));
// Padrao ARIA de abas: so a aba ativa fica no Tab; setas, Home e End trocam.
// A seta para aqui: numa aba ela navega, nunca gira a cadeira.
document.querySelector(".abas").addEventListener("keydown", (e) => {
  const i = ABAS.indexOf(E.aba);
  const j = { ArrowLeft: i - 1, ArrowRight: i + 1, Home: 0, End: ABAS.length - 1 }[e.key];
  if (j === undefined || e.ctrlKey || e.metaKey || e.altKey) return;
  e.preventDefault();
  e.stopPropagation();
  const nova = ABAS[(j + ABAS.length) % ABAS.length];
  mostrarAba(nova, true);
  $("tab-" + nova).focus();
});
$("btn-demo").addEventListener("click", entrarDemo);
$("btn-sair-demo").addEventListener("click", sairDemo);
$("sim-obstaculo").addEventListener("click", (e) => {
  if (!E.demo) return;
  e.currentTarget.setAttribute("aria-pressed", String(E.demo.alternar("sensor")));
  atualizarDemo();
});
$("sim-emergencia").addEventListener("click", (e) => {
  if (!E.demo) return;
  e.currentTarget.setAttribute("aria-pressed", String(E.demo.alternar("emergencia")));
  atualizarDemo();
});
$("btn-limpar").addEventListener("click", () => {
  E.historico = [];
  E.histSujo = true;
  renderHistorico();
});

// Teclado no PC. Espaco para sempre; o resto so com o controle liberado e
// no painel, onde o direcional esta visivel.
const TECLA_SEGURAR = { ArrowUp: "frente", ArrowDown: "tras" };
const TECLA_TOQUE = { ArrowLeft: "esquerda", ArrowRight: "direita", "+": "rapido", "=": "rapido", "-": "devagar" };
const botaoDe = (cmd) => $(cmd === "tras" ? "btn-re" : "btn-" + cmd);
document.addEventListener("keydown", (e) => {
  if (e.ctrlKey || e.metaKey || e.altKey) return;
  if (e.key === " " || e.code === "Space") {
    e.preventDefault();
    if (!e.repeat) acionarParar();
    return;
  }
  // Com o foco numa aba, as teclas sao da navegacao (acima), nao de conducao.
  if (e.target instanceof Element && e.target.closest('[role="tab"]')) return;
  const alvo = e.target instanceof Element ? e.target.closest("[data-segurar]") : null;
  const cmdSeg = TECLA_SEGURAR[e.key] || (e.key === "Enter" && alvo ? alvo.dataset.segurar : null);
  if (cmdSeg) {
    if (E.travado || E.aba !== "painel") return;
    e.preventDefault();
    if (e.repeat) return;
    if (segurando) { if (segurando.tecla !== e.key) conflito(); return; }
    iniciarSegurar(cmdSeg, botaoDe(cmdSeg), null, e.key);
    return;
  }
  const cmdToque = TECLA_TOQUE[e.key];
  if (cmdToque && !E.travado && E.aba === "painel") {
    e.preventDefault();
    if (!e.repeat) tocar(cmdToque);
  }
});
document.addEventListener("keyup", (e) => {
  if (e.key === " " || e.code === "Space") e.preventDefault();
  if (segurando && segurando.tecla === e.key) encerrarSegurar(true);
});

// Perdeu o foco, escondeu a pagina ou saiu dela: o "soltar" talvez nunca chegue.
window.addEventListener("blur", () => encerrarSegurar(true));
window.addEventListener("pagehide", () => encerrarSegurar(true));
document.addEventListener("visibilitychange", () => { if (document.hidden) encerrarSegurar(true); });

try { if (window.matchMedia("(hover: hover) and (pointer: fine)").matches) $("atalhos").hidden = false; } catch (e) { /* sem matchMedia */ }

// A faixa da demo muda a altura do cabecalho; o PARAR do PC se alinha logo abaixo dele.
const cabecalho = document.querySelector(".topo");
const medirCabecalho = () => document.documentElement.style.setProperty("--cabecalho", cabecalho.offsetHeight + "px");
try { new ResizeObserver(medirCabecalho).observe(cabecalho); } catch (e) { window.addEventListener("resize", medirCabecalho); }
medirCabecalho();

let abaInicial = "painel";
try { const h = location.hash.slice(1); if (ABAS.indexOf(h) >= 0) abaInicial = h; } catch (e) { /* sem hash */ }
renderEquipe();
mostrarAba(abaInicial);
renderizar();
consultarEstado();
})();
</script>
</body>
</html>
)PAGINA_HTML";

#endif  // PAGINA_H
