#!/usr/bin/env python3
"""
gerar_pagina.py - Embute o app (app/index.html) no firmware do ESP8266.

O ESP8266 serve a pagina direto da flash. Este script copia o HTML para
firmware/esp_wifi/pagina.h, como texto PROGMEM. Rode sempre que mudar o app e
grave o ESP de novo:

    python3 ferramentas/gerar_pagina.py

Com --verificar nao escreve nada: so confere se o pagina.h esta igual ao que
seria gerado agora (o "make -C testes" usa isso para pegar app novo com
pagina.h velho).

Opcoes --entrada e --saida trocam os arquivos padrao. So biblioteca padrao do
Python, sem nada para instalar.
"""

import argparse
import os
import sys

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ENTRADA_PADRAO = os.path.join(RAIZ, "app", "index.html")
SAIDA_PADRAO = os.path.join(RAIZ, "firmware", "esp_wifi", "pagina.h")

# O HTML vai dentro de um literal bruto do C++, R"PAGINA_HTML(...)PAGINA_HTML".
# Se o proprio HTML tiver o fechamento, o literal terminaria no meio dele.
FECHAMENTO = b')PAGINA_HTML"'

CABECALHO = """\
// ARQUIVO GERADO, nao edite; edite app/index.html e rode python3 ferramentas/gerar_pagina.py
//
// App da cadeira embutido no firmware do ESP8266. A pagina fica na flash
// (PROGMEM) e e enviada inteira em GET /.
// Tamanho: {tamanho} bytes.
#ifndef PAGINA_H
#define PAGINA_H

#include <Arduino.h>

static const char PAGINA_HTML[] PROGMEM = R"PAGINA_HTML("""

RODAPE = """)PAGINA_HTML";

#endif  // PAGINA_H
"""

COMANDO = "python3 ferramentas/gerar_pagina.py"


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


def gerar(caminho_html):
    """Le o HTML e devolve o conteudo do pagina.h, em bytes."""
    if not os.path.isfile(caminho_html):
        raise ErroEntrada(
            "arquivo do app nao encontrado: %s" % caminho_legivel(caminho_html))
    with open(caminho_html, "rb") as f:
        html = f.read()

    # Fim de linha sempre "\n": o compilador troca "\r\n" por "\n" dentro do
    # literal, e o tamanho no cabecalho tem que ser o que vai para a flash.
    html = html.replace(b"\r\n", b"\n").replace(b"\r", b"\n")

    if not html.strip():
        raise ErroEntrada(
            "arquivo do app esta vazio: %s" % caminho_legivel(caminho_html))
    if FECHAMENTO in html:
        raise ErroEntrada(
            "%s contem o texto %s, que fecha o literal do pagina.h antes da "
            "hora. Troque esse trecho no HTML."
            % (caminho_legivel(caminho_html), FECHAMENTO.decode("ascii")))
    if b"\0" in html:
        # O ESP envia a pagina ate o primeiro byte zero.
        raise ErroEntrada(
            "%s contem um byte zero; a pagina seria cortada nele."
            % caminho_legivel(caminho_html))
    try:
        html.decode("utf-8")
    except UnicodeDecodeError as e:
        raise ErroEntrada(
            "%s nao esta em UTF-8 (byte %d). Salve o arquivo como UTF-8."
            % (caminho_legivel(caminho_html), e.start))

    # Sem data nem caminho absoluto: gerar duas vezes da o mesmo arquivo, e o
    # --verificar so acusa diferenca quando o app mudou de verdade.
    return (CABECALHO.format(tamanho=len(html)).encode("ascii") + html +
            RODAPE.encode("ascii"))


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
        novo = gerar(args.entrada)
    except ErroEntrada as e:
        print("gerar_pagina: erro: %s" % e, file=sys.stderr)
        return 2

    saida = caminho_legivel(args.saida)
    atual = None
    if os.path.isfile(args.saida):
        with open(args.saida, "rb") as f:
            # O Git no Windows (core.autocrlf) entrega o pagina.h com "\r\n".
            # O conteudo e o mesmo, entao nao conta como desatualizado.
            atual = f.read().replace(b"\r\n", b"\n")

    if args.verificar:
        if atual == novo:
            print("%s esta atualizado." % saida)
            return 0
        if atual is None:
            motivo = "nao existe"
        else:
            motivo = ("esta desatualizado em relacao a %s"
                      % caminho_legivel(args.entrada))
        print("gerar_pagina: %s %s: rode %s" % (saida, motivo, COMANDO),
              file=sys.stderr)
        return 1

    if atual == novo:
        print("%s ja estava atualizado." % saida)
        return 0
    pasta = os.path.dirname(os.path.abspath(args.saida))
    os.makedirs(pasta, exist_ok=True)
    with open(args.saida, "wb") as f:
        f.write(novo)
    print("%s gerado a partir de %s."
          % (saida, caminho_legivel(args.entrada)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
