/*
 * e2e_app.js - Teste de ponta a ponta: o app no navegador contra o codigo
 * real do ESP8266 e do Mega, rodando no simulador (servidor.py).
 *
 * Precisa de Node e do Playwright com o Chromium:
 *
 *   NODE_PATH=$(npm root -g) node testes/simulador/e2e_app.js
 *
 * Cada verificacao confere o que o app mostra e tambem o que a cadeira fez
 * de verdade, lendo /api/estado direto do ESP simulado.
 */
"use strict";

const { chromium } = require("playwright");
const { spawn } = require("child_process");
const http = require("http");
const net = require("net");
const path = require("path");

const SERVIDOR = path.join(__dirname, "servidor.py");
const BUILD = path.join(__dirname, "..", "build");

let total = 0;
let falhas = 0;
function verificar(ok, descricao) {
  total++;
  if (!ok) falhas++;
  console.log(`  [${ok ? "ok" : "FALHA"}]${ok ? "   " : " "} ${descricao}`);
}
function titulo(s) {
  console.log(`\n${s}`);
}
const esperar = (ms) => new Promise((r) => setTimeout(r, ms));

function portaLivre() {
  return new Promise((resolve) => {
    const s = net.createServer();
    s.listen(0, "127.0.0.1", () => {
      const porta = s.address().port;
      s.close(() => resolve(porta));
    });
  });
}

// Sobe o servidor.py e espera ele anunciar o endereco.
function ligarSimulador(porta, extras = []) {
  return new Promise((resolve, reject) => {
    const p = spawn("python3", [SERVIDOR, "--porta", String(porta), ...extras], {
      stdio: ["ignore", "pipe", "inherit"],
    });
    let saida = "";
    const prazo = setTimeout(() => reject(new Error("o simulador nao subiu: " + saida)), 60000);
    p.stdout.on("data", (d) => {
      saida += d;
      if (saida.includes("http://localhost:")) {
        clearTimeout(prazo);
        resolve(p);
      }
    });
    p.on("exit", (c) => reject(new Error(`o simulador saiu com codigo ${c}: ${saida}`)));
  });
}

function pedir(porta, caminho, corpo) {
  return new Promise((resolve, reject) => {
    const opcoes = { host: "127.0.0.1", port: porta, path: caminho, method: corpo ? "POST" : "GET" };
    if (corpo) opcoes.headers = { "Content-Type": "application/x-www-form-urlencoded" };
    const r = http.request(opcoes, (res) => {
      let dados = "";
      res.on("data", (d) => (dados += d));
      res.on("end", () => {
        try {
          resolve(JSON.parse(dados));
        } catch (e) {
          resolve(null);
        }
      });
    });
    r.on("error", reject);
    if (corpo) r.write(corpo);
    r.end();
  });
}

// Le a cadeira direto do ESP, sem passar pelo app.
async function cadeira(porta) {
  const e = await pedir(porta, "/api/estado");
  return e && e.tel ? e.tel : {};
}

// Espera ate a condicao valer, lendo a cadeira a cada 100 ms.
async function ate(porta, condicao, prazo_ms) {
  const fim = Date.now() + prazo_ms;
  for (;;) {
    const t = await cadeira(porta);
    if (condicao(t)) return t;
    if (Date.now() > fim) return t;
    await esperar(100);
  }
}

async function texto(pagina, seletor) {
  return ((await pagina.textContent(seletor)) || "").trim();
}

async function visivel(pagina, seletor) {
  return pagina.isVisible(seletor);
}

// Segura um botao com o ponteiro, como um dedo, e devolve a funcao que solta.
async function segurar(pagina, seletor) {
  const caixa = await pagina.locator(seletor).boundingBox();
  await pagina.mouse.move(caixa.x + caixa.width / 2, caixa.y + caixa.height / 2);
  await pagina.mouse.down();
  return () => pagina.mouse.up();
}

async function abrirApp(navegador, porta) {
  const contexto = await navegador.newContext({
    viewport: { width: 390, height: 844 },
    deviceScaleFactor: 2,
    isMobile: true,
    hasTouch: true,
    locale: "pt-BR",
  });
  const pagina = await contexto.newPage();
  const erros = [];
  pagina.on("pageerror", (e) => erros.push(String(e)));
  pagina.on("console", (m) => {
    if (m.type() === "error" && !/Failed to load resource|net::ERR/.test(m.text())) erros.push(m.text());
  });
  await pagina.goto(`http://localhost:${porta}/`);
  return { contexto, pagina, erros };
}

async function principal(navegador) {
  const porta = await portaLivre();
  const sim = await ligarSimulador(porta);
  const { contexto, pagina, erros } = await abrirApp(navegador, porta);
  try {
    // ------------------------------------------------------------------
    titulo("1. Conexao e historico");
    await pagina.waitForFunction(() => document.getElementById("pilula").textContent.includes("Conectado"), null, { timeout: 8000 });
    // A primeira resposta do ESP pode chegar antes da primeira telemetria do
    // Mega; os botoes so liberam com uma leitura da cadeira na mao.
    await pagina.waitForFunction(() => !document.getElementById("estado-nome").textContent.includes("Sem leitura"), null, { timeout: 5000 });
    verificar(true, "o app mostra Conectado e a leitura da cadeira");
    await pagina.click("#tab-historico");
    await pagina.waitForSelector("#hist-lista >> text=Cadeira ligada", { timeout: 3000 });
    verificar(true, "o historico mostra \"Cadeira ligada\" (evento inicio do Mega)");
    await pagina.click("#tab-painel");

    // ------------------------------------------------------------------
    titulo("2. Controle bloqueado e segure para andar");
    verificar(await pagina.isDisabled("#btn-frente"), "o controle comeca bloqueado (Frente desabilitado)");
    await pagina.click("#trava");
    verificar(!(await pagina.isDisabled("#btn-frente")), "desbloqueado, Frente fica disponivel");

    let soltar = await segurar(pagina, "#btn-frente");
    let t = await ate(porta, (x) => x.estado === "FRENTE" && x.pwmEsq === 130, 3000);
    verificar(t.estado === "FRENTE" && t.pwmEsq === 130 && t.pwmDir === 130,
      "segurando Frente, o Mega anda e a rampa chega a PWM 130 (nivel 1)");
    await esperar(3500);
    t = await cadeira(porta);
    verificar(t.estado === "FRENTE", "segurando por mais de 3 s, as renovacoes mantem a cadeira andando");
    await pagina.waitForFunction(() => document.getElementById("estado-nome").textContent.includes("Andando para frente"), null, { timeout: 2000 });
    verificar(true, "o painel mostra \"Andando para frente\"");
    await soltar();
    t = await ate(porta, (x) => x.estado === "PARADA", 1500);
    verificar(t.estado === "PARADA" && t.motivo === "COMANDO", "soltou, parou (parada por comando do usuario)");
    await pagina.waitForFunction(() => document.getElementById("ultima-parada").textContent.includes("comando do usuário"), null, { timeout: 2000 });
    verificar(true, "o painel mostra \"Última parada: comando do usuário\"");

    // ------------------------------------------------------------------
    titulo("3. Giro e um pulso curto");
    await pagina.click("#btn-esquerda");
    t = await ate(porta, (x) => x.estado === "GIRO_ESQ", 1500);
    verificar(t.estado === "GIRO_ESQ", "tocar Esquerda gira a cadeira");
    t = await ate(porta, (x) => x.estado === "PARADA", 2000);
    verificar(t.estado === "PARADA" && t.motivo === "FIM_DO_GIRO", "o giro termina sozinho (fim do giro)");

    // ------------------------------------------------------------------
    titulo("4. Obstaculo");
    await pedir(porta, "/sim/obstaculo?v=1");
    await pagina.waitForSelector("#alerta-obstaculo", { state: "visible", timeout: 3000 });
    verificar(true, "o app mostra a faixa de obstaculo");
    soltar = await segurar(pagina, "#btn-frente");
    await pagina.waitForFunction(() => /recusad/i.test(document.getElementById("toast-texto").textContent), null, { timeout: 3000 }).catch(() => {});
    verificar(/recusad/i.test(await texto(pagina, "#toast-texto")), "Frente com obstaculo: o app avisa que foi recusado");
    t = await cadeira(porta);
    verificar(t.estado === "PARADA" && t.pwmEsq === 0, "e a cadeira continua parada");
    await soltar();
    const sob_o_dedo = await pagina.evaluate(() => {
      const b = document.getElementById("btn-re").getBoundingClientRect();
      const e = document.elementFromPoint(b.left + b.width / 2, b.top + b.height / 2);
      return e && e.closest("button") ? e.closest("button").id : null;
    });
    verificar(sob_o_dedo === "btn-re", "com o alerta na tela, a Re fica a vista, sem ficar atras do PARAR");
    soltar = await segurar(pagina, "#btn-re");
    t = await ate(porta, (x) => x.estado === "RE", 2000);
    verificar(t.estado === "RE", "Re continua funcionando com obstaculo a frente");
    await soltar();
    await ate(porta, (x) => x.estado === "PARADA", 1500);
    await pedir(porta, "/sim/obstaculo?v=0");
    await pagina.waitForSelector("#alerta-obstaculo", { state: "hidden", timeout: 3000 });

    // ------------------------------------------------------------------
    titulo("5. Botao de emergencia");
    soltar = await segurar(pagina, "#btn-frente");
    await ate(porta, (x) => x.estado === "FRENTE", 2000);
    await pedir(porta, "/sim/emergencia?v=1");
    t = await ate(porta, (x) => x.emergencia === 1, 2000);
    verificar(t.emergencia === 1 && t.estado === "PARADA" && t.pwmEsq === 0 && t.pwmDir === 0,
      "o botao para a cadeira na hora (PWM 0)");
    await pagina.waitForSelector("#alerta-emergencia", { state: "visible", timeout: 3000 });
    verificar(true, "o app mostra a faixa EMERGÊNCIA");
    await soltar();
    // Um comando que chega ao Mega durante a emergencia, direto pela API.
    await pedir(porta, "/api/comando", "c=frente");
    await esperar(300);
    await pedir(porta, "/sim/emergencia?v=0");
    await esperar(1500);
    t = await cadeira(porta);
    verificar(t.emergencia === 0 && t.estado === "PARADA" && t.motivo === "EMERGENCIA",
      "soltou o botao: nada que chegou durante a emergencia e executado");

    // ------------------------------------------------------------------
    titulo("6. Wi-Fi cai com a cadeira andando");
    soltar = await segurar(pagina, "#btn-frente");
    await ate(porta, (x) => x.estado === "FRENTE", 2000);
    await pagina.route("**/api/comando", (r) => r.abort());
    const queda = Date.now();
    t = await ate(porta, (x) => x.estado === "PARADA", 5000);
    const segundos = ((Date.now() - queda) / 1000).toFixed(1);
    verificar(t.estado === "PARADA" && t.motivo === "TEMPO_LIMITE" && Date.now() - queda < 4200,
      `sem nenhum comando chegando, o homem-morto para a cadeira sozinho (${segundos} s)`);
    await soltar();
    await pagina.unroute("**/api/comando");
    await esperar(500);

    // ------------------------------------------------------------------
    titulo("7. A placa do Mega para de responder");
    await pedir(porta, "/sim/mega_desligado?v=1");
    await pagina.waitForFunction(() => document.getElementById("aviso-titulo").textContent.includes("Wi-Fi ok, mas a placa não responde"), null, { timeout: 5000 }).catch(() => {});
    verificar((await texto(pagina, "#aviso-titulo")).includes("Wi-Fi ok, mas a placa não responde"),
      "o app avisa: Wi-Fi ok, mas a placa nao responde");
    verificar(await pagina.isDisabled("#btn-frente"), "sem a placa, os comandos de movimento ficam desabilitados");
    await pedir(porta, "/sim/mega_desligado?v=0");
    await pagina.waitForFunction(() => document.getElementById("pilula").textContent.includes("Conectado"), null, { timeout: 5000 });
    verificar(true, "a placa volta e o app reconecta sozinho");

    // ------------------------------------------------------------------
    titulo("8. O ESP8266 reinicia");
    await pedir(porta, "/sim/reiniciar_esp");
    await esperar(1500);
    await pagina.waitForFunction(() => document.getElementById("pilula").textContent.includes("Conectado"), null, { timeout: 8000 });
    verificar(true, "o app reconecta depois do reinicio do ESP");
    if (await pagina.getAttribute("#trava", "aria-checked") !== "true") await pagina.click("#trava");
    soltar = await segurar(pagina, "#btn-frente");
    t = await ate(porta, (x) => x.estado === "FRENTE", 3000);
    verificar(t.estado === "FRENTE", "depois do lixo de boot do ESP, o primeiro comando chega ao Mega");
    await soltar();
    t = await ate(porta, (x) => x.estado === "PARADA", 1500);
    verificar(t.estado === "PARADA", "e soltar para a cadeira");

    // ------------------------------------------------------------------
    titulo("9. Console do navegador");
    verificar(erros.length === 0, erros.length ? "erros: " + erros.join(" | ") : "nenhum erro de JavaScript");
  } finally {
    await contexto.close();
    sim.kill();
  }
}

async function soAcompanha(navegador) {
  titulo("10. Firmware em modo so acompanhamento (PERMITIR_COMANDO_APP 0)");
  const porta = await portaLivre();
  const sim = await ligarSimulador(porta, ["--mega", path.join(BUILD, "mega_sim_so_acompanha"),
                                           "--esp", path.join(BUILD, "esp_sim")]);
  const { contexto, pagina } = await abrirApp(navegador, porta);
  try {
    await pagina.waitForFunction(() => document.getElementById("pilula").textContent.includes("Conectado"), null, { timeout: 8000 });
    await pagina.waitForSelector("#nota-app", { state: "visible", timeout: 3000 });
    verificar(await pagina.isDisabled("#trava"), "o interruptor do controle fica desabilitado, com a nota explicando");
    await pedir(porta, "/api/comando", "c=tras");
    await esperar(300);
    let t = await cadeira(porta);
    verificar(t.estado === "PARADA", "movimento pedido direto pela API e recusado pelo Mega");
    await pagina.locator("#btn-parar").dispatchEvent("pointerdown");
    await esperar(800);
    const e = await pedir(porta, "/api/estado?desde=0");
    const parar = (e.eventos || []).some((x) => x.dados.cmd === "PARAR" && x.dados.origem === "app" && x.dados.aceito === 1);
    verificar(parar, "PARAR do app continua sendo aceito");
  } finally {
    await contexto.close();
    sim.kill();
  }
}

(async () => {
  console.log("Teste de ponta a ponta: app + firmwares reais no simulador");
  const navegador = await chromium.launch();
  try {
    await principal(navegador);
    await soAcompanha(navegador);
  } catch (e) {
    falhas++;
    console.log("  [FALHA] " + (e && e.stack ? e.stack : e));
  } finally {
    await navegador.close();
  }
  console.log("\n----------------------------------------");
  console.log(`Total: ${total}   Falhas: ${falhas}`);
  if (falhas === 0) console.log("Todos os testes de ponta a ponta passaram.");
  process.exit(falhas === 0 ? 0 : 1);
})();
