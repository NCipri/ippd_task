import sys

def ler(caminho):
    valores = []
    with open(caminho) as f:
        for linha in f:
            if linha.startswith("#"):
                continue
            valores.extend(float(x) for x in linha.split())
    nx, ny = int(valores[0]), int(valores[1])
    return nx, ny, valores[2:]


def main():
    if len(sys.argv) != 3:
        print(__doc__)
        return 1

    nx_a, ny_a, a = ler(sys.argv[1])
    nx_b, ny_b, b = ler(sys.argv[2])

    if (nx_a, ny_a) != (nx_b, ny_b) or len(a) != len(b):
        print(f"malhas diferentes: {nx_a}x{ny_a} vs {nx_b}x{ny_b}")
        return 1

    dif_max = 0.0
    pos = 0
    for i, (x, y) in enumerate(zip(a, b)):
        d = abs(x - y)
        if d > dif_max:
            dif_max, pos = d, i

    print(f"malha            : {nx_a} x {ny_a}")
    print(f"diferenca maxima : {dif_max:.3e} (celula {pos % nx_a}, {pos // nx_a})")
    print("resultado        : " + ("OK, as versoes concordam" if dif_max < 1e-6
                                   else "DIVERGIRAM, investigar"))
    return 0 if dif_max < 1e-6 else 1


if __name__ == "__main__":
    sys.exit(main())
