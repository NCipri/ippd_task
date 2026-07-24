import struct
import sys
import zlib

FONTE = {
    "A": [0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11],
    "B": [0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E],
    "C": [0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E],
    "D": [0x1E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1E],
    "E": [0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F],
    "F": [0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10],
    "G": [0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0F],
    "H": [0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11],
    "I": [0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E],
    "J": [0x07, 0x02, 0x02, 0x02, 0x02, 0x12, 0x0C],
    "K": [0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11],
    "L": [0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F],
    "M": [0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11],
    "N": [0x11, 0x11, 0x19, 0x15, 0x13, 0x11, 0x11],
    "O": [0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E],
    "P": [0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10],
    "Q": [0x0E, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0D],
    "R": [0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11],
    "S": [0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E],
    "T": [0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04],
    "U": [0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E],
    "V": [0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04],
    "W": [0x11, 0x11, 0x11, 0x15, 0x15, 0x1B, 0x11],
    "X": [0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11],
    "Y": [0x11, 0x11, 0x0A, 0x04, 0x04, 0x04, 0x04],
    "Z": [0x1F, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1F],
    "0": [0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E],
    "1": [0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E],
    "2": [0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F],
    "3": [0x1F, 0x02, 0x04, 0x02, 0x01, 0x11, 0x0E],
    "4": [0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02],
    "5": [0x1F, 0x10, 0x1E, 0x01, 0x01, 0x11, 0x0E],
    "6": [0x06, 0x08, 0x10, 0x1E, 0x11, 0x11, 0x0E],
    "7": [0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08],
    "8": [0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E],
    "9": [0x0E, 0x11, 0x11, 0x0F, 0x01, 0x02, 0x0C],
    " ": [0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00],
    ".": [0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C],
    ",": [0x00, 0x00, 0x00, 0x00, 0x0C, 0x04, 0x08],
    ":": [0x00, 0x0C, 0x0C, 0x00, 0x0C, 0x0C, 0x00],
    "-": [0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00],
    "+": [0x00, 0x04, 0x04, 0x1F, 0x04, 0x04, 0x00],
    "=": [0x00, 0x00, 0x1F, 0x00, 0x1F, 0x00, 0x00],
    "/": [0x01, 0x02, 0x02, 0x04, 0x08, 0x08, 0x10],
    "(": [0x02, 0x04, 0x08, 0x08, 0x08, 0x04, 0x02],
    ")": [0x08, 0x04, 0x02, 0x02, 0x02, 0x04, 0x08],
    "%": [0x19, 0x1A, 0x02, 0x04, 0x08, 0x0B, 0x13],
}

PALETA = [
    (0.00, (0, 0, 4)),
    (0.15, (40, 11, 84)),
    (0.30, (101, 21, 110)),
    (0.45, (159, 42, 99)),
    (0.60, (212, 72, 66)),
    (0.75, (245, 125, 21)),
    (0.90, (250, 193, 39)),
    (1.00, (252, 255, 164)),
]

FUNDO = (18, 18, 20)
TEXTO = (235, 235, 235)


def cor(t):
    """Mapeia t em [0,1] para uma cor RGB interpolada na paleta."""
    if t <= 0.0:
        return PALETA[0][1]
    if t >= 1.0:
        return PALETA[-1][1]
    for i in range(len(PALETA) - 1):
        p0, c0 = PALETA[i]
        p1, c1 = PALETA[i + 1]
        if p0 <= t <= p1:
            f = (t - p0) / (p1 - p0)
            return tuple(int(c0[k] + f * (c1[k] - c0[k])) for k in range(3))
    return PALETA[-1][1]


def ler_campo(caminho):
    """Le um arquivo de saida e devolve (nx, ny, lista de temperaturas)."""
    valores = []
    with open(caminho) as f:
        for linha in f:
            if linha.startswith("#"):
                continue
            valores.extend(float(x) for x in linha.split())
    if len(valores) < 2:
        raise ValueError(f"{caminho}: arquivo vazio ou sem cabecalho")
    nx, ny = int(valores[0]), int(valores[1])
    campo = valores[2:]
    if len(campo) != nx * ny:
        raise ValueError(f"{caminho}: esperava {nx*ny} valores, achei {len(campo)}")
    return nx, ny, campo


def reduzir(nx, ny, campo, lado_max):
    """Reduz a malha por amostragem para a imagem nao ficar gigante."""
    fator = 1
    while (nx + fator - 1) // fator > lado_max or (ny + fator - 1) // fator > lado_max:
        fator += 1
    if fator == 1:
        return nx, ny, campo
    novo_nx = (nx + fator - 1) // fator
    novo_ny = (ny + fator - 1) // fator
    novo = [campo[(j * fator) * nx + (i * fator)]
            for j in range(novo_ny) for i in range(novo_nx)]
    return novo_nx, novo_ny, novo


def nova_tela(largura, altura, fundo=FUNDO):
    return [bytearray(bytes(fundo) * largura) for _ in range(altura)]


def ponto(tela, x, y, c):
    if 0 <= y < len(tela) and 0 <= x * 3 < len(tela[0]):
        tela[y][x * 3:x * 3 + 3] = bytes(c)


def texto(tela, x, y, s, c=TEXTO, escala=1):
    """Escreve s na posicao (x, y). Retorna a largura ocupada em pixels."""
    cx = x
    for ch in s.upper():
        glifo = FONTE.get(ch, FONTE[" "])
        for linha in range(7):
            for coluna in range(5):
                if glifo[linha] & (1 << (4 - coluna)):
                    for dy in range(escala):
                        for dx in range(escala):
                            ponto(tela, cx + coluna * escala + dx, y + linha * escala + dy, c)
        cx += 6 * escala
    return cx - x


def largura_texto(s, escala=1):
    return len(s) * 6 * escala


def desenhar_campo(tela, x0, y0, nx, ny, campo, vmin, vmax, escala):
    """Desenha a malha como mapa de calor no canto (x0, y0)."""
    faixa = vmax - vmin
    for j in range(ny):
        for i in range(nx):
            t = 0.0 if faixa <= 0 else (campo[j * nx + i] - vmin) / faixa
            c = cor(t)
            for dy in range(escala):
                linha = tela[y0 + j * escala + dy]
                base = (x0 + i * escala) * 3
                linha[base:base + 3 * escala] = bytes(c) * escala


def desenhar_barra(tela, x0, y0, largura, altura, vmin, vmax, unidade):
    """Barra de cores com os rotulos de minimo e maximo."""
    for i in range(largura):
        c = cor(i / max(1, largura - 1))
        for j in range(altura):
            ponto(tela, x0 + i, y0 + j, c)
    rot_min = f"{vmin:.2f} {unidade}"
    rot_max = f"{vmax:.2f} {unidade}"
    texto(tela, x0, y0 + altura + 4, rot_min, TEXTO, 1)
    texto(tela, x0 + largura - largura_texto(rot_max, 1), y0 + altura + 4, rot_max, TEXTO, 1)


def escrever_png(caminho, tela):
    altura = len(tela)
    largura = len(tela[0]) // 3
    bruto = b"".join(b"\x00" + bytes(linha) for linha in tela)

    def bloco(tipo, dados):
        return (struct.pack(">I", len(dados)) + tipo + dados
                + struct.pack(">I", zlib.crc32(tipo + dados) & 0xFFFFFFFF))

    png = b"\x89PNG\r\n\x1a\n"
    png += bloco(b"IHDR", struct.pack(">IIBBBBB", largura, altura, 8, 2, 0, 0, 0))
    png += bloco(b"IDAT", zlib.compress(bruto, 9))
    png += bloco(b"IEND", b"")
    with open(caminho, "wb") as f:
        f.write(png)
    return largura, altura


MARGEM = 24
ESPACO = 28
ALT_TITULO = 16
ALT_BARRA = 12
ALT_ROTULO = 12


def montar(paineis, caminho, lado_max=480):
    """paineis: lista de (titulo, subtitulo, nx, ny, campo, unidade)."""
    preparados = []
    for titulo, sub, nx, ny, campo, unidade in paineis:
        nx, ny, campo = reduzir(nx, ny, campo, lado_max)
        escala = max(1, lado_max // max(nx, ny))
        preparados.append((titulo, sub, nx, ny, campo, unidade, escala))

    larg_paineis = [max(p[2] * p[6], largura_texto(p[0], 2), largura_texto(p[1], 1))
                    for p in preparados]
    alt_imagem = max(p[3] * p[6] for p in preparados)

    largura = MARGEM * 2 + sum(larg_paineis) + ESPACO * (len(preparados) - 1)
    altura = (MARGEM * 2 + ALT_TITULO + ALT_ROTULO + alt_imagem + 10
              + ALT_BARRA + 4 + 8 + 6)
    tela = nova_tela(largura, altura)

    x = MARGEM
    for (titulo, sub, nx, ny, campo, unidade, escala), larg in zip(preparados, larg_paineis):
        vmin, vmax = min(campo), max(campo)
        y = MARGEM
        texto(tela, x, y, titulo, TEXTO, 2)
        y += ALT_TITULO
        texto(tela, x, y, sub, (150, 150, 155), 1)
        y += ALT_ROTULO
        desenhar_campo(tela, x, y, nx, ny, campo, vmin, vmax, escala)
        y += alt_imagem + 10
        desenhar_barra(tela, x, y, nx * escala, ALT_BARRA, vmin, vmax, unidade)
        x += larg + ESPACO

    return escrever_png(caminho, tela)


def main():
    args = [a for a in sys.argv[1:]]
    comparar = "--comparar" in args
    if comparar:
        args.remove("--comparar")

    if comparar and len(args) == 3:
        nx_a, ny_a, a = ler_campo(args[0])
        nx_b, ny_b, b = ler_campo(args[1])
        if (nx_a, ny_a) != (nx_b, ny_b):
            print(f"malhas diferentes: {nx_a}x{ny_a} vs {nx_b}x{ny_b}")
            return 1
        dif = [abs(x - y) for x, y in zip(a, b)]
        dif_max = max(dif)
        estado = "IDENTICAS" if dif_max == 0.0 else f"MAX {dif_max:.2e}"
        paineis = [
            ("SERIAL", f"{nx_a}X{ny_a} SEM MPI E SEM OPENMP", nx_a, ny_a, a, "C"),
            ("PARALELO", f"{nx_b}X{ny_b} COM MPI + OPENMP", nx_b, ny_b, b, "C"),
            ("DIFERENCA", f"ABS(SERIAL - PARALELO): {estado}", nx_a, ny_a, dif, "C"),
        ]
        larg, alt = montar(paineis, args[2])
        print(f"imagem gerada: {args[2]} ({larg}x{alt} px)")
        print(f"diferenca maxima: {dif_max:.3e} C")
        return 0

    if not comparar and len(args) == 2:
        nx, ny, campo = ler_campo(args[0])
        paineis = [("CAMPO FINAL", f"{nx}X{ny} CELULAS", nx, ny, campo, "C")]
        larg, alt = montar(paineis, args[1])
        print(f"imagem gerada: {args[1]} ({larg}x{alt} px)")
        return 0

    print(__doc__)
    return 1


if __name__ == "__main__":
    sys.exit(main())
