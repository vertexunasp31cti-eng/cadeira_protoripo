#!/usr/bin/env python3
"""
gerar_pagina.py - Embute o app (app/index.html) no firmware do ESP8266.

O ESP8266 serve a pagina direto da flash. Este script compacta o HTML com
gzip e grava os bytes em firmware/esp_wifi/pagina.h, como um vetor PROGMEM.
Rode sempre que mudar o app e grave o ESP de novo:

    python3 ferramentas/gerar_pagina.py

Por que gzip: o app tem uns 100 KB e compactado fica perto de 28 KB. O
celular abre mais rapido e o ESP passa menos tempo preso enviando a pagina.
Todo navegador descompacta sozinho (cabecalho Content-Encoding: gzip).

Com --verificar nao escreve nada: so confere se o pagina.h, descompactado, e
igual ao app/index.html (o "make -C testes" usa isso para pegar app novo com
pagina.h velho). A comparacao e do conteudo, e nao dos bytes compactados,
porque versoes diferentes da zlib podem compactar o mesmo texto de jeitos
diferentes.

Opcoes --entrada e --saida trocam os arquivos padrao. So biblioteca padrao do
Python, sem nada para instalar.
"""

import argparse
import gzip
import os
import re
import sys
import zlib

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ENTRADA_PADRAO = os.path.join(RAIZ, "app", "index.html")
SAIDA_PADRAO = os.path.join(RAIZ, "firmware", "esp_wifi", "pagina.h")

CABECALHO = """\
// ARQUIVO GERADO, nao edite; edite app/index.html e rode python3 ferramentas/gerar_pagina.py
//
// App da cadeira embutido no firmware do ESP8266, compactado com gzip. Fica
// na flash (PROGMEM) e e enviado inteiro em GET / com o cabecalho
// Content-Encoding: gzip; o navegador descompacta sozinho.
// Tamanho: {original} bytes de HTML, {compactado} bytes compactados.
#ifndef PAGINA_H
#define PAGINA_H

#include <Arduino.h>

static const unsigned int PAGINA_HTML_GZ_TAM = {compactado};

static const uint8_t PAGINA_HTML_GZ[] PROGMEM = {{
"""

RODAPE = """\
};

#endif  // PAGINA_H
"""

BYTES_POR_LINHA = 16

COMANDO = "python3 ferramentas/gerar_pagina.py"

VETOR = re.compile(
    rb"PAGINA_HTML_GZ_TAM\s*=\s*(\d+)\s*;.*?"
    rb"PAGINA_HTML_GZ\[\]\s*PROGMEM\s*=\s*\{(.*?)\};",
    re.S)


class ErroEntrada(Exception):
    pass


def caminho_legivel(caminho):
    """Caminho relativo a raiz do repositorio, quando estiver dentro dele."""
    absoluto = os.path.abspath(caminho)
    try:
        relativo = os.path.relpath(absoluto, RAIZ)
    except ValueError:  # outro disco, no Windows
        return absoluto
    return absoluto if relativo.startswith("..") else relativo


def ler_html(caminho_html):
    """Le e confere o HTML do app. Devolve os bytes com fim de linha "\\n"."""
    if not os.path.isfile(caminho_html):
        raise ErroEntrada(
            "arquivo do app nao encontrado: %s" % caminho_legivel(caminho_html))
    with open(caminho_html, "rb") as f:
        html = f.read()

    # Fim de linha sempre "\n", para o Git no Windows (core.autocrlf) nao
    # mudar o conteudo embutido.
    html = html.replace(b"\r\n", b"\n").replace(b"\r", b"\n")

    if not html.strip():
        raise ErroEntrada(
            "arquivo do app esta vazio: %s" % caminho_legivel(caminho_html))
    try:
        html.decode("utf-8")
    except UnicodeDecodeError as e:
        raise ErroEntrada(
            "%s nao esta em UTF-8 (byte %d). Salve o arquivo como UTF-8."
            % (caminho_legivel(caminho_html), e.start))
    return html


def gerar(html):
    """Devolve o conteudo do pagina.h, em bytes."""
    # mtime=0: sem data no cabecalho do gzip, gerar duas vezes na mesma
    # maquina da o mesmo arquivo.
    compactado = gzip.compress(html, compresslevel=9, mtime=0)
    linhas = []
    for i in range(0, len(compactado), BYTES_POR_LINHA):
        trecho = compactado[i:i + BYTES_POR_LINHA]
        linhas.append("  " + ", ".join("0x%02x" % b for b in trecho) + ",")
    texto = (CABECALHO.format(original=len(html), compactado=len(compactado)) +
             "\n".join(linhas) + "\n" + RODAPE)
    return texto.encode("ascii")


def conteudo_embutido(pagina_h):
    """HTML guardado num pagina.h, ou None se o arquivo nao tiver o formato
    esperado (por exemplo, um pagina.h antigo ou editado a mao)."""
    m = VETOR.search(pagina_h)
    if not m:
        return None
    try:
        dados = bytes(int(x, 16) for x in re.findall(rb"0x([0-9a-fA-F]{2})",
                                                      m.group(2)))
        if len(dados) != int(m.group(1)):
            return None
        return gzip.decompress(dados)
    except (OSError, EOFError, ValueError, zlib.error):
        return None


def main(argv=None):
    p = argparse.ArgumentParser(
        description="Gera firmware/esp_wifi/pagina.h a partir de app/index.html.")
    p.add_argument("--entrada", default=ENTRADA_PADRAO,
                   help="HTML do app (padrao: app/index.html)")
    p.add_argument("--saida", default=SAIDA_PADRAO,
                   help="cabecalho gerado (padrao: firmware/esp_wifi/pagina.h)")
    p.add_argument("--verificar", action="store_true",
                   help="nao escreve; sai com 1 se o pagina.h estiver "
                        "desatualizado")
    args = p.parse_args(argv)

    try:
        html = ler_html(args.entrada)
    except ErroEntrada as e:
        print("gerar_pagina: erro: %s" % e, file=sys.stderr)
        return 2

    saida = caminho_legivel(args.saida)
    embutido = None
    if os.path.isfile(args.saida):
        with open(args.saida, "rb") as f:
            embutido = conteudo_embutido(f.read())

    if args.verificar:
        if embutido == html:
            print("%s esta atualizado." % saida)
            return 0
        if not os.path.isfile(args.saida):
            motivo = "nao existe"
        else:
            motivo = ("esta desatualizado em relacao a %s"
                      % caminho_legivel(args.entrada))
        print("gerar_pagina: %s %s: rode %s" % (saida, motivo, COMANDO),
              file=sys.stderr)
        return 1

    if embutido == html:
        print("%s ja estava atualizado." % saida)
        return 0
    pasta = os.path.dirname(os.path.abspath(args.saida))
    os.makedirs(pasta, exist_ok=True)
    with open(args.saida, "wb") as f:
        f.write(gerar(html))
    print("%s gerado a partir de %s."
          % (saida, caminho_legivel(args.entrada)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
