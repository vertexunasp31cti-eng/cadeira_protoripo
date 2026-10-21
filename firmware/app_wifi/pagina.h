/*
 * pagina.h - O app de controle, servido pelo ESP8266 em http://192.168.4.1
 *
 * E uma pagina unica (HTML + CSS + JavaScript) sem nenhum arquivo externo,
 * porque o celular conectado na rede da cadeira nao tem internet.
 *
 * Como o app conversa com a cadeira:
 *   GET /cmd?c=frente   envia um comando (frente, re, esquerda, direita,
 *                       parar, rapido, devagar)
 *   GET /estado         ligacao com o Mega + ultima telemetria, a cada 250 ms
 *
 * Botoes de movimento funcionam como "segure para andar": enquanto o dedo
 * esta no botao o comando e repetido a cada 300 ms, e ao soltar o app manda
 * PARAR. Se o celular perder a conexao no meio, o ESP8266 para a cadeira
 * sozinho (LIMITE_APP_MS em app_wifi.ino).
 */
#ifndef PAGINA_H
#define PAGINA_H

#include <Arduino.h>

const char PAGINA_APP[] PROGMEM = R"PAGINA(<!doctype html>
<html lang="pt-BR">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no">
<meta name="theme-color" content="#101418">
<title>Cadeira TCC</title>
<style>
:root{--fundo:#101418;--cartao:#1a2026;--borda:#2a323b;--texto:#e8edf2;--suave:#8a96a3;
--ok:#2fbf71;--alerta:#f2a93b;--perigo:#e5484d;--acao:#3b82f6;--acao2:#24324a}
*{box-sizing:border-box;-webkit-tap-highlight-color:transparent}
html,body{margin:0;background:var(--fundo);color:var(--texto);
font:16px/1.4 system-ui,-apple-system,"Segoe UI",Roboto,sans-serif}
body{max-width:480px;margin:0 auto;padding:12px 16px 24px;
user-select:none;-webkit-user-select:none;touch-action:manipulation}
header{display:flex;align-items:center;justify-content:space-between;gap:8px;margin-bottom:12px}
h1{font-size:20px;margin:0}
.ligacoes{display:flex;gap:6px;flex-wrap:wrap;justify-content:flex-end}
.pilula{display:flex;align-items:center;gap:6px;font-size:12px;padding:4px 10px;
border-radius:999px;background:var(--cartao);border:1px solid var(--borda);color:var(--suave)}
.pilula i{width:8px;height:8px;border-radius:50%;background:var(--perigo)}
.pilula.on i{background:var(--ok)}
.aviso{display:none;padding:10px 12px;border-radius:10px;margin-bottom:12px;font-weight:600}
.aviso.mostrar{display:block}
.aviso.perigo{background:var(--perigo);color:#fff}
.aviso.alerta{background:var(--alerta);color:#1a1200}
.cartao{background:var(--cartao);border:1px solid var(--borda);border-radius:14px;padding:14px;margin-bottom:12px}
.linha{display:flex;justify-content:space-between;align-items:baseline;gap:8px}
.estado{font-size:26px;font-weight:700;letter-spacing:.5px}
.rotulo{font-size:12px;color:var(--suave);text-transform:uppercase;letter-spacing:.6px}
.niveis{display:flex;gap:4px}
.niveis b{width:18px;height:8px;border-radius:3px;background:var(--borda)}
.niveis b.on{background:var(--acao)}
.barras{margin-top:12px;display:grid;gap:8px}
.barra{display:grid;grid-template-columns:70px 1fr 40px;align-items:center;gap:8px;font-size:13px;color:var(--suave)}
.trilho{position:relative;height:10px;background:var(--fundo);border-radius:5px;overflow:hidden}
.trilho::after{content:"";position:absolute;left:50%;top:0;bottom:0;width:1px;background:var(--borda)}
.trilho span{position:absolute;top:0;bottom:0;background:var(--acao);transition:all .15s}
.barra em{font-style:normal;text-align:right;color:var(--texto);font-variant-numeric:tabular-nums}
.detalhes{margin-top:10px;font-size:13px;color:var(--suave);display:flex;justify-content:space-between;gap:8px;flex-wrap:wrap}
.detalhes strong{color:var(--texto);font-weight:600}
.controle{display:grid;grid-template-columns:repeat(3,1fr);gap:10px;margin:4px 0 12px}
button{font:inherit;color:var(--texto);background:var(--acao2);border:1px solid var(--borda);
border-radius:16px;min-height:84px;font-size:18px;font-weight:600;cursor:pointer;touch-action:none}
button .seta{display:block;font-size:30px;line-height:1}
button.ativo{background:var(--acao);border-color:var(--acao)}
button:disabled{opacity:.35}
#parar{background:var(--perigo);border-color:var(--perigo);color:#fff;font-size:20px}
#parar:active{filter:brightness(.85)}
.vazio{visibility:hidden}
.velocidade{display:grid;grid-template-columns:1fr 1fr;gap:10px}
.velocidade button{min-height:56px;font-size:16px}
.dica{text-align:center;font-size:13px;color:var(--suave);margin-top:14px}
</style>
</head>
<body>
<header>
  <h1>Cadeira TCC</h1>
  <div class="ligacoes">
    <div class="pilula" id="lig-wifi"><i></i>Wi-Fi</div>
    <div class="pilula" id="lig-cadeira"><i></i>Cadeira</div>
  </div>
</header>

<div class="aviso perigo" id="aviso"></div>

<section class="cartao" aria-live="polite">
  <div class="linha">
    <div>
      <div class="rotulo">Estado</div>
      <div class="estado" id="estado">--</div>
    </div>
    <div style="text-align:right">
      <div class="rotulo">Velocidade</div>
      <div class="niveis" id="niveis"><b></b><b></b><b></b></div>
    </div>
  </div>
  <div class="barras">
    <div class="barra">Roda esq.<div class="trilho"><span id="barra-esq"></span></div><em id="pwm-esq">0</em></div>
    <div class="barra">Roda dir.<div class="trilho"><span id="barra-dir"></span></div><em id="pwm-dir">0</em></div>
  </div>
  <div class="detalhes">
    <span>Última parada: <strong id="motivo">-</strong></span>
    <span>Voz: <strong id="voz">-</strong></span>
  </div>
</section>

<section class="controle">
  <span class="vazio"></span>
  <button class="mover" data-c="frente"><span class="seta">&#9650;</span>Frente</button>
  <span class="vazio"></span>
  <button class="mover" data-c="esquerda"><span class="seta">&#9664;</span>Esquerda</button>
  <button id="parar">PARAR</button>
  <button class="mover" data-c="direita"><span class="seta">&#9654;</span>Direita</button>
  <span class="vazio"></span>
  <button class="mover" data-c="re"><span class="seta">&#9660;</span>Ré</button>
  <span class="vazio"></span>
</section>

<section class="velocidade">
  <button data-c="devagar" class="vel">&minus; Devagar</button>
  <button data-c="rapido" class="vel">+ Rápido</button>
</section>

<p class="dica">Segure a seta para andar. Ao soltar o dedo, a cadeira para.</p>

<script>
(function(){
var REPETIR_MS=300, CONSULTA_MS=250, TEMPO_RESPOSTA_MS=900;
var NOMES={PARADA:"Parada",FRENTE:"Frente",RE:"Ré",GIRO_ESQ:"Girando à esquerda",GIRO_DIR:"Girando à direita"};
var $=function(id){return document.getElementById(id)};
var segurando=null, repeticao=null, falhas=0, cadeiraOk=false;

function enviar(c){
  return fetch("/cmd?c="+c,{cache:"no-store"}).catch(function(){});
}
function vibrar(){ if(navigator.vibrate) navigator.vibrate(15); }

// --- Botoes de movimento: segure para andar --------------------------------
function comecar(botao){
  if(botao.disabled) return;
  soltar(false);
  segurando=botao; botao.classList.add("ativo"); vibrar();
  enviar(botao.dataset.c);
  repeticao=setInterval(function(){ enviar(botao.dataset.c); },REPETIR_MS);
}
function soltar(mandarParar){
  if(!segurando) return;
  clearInterval(repeticao); repeticao=null;
  segurando.classList.remove("ativo"); segurando=null;
  if(mandarParar!==false) enviar("parar");
}
Array.prototype.forEach.call(document.querySelectorAll(".mover"),function(b){
  b.addEventListener("pointerdown",function(e){ e.preventDefault(); b.setPointerCapture(e.pointerId); comecar(b); });
  ["pointerup","pointercancel","lostpointercapture"].forEach(function(ev){
    b.addEventListener(ev,function(){ if(segurando===b) soltar(); });
  });
  b.addEventListener("contextmenu",function(e){ e.preventDefault(); });
});
$("parar").addEventListener("pointerdown",function(e){ e.preventDefault(); soltar(false); vibrar(); enviar("parar"); });
Array.prototype.forEach.call(document.querySelectorAll(".vel"),function(b){
  b.addEventListener("click",function(){ vibrar(); enviar(b.dataset.c); });
});
// Saiu do app (trocou de aba, bloqueou a tela): para na hora.
document.addEventListener("visibilitychange",function(){ if(document.hidden){ soltar(false); enviar("parar"); } });
window.addEventListener("blur",function(){ if(segurando) soltar(); });

// Teclado, para testar no computador: setas, espaco, + e -.
var TECLAS={ArrowUp:"frente",ArrowDown:"re",ArrowLeft:"esquerda",ArrowRight:"direita"};
document.addEventListener("keydown",function(e){
  if(e.repeat) return;
  var c=TECLAS[e.key];
  if(c){ e.preventDefault(); comecar(document.querySelector('[data-c="'+c+'"]')); }
  else if(e.key===" "){ e.preventDefault(); soltar(false); enviar("parar"); }
  else if(e.key==="+") enviar("rapido");
  else if(e.key==="-") enviar("devagar");
});
document.addEventListener("keyup",function(e){ if(TECLAS[e.key]) soltar(); });

// --- Estado da cadeira -----------------------------------------------------
function ligar(id,on){ $(id).classList.toggle("on",!!on); }
function barra(id,pwm){
  var el=$("barra-"+id), p=Math.min(Math.abs(pwm),255)/255*50;
  el.style.left=(pwm>=0?50:50-p)+"%"; el.style.width=p+"%";
  el.style.background=pwm<0?"var(--alerta)":"var(--acao)";
  $("pwm-"+id).textContent=pwm;
}
function aviso(texto,tipo){
  var a=$("aviso"); a.textContent=texto||"";
  a.className="aviso "+(tipo||"perigo")+(texto?" mostrar":"");
}
function habilitar(on){
  Array.prototype.forEach.call(document.querySelectorAll(".mover,.vel"),function(b){ b.disabled=!on; });
  if(!on) soltar(false);
}
function limpar(texto){
  $("estado").textContent=texto; barra("esq",0); barra("dir",0);
  $("motivo").textContent="-"; $("voz").textContent="-";
  Array.prototype.forEach.call($("niveis").children,function(b){ b.classList.remove("on"); });
}
function mostrar(d){
  var c=d.cadeira;
  cadeiraOk=!!d.mega && !!c;
  ligar("lig-wifi",true); ligar("lig-cadeira",cadeiraOk);
  habilitar(cadeiraOk && !(c && c.emergencia));
  if(!cadeiraOk){ limpar("Aguardando"); aviso("Wi-Fi conectado, mas a cadeira não responde. Confira as chaves DIP 1 e 2.","alerta"); return; }
  $("estado").textContent=c.emergencia?"EMERGÊNCIA":(NOMES[c.estado]||c.estado);
  Array.prototype.forEach.call($("niveis").children,function(b,i){ b.classList.toggle("on",i<c.nivel); });
  barra("esq",c.pwmEsq); barra("dir",c.pwmDir);
  $("motivo").textContent=c.motivo;
  $("voz").textContent=c.voz?"pronta":"indisponível";
  if(c.emergencia) aviso("Botão de emergência acionado. Solte o botão na cadeira para voltar.");
  else if(c.obstaculo) aviso("Obstáculo à frente! Só a ré está liberada.");
  else aviso("");
  var f=document.querySelector('[data-c="frente"]');
  if(c.obstaculo) { f.disabled=true; if(segurando===f) soltar(false); }
}
function consultar(){
  var ctl=window.AbortController?new AbortController():null;
  var t=setTimeout(function(){ if(ctl) ctl.abort(); },TEMPO_RESPOSTA_MS);
  fetch("/estado",{cache:"no-store",signal:ctl?ctl.signal:undefined})
    .then(function(r){ return r.json(); })
    .then(function(d){ falhas=0; mostrar(d); })
    .catch(function(){
      if(++falhas>=3){
        ligar("lig-wifi",false); ligar("lig-cadeira",false); habilitar(false);
        limpar("Sem conexão");
        aviso("Sem conexão. Conecte o celular à rede Wi-Fi “Cadeira-TCC”.");
      }
    })
    .then(function(){ clearTimeout(t); setTimeout(consultar,CONSULTA_MS); });
}
habilitar(false);
consultar();
})();
</script>
</body>
</html>
)PAGINA";

#endif  // PAGINA_H
