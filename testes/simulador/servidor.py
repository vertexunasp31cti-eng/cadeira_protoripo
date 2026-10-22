#!/usr/bin/env python3
"""
servidor.py - Simulador da cadeira no PC: o app conversando com o codigo real.

Roda os dois firmwares compilados para o PC (mega_sim e esp_sim), liga a
serial de um na do outro, como as chaves DIP 1 e 2 fazem na placa, e atende
o navegador em http://localhost:8000 repassando cada requisicao para o
esp_sim. Assim o app, as rotas do ESP8266 e a maquina de estados do Mega sao
os mesmos que vao para a placa; so o hardware e simulado.

Uso:

    python3 testes/simulador/servidor.py            # http://localhost:8000
    python3 testes/simulador/servidor.py --porta 8080
    python3 testes/simulador/servidor.py --rede     # aceita o celular na mesma rede

Os binarios sao compilados com "make -C testes simulador" sempre que algo
mudou (precisa de g++ e make). So biblioteca padrao do Python.

Controles da simulacao (no navegador, ou com curl):

    /sim/obstaculo?v=1       sensor ve um obstaculo (v=0 libera)
    /sim/emergencia?v=1      botao de emergencia apertado (v=0 solta)
    /sim/mega_desligado?v=1  a placa do Mega para de responder (v=0 volta)
    /sim/reiniciar_esp       o ESP8266 reinicia, com o lixo de boot na serial
    /sim/                    estado da simulacao
"""

import argparse
import itertools
import json
import os
import random
import subprocess
import sys
import threading
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from urllib.parse import parse_qs, urlsplit

AQUI = os.path.dirname(os.path.abspath(__file__))
TESTES = os.path.dirname(AQUI)
MEGA_PADRAO = os.path.join(TESTES, "build", "mega_sim")
ESP_PADRAO = os.path.join(TESTES, "build", "esp_sim")

# O ESP8266 imprime a mensagem de boot a 74880 bps; lida a 115200 pela
# Serial3 do Mega, vira bytes sem sentido e sem nenhum '\n'.
TAMANHO_LIXO_BOOT = 216

TEMPO_RESPOSTA_S = 5


class Placa:
    """Um firmware rodando como processo filho, falando por linhas."""

    def __init__(self, caminho, nome, ao_receber, depuracao):
        self.caminho = caminho
        self.nome = nome
        self.ao_receber = ao_receber
        self.depuracao = depuracao
        self.processo = None
        self.trava_escrita = threading.Lock()

    def ligar(self):
        self.processo = subprocess.Popen(
            [self.caminho],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=None if self.depuracao else subprocess.DEVNULL,
        )
        threading.Thread(target=self._ler, args=(self.processo,),
                         daemon=True).start()

    def desligar(self):
        p = self.processo
        self.processo = None
        if p is not None:
            try:
                p.stdin.close()
            except OSError:
                pass
            p.terminate()
            try:
                p.wait(timeout=2)
            except subprocess.TimeoutExpired:
                p.kill()

    def enviar(self, linha):
        p = self.processo
        if p is None:
            return
        with self.trava_escrita:
            try:
                p.stdin.write(linha.encode("ascii") + b"\n")
                p.stdin.flush()
            except (BrokenPipeError, ValueError, OSError):
                pass

    def _ler(self, processo):
        for bruto in processo.stdout:
            self.ao_receber(bruto.decode("ascii", "replace").rstrip("\n"))
        if self.processo is processo:
            print("simulador: o %s parou de rodar" % self.nome, file=sys.stderr)


class Simulacao:
    def __init__(self, mega, esp, depuracao):
        self.mega_desligado = False
        self.obstaculo = False
        self.emergencia = False
        self.ids = itertools.count(1)
        self.pendentes = {}
        self.trava = threading.Lock()
        self.mega = Placa(mega, "mega_sim", self._do_mega, depuracao)
        self.esp = Placa(esp, "esp_sim", self._do_esp, depuracao)

    def ligar(self):
        self.mega.ligar()
        self.esp.ligar()

    def desligar(self):
        self.esp.desligar()
        self.mega.desligar()

    # --- Fio serial entre as placas -----------------------------------------
    def _do_mega(self, linha):
        if linha.startswith("S ") and not self.mega_desligado:
            self.esp.enviar(linha)

    def _do_esp(self, linha):
        if linha.startswith("S "):
            if not self.mega_desligado:
                self.mega.enviar(linha)
        elif linha.startswith("R "):
            partes = linha.split(" ")
            with self.trava:
                espera = self.pendentes.get(partes[1])
            if espera is not None:
                espera["resposta"] = partes[2:]
                espera["pronta"].set()

    # --- HTTP para o esp_sim ---------------------------------------------------
    def pedir(self, metodo, caminho, corpo):
        ident = str(next(self.ids))
        espera = {"pronta": threading.Event(), "resposta": None}
        with self.trava:
            self.pendentes[ident] = espera
        self.esp.enviar("H %s %s %s %s" % (
            ident, metodo, caminho.encode("utf-8").hex() or "-",
            corpo.hex() or "-"))
        ok = espera["pronta"].wait(TEMPO_RESPOSTA_S)
        with self.trava:
            self.pendentes.pop(ident, None)
        if not ok:
            return None
        status, tipo, cabecalhos, corpo = espera["resposta"]

        def de_hex(h):
            return b"" if h == "-" else bytes.fromhex(h)

        lista = []
        for c in de_hex(cabecalhos).decode("utf-8").splitlines():
            nome, _, valor = c.partition(": ")
            lista.append((nome, valor))
        return int(status), de_hex(tipo).decode("utf-8"), lista, de_hex(corpo)

    # --- Controles da simulacao -------------------------------------------------
    def controlar(self, acao, valor):
        if acao == "obstaculo":
            self.obstaculo = bool(valor)
            self.mega.enviar("O %d" % valor)
        elif acao == "emergencia":
            self.emergencia = bool(valor)
            self.mega.enviar("E %d" % valor)
        elif acao == "mega_desligado":
            self.mega_desligado = bool(valor)
        elif acao == "reiniciar_esp":
            self.esp.desligar()
            if not self.mega_desligado:
                lixo = bytes(random.randrange(0x80, 0x100)
                             for _ in range(TAMANHO_LIXO_BOOT))
                self.mega.enviar("S " + lixo.hex())
            self.esp.ligar()
        else:
            return False
        return True

    def estado(self):
        return {"obstaculo": self.obstaculo, "emergencia": self.emergencia,
                "mega_desligado": self.mega_desligado}


def criar_atendente(sim):
    class Atendente(BaseHTTPRequestHandler):
        protocol_version = "HTTP/1.1"

        def log_message(self, formato, *args):
            pass  # o polling do app imprimiria uma linha a cada 300 ms

        def _responder(self, status, tipo, cabecalhos, corpo):
            self.send_response(status)
            if tipo:
                self.send_header("Content-Type", tipo)
            for nome, valor in cabecalhos:
                if nome.lower() not in ("content-length", "content-type"):
                    self.send_header(nome, valor)
            self.send_header("Content-Length", str(len(corpo)))
            self.end_headers()
            if self.command != "HEAD":
                self.wfile.write(corpo)

        def _simulacao(self, partes):
            acao = partes.path[len("/sim/"):].strip("/")
            v = parse_qs(partes.query).get("v", ["1"])[0]
            if acao and not sim.controlar(acao, 1 if v == "1" else 0):
                self._responder(404, "application/json", [],
                                b'{"ok":false,"erro":"controle_invalido"}')
                return
            corpo = json.dumps(dict(ok=True, **sim.estado())).encode("utf-8")
            self._responder(200, "application/json",
                            [("Cache-Control", "no-store")], corpo)

        def _repassar(self):
            partes = urlsplit(self.path)
            if partes.path.startswith("/sim/") or partes.path == "/sim":
                self._simulacao(partes)
                return
            tamanho = int(self.headers.get("Content-Length") or 0)
            corpo = self.rfile.read(tamanho) if tamanho > 0 else b""
            resposta = sim.pedir(self.command, self.path, corpo)
            if resposta is None:
                self._responder(504, "text/plain; charset=utf-8", [],
                                "o esp_sim nao respondeu".encode("utf-8"))
                return
            self._responder(*resposta)

        do_GET = do_POST = do_HEAD = do_OPTIONS = _repassar

    return Atendente


def compilar():
    try:
        r = subprocess.run(["make", "-s", "-C", TESTES, "simulador"])
    except FileNotFoundError:
        return False
    return r.returncode == 0


def main(argv=None):
    p = argparse.ArgumentParser(
        description="Simulador da cadeira: app + firmwares reais no PC.")
    p.add_argument("--porta", type=int, default=8000)
    p.add_argument("--rede", action="store_true",
                   help="atende tambem outros aparelhos da rede (celular)")
    p.add_argument("--mega", default=MEGA_PADRAO,
                   help="binario do Mega (padrao: testes/build/mega_sim)")
    p.add_argument("--esp", default=ESP_PADRAO,
                   help="binario do ESP (padrao: testes/build/esp_sim)")
    p.add_argument("--depuracao", action="store_true",
                   help="mostra a Serial de depuracao do Mega")
    args = p.parse_args(argv)

    # O make so recompila o que mudou; sem g++ ou make, usa o que ja existe.
    if not compilar():
        print("simulador: nao consegui rodar \"make -C testes simulador\" "
              "(precisa de g++ e make).", file=sys.stderr)
    for caminho in (args.mega, args.esp):
        if not os.path.isfile(caminho):
            print("simulador: binario nao encontrado: %s" % caminho,
                  file=sys.stderr)
            return 1

    sim = Simulacao(args.mega, args.esp, args.depuracao)
    sim.ligar()
    endereco = "0.0.0.0" if args.rede else "127.0.0.1"
    try:
        servidor = ThreadingHTTPServer((endereco, args.porta),
                                       criar_atendente(sim))
    except OSError as e:
        sim.desligar()
        print("simulador: porta %d indisponivel (%s)" % (args.porta, e),
              file=sys.stderr)
        return 1
    servidor.daemon_threads = True
    print("Simulador da cadeira em http://localhost:%d  (Ctrl+C para sair)"
          % args.porta, flush=True)
    try:
        servidor.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        servidor.server_close()
        sim.desligar()
    return 0


if __name__ == "__main__":
    sys.exit(main())
