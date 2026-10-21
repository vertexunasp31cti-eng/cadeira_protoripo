/*
 * pagina.h - O app de controle, servido pelo ESP8266 em http://192.168.4.1
 *
 * E uma pagina unica (HTML + CSS + JavaScript) sem nenhum arquivo externo,
 * porque o celular conectado na rede da cadeira nao tem internet.
 *
 * Como o app conversa com a cadeira:
 *   GET /cmd?c=frente   envia um comando (frente, re, esquerda, direita,
 *                       parar, rapido, devagar)
 *   GET /estado         ligacao com o Mega + ultima telemetria + login,
 *                       a cada 250 ms
 *   GET /login          registra o nome e a data do primeiro acesso
 *
 * Na primeira vez o app pede o nome. A data desse login fica gravada no
 * ESP8266, e o app conta ha quantos dias a cadeira esta em uso com o relogio
 * do proprio celular, porque o ESP8266 nao tem relogio.
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
.info{display:grid;grid-template-columns:1fr 1fr;gap:12px;margin-bottom:12px}
.info .cartao{margin:0}
.grande{font-size:28px;font-weight:700;line-height:1.1;margin-top:2px;font-variant-numeric:tabular-nums}
.grande small{font-size:14px;font-weight:600;color:var(--suave)}
.sub{font-size:12px;color:var(--suave);margin-top:4px}
.pilha{height:8px;border-radius:4px;background:var(--fundo);margin-top:8px;overflow:hidden}
.pilha span{display:block;height:100%;width:0;background:var(--ok);transition:width .3s}
.rodape{display:flex;justify-content:center;gap:8px;flex-wrap:wrap;font-size:13px;color:var(--suave);margin-top:10px}
.link{background:none;border:0;min-height:0;padding:0;font-size:13px;font-weight:600;color:var(--acao);text-decoration:underline;touch-action:manipulation}
.confirma{display:none;margin-top:10px;text-align:center;font-size:14px}
.confirma.mostrar{display:block}
.confirma div{display:flex;gap:8px;justify-content:center;margin-top:8px}
.confirma button{min-height:44px;padding:0 16px;font-size:15px}
#login{display:none}
.sem-login #login{display:block}
.sem-login #principal{display:none}
#login h2{font-size:22px;margin:8px 0 4px}
#login p{color:var(--suave);margin:0 0 16px}
#login label{display:block;font-size:13px;color:var(--suave);margin-bottom:6px}
#login input{width:100%;font:inherit;font-size:18px;padding:14px;border-radius:12px;border:1px solid var(--borda);
background:var(--fundo);color:var(--texto);margin-bottom:12px}
#login input:focus{outline:2px solid var(--acao);outline-offset:1px}
#login button{width:100%;min-height:56px;background:var(--acao);border-color:var(--acao)}
#login-erro{color:var(--perigo);font-size:14px;min-height:20px;margin-top:8px}
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

<form id="login" class="cartao" autocomplete="off">
  <h2>Bem-vindo</h2>
  <p>Primeiro acesso a esta cadeira. A partir de hoje o app conta há quantos dias ela está em uso.</p>
  <label for="login-nome">Seu nome</label>
  <input id="login-nome" maxlength="20" required placeholder="Ex.: Ana">
  <button type="submit">Entrar</button>
  <div id="login-erro" role="alert"></div>
</form>

<div id="principal">
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

<section class="info">
  <div class="cartao">
    <div class="rotulo">Bateria</div>
    <div class="grande" id="bat-pct">--</div>
    <div class="pilha"><span id="bat-barra"></span></div>
    <div class="sub" id="bat-sub">estimada pela tensão</div>
  </div>
  <div class="cartao">
    <div class="rotulo">Em uso há</div>
    <div class="grande" id="dias">--</div>
    <div class="sub" id="dias-sub">desde o login</div>
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
<div class="rodape"><span id="usuario"></span><button type="button" class="link" id="refazer">Refazer login</button></div>
<div class="confirma" id="confirma">
  Apagar o login e começar a contar os dias de novo?
  <div><button type="button" id="confirma-sim">Apagar</button><button type="button" id="confirma-nao">Cancelar</button></div>
</div>
</div>

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
  // Sem login os botoes ficam escondidos; as setas do teclado tambem nao valem.
  if(botao.disabled||document.body.classList.contains("sem-login")) return;
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
// --- Bateria e dias de uso ------------------------------------------------
function bateria(c){
  var pct=c?c.batPct:-1, b=$("bat-barra");
  if(pct==null||pct<0){
    $("bat-pct").textContent="--"; b.style.width="0";
    $("bat-sub").textContent=c?"não medida (divisor no A0)":"estimada pela tensão";
    return;
  }
  $("bat-pct").textContent=pct+"%";
  b.style.width=pct+"%";
  b.style.background=pct<=15?"var(--perigo)":pct<=35?"var(--alerta)":"var(--ok)";
  $("bat-sub").textContent=(c.batMv/1000).toFixed(1).replace(".",",")+" V · estimada";
}
// Dias de calendario entre o login e hoje, no fuso do celular.
function diasDesde(seg){
  var a=new Date(seg*1000), h=new Date(Date.now());
  var d0=new Date(a.getFullYear(),a.getMonth(),a.getDate()), d1=new Date(h.getFullYear(),h.getMonth(),h.getDate());
  return Math.max(0,Math.round((d1-d0)/86400000));
}
function dataBr(seg){
  var a=new Date(seg*1000);
  return ("0"+a.getDate()).slice(-2)+"/"+("0"+(a.getMonth()+1)).slice(-2)+"/"+a.getFullYear();
}
function login(l){
  document.body.classList.toggle("sem-login",!l);
  if(!l){ $("dias").textContent="--"; $("usuario").textContent=""; return; }
  var n=diasDesde(l.desde);
  $("dias").innerHTML=n===0?"hoje":n+" <small>"+(n===1?"dia":"dias")+"</small>";
  $("dias-sub").textContent="desde "+dataBr(l.desde);
  $("usuario").textContent="Logado como "+l.nome+" ·";
}
$("login").addEventListener("submit",function(e){
  e.preventDefault();
  var nome=$("login-nome").value.trim();
  if(!nome){ $("login-erro").textContent="Digite seu nome."; return; }
  $("login-erro").textContent="";
  fetch("/login?nome="+encodeURIComponent(nome)+"&t="+Math.floor(Date.now()/1000),{cache:"no-store"})
    .then(function(r){ if(!r.ok) throw 0; $("login-nome").value=""; document.body.classList.remove("sem-login"); })
    .catch(function(){ $("login-erro").textContent="Não foi possível entrar. Confira o Wi-Fi e a hora do celular."; });
});
$("refazer").addEventListener("click",function(){ $("confirma").classList.add("mostrar"); });
$("confirma-nao").addEventListener("click",function(){ $("confirma").classList.remove("mostrar"); });
$("confirma-sim").addEventListener("click",function(){
  $("confirma").classList.remove("mostrar"); soltar(false); enviar("parar");
  fetch("/login?apagar=1",{cache:"no-store"}).catch(function(){});
});

function mostrar(d){
  var c=d.cadeira;
  login(d.login);
  cadeiraOk=!!d.mega && !!c;
  ligar("lig-wifi",true); ligar("lig-cadeira",cadeiraOk);
  habilitar(cadeiraOk && !(c && c.emergencia));
  if(!cadeiraOk){ limpar("Aguardando"); bateria(null); aviso("Wi-Fi conectado, mas a cadeira não responde. Confira as chaves DIP 1 e 2.","alerta"); return; }
  $("estado").textContent=c.emergencia?"EMERGÊNCIA":(NOMES[c.estado]||c.estado);
  Array.prototype.forEach.call($("niveis").children,function(b,i){ b.classList.toggle("on",i<c.nivel); });
  barra("esq",c.pwmEsq); barra("dir",c.pwmDir);
  $("motivo").textContent=c.motivo;
  $("voz").textContent=c.voz?"pronta":"indisponível";
  bateria(c);
  if(c.emergencia) aviso("Botão de emergência acionado. Solte o botão na cadeira para voltar.");
  else if(c.obstaculo) aviso("Obstáculo à frente! Só a ré está liberada.");
  else if(c.batPct>=0&&c.batPct<=15) aviso("Bateria fraca ("+c.batPct+"%). Carregue antes de continuar.","alerta");
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
