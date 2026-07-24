import sys

from visualizar import (FUNDO, TEXTO, cor, escrever_png, largura_texto,
                        nova_tela, ponto, texto)

MARGEM = 24
ALT_BARRA = 22
ESPACO_BARRA = 10
LARG_ROTULO = 150
LARG_VALOR = 170
LARG_GRAFICO = 420

COR_SERIAL = (110, 110, 120)


def ler_tabela(caminho):
    """Devolve (tempo_serial, [(processos, threads, tempo, speedup, eficiencia)], cabecalho)."""
    serial = None
    linhas = []
    cabecalho = []
    with open(caminho) as f:
        for linha in f:
            linha = linha.strip()
            if not linha:
                continue
            if linha.startswith("#"):
                texto_cab = linha.lstrip("# ").strip()
                if not texto_cab.startswith(("processos", "tempo do serial")):
                    cabecalho.append(texto_cab)
                continue
            campos = linha.split()
            if campos[0] == "serial":
                serial = float(campos[1])
            elif len(campos) >= 5:
                linhas.append((int(campos[0]), int(campos[1]), float(campos[2]),
                               float(campos[3]), float(campos[4])))
    if serial is None:
        raise ValueError(f"{caminho}: nao achei a linha 'serial <tempo>'")
    return serial, linhas, cabecalho


def barra(tela, x, y, largura, altura, c):
    for j in range(altura):
        for i in range(largura):
            ponto(tela, x + i, y + j, c)


def main():
    if len(sys.argv) != 3:
        print(__doc__)
        return 1

    serial, linhas, cabecalho = ler_tabela(sys.argv[1])
    if not linhas:
        print("erro: tabela sem configuracoes paralelas")
        return 1

    barras = [("SERIAL", serial, 1.0, COR_SERIAL)]
    for np_, th, t, sp, _ in linhas:
        barras.append((f"MPI {np_} X OMP {th}", t, sp, None))

    t_max = max(b[1] for b in barras)
    sp_max = max(b[2] for b in barras)

    largura = MARGEM * 2 + LARG_ROTULO + LARG_GRAFICO + LARG_VALOR
    altura = (MARGEM * 2 + 20 + len(cabecalho) * 10 + 14
              + len(barras) * (ALT_BARRA + ESPACO_BARRA) + 6)
    tela = nova_tela(largura, altura)

    y = MARGEM
    texto(tela, MARGEM, y, "TEMPO DE EXECUCAO: SERIAL X MPI + OPENMP", TEXTO, 2)
    y += 22
    for linha in cabecalho:
        texto(tela, MARGEM, y, linha[:96], (140, 140, 148), 1)
        y += 10
    y += 8

    x_barra = MARGEM + LARG_ROTULO
    for rotulo, tempo, speedup, cor_fixa in barras:
        texto(tela, MARGEM, y + (ALT_BARRA - 7) // 2, rotulo, TEXTO, 1)

        comprimento = max(2, int(LARG_GRAFICO * tempo / t_max))
        c = cor_fixa if cor_fixa else cor(0.25 + 0.7 * (speedup / sp_max))
        barra(tela, x_barra, y, comprimento, ALT_BARRA, c)

        valor = f"{tempo:.3f} S"
        if cor_fixa is None:
            valor += f"  ({speedup:.2f}X)"
        texto(tela, x_barra + LARG_GRAFICO + 12, y + (ALT_BARRA - 7) // 2, valor, TEXTO, 1)

        y += ALT_BARRA + ESPACO_BARRA

    x_ref = x_barra + int(LARG_GRAFICO * serial / t_max)
    for j in range(MARGEM + 20 + len(cabecalho) * 10 + 14, y - ESPACO_BARRA):
        if (j // 3) % 2 == 0:
            ponto(tela, x_ref, j, (200, 200, 210))

    larg, alt = escrever_png(sys.argv[2], tela)
    print(f"grafico gerado: {sys.argv[2]} ({larg}x{alt} px)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
