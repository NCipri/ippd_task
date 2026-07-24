#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

namespace {

bool ProximoToken(std::istream& in, std::string& tok) {
  while (in >> tok) {
    if (!tok.empty() && tok[0] == '#') {
      in.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
      continue;
    }
    return true;
  }
  return false;
}

double LerDouble(std::istream& in) {
  std::string tok;
  if (!ProximoToken(in, tok)) {
    std::cerr << "erro: arquivo de entrada terminou antes do esperado\n";
    std::exit(1);
  }
  return std::atof(tok.c_str());
}

int LerInt(std::istream& in) {
  std::string tok;
  if (!ProximoToken(in, tok)) {
    std::cerr << "erro: arquivo de entrada terminou antes do esperado\n";
    std::exit(1);
  }
  return std::atoi(tok.c_str());
}

}

int main(int argc, char** argv) {
  if (argc != 3) {
    std::fprintf(stderr, "uso: %s <arquivo_entrada> <arquivo_saida>\n", argv[0]);
    return 1;
  }

  std::ifstream in(argv[1]);
  if (!in) {
    std::fprintf(stderr, "erro: nao foi possivel abrir %s\n", argv[1]);
    return 1;
  }

  const int nx = LerInt(in);
  const int ny = LerInt(in);
  const int passos = LerInt(in);
  const double alfa = LerDouble(in);
  const double dx = LerDouble(in);
  const double dt = LerDouble(in);

  const size_t total = static_cast<size_t>(nx) * ny;
  std::vector<double> atual(total), proximo(total);
  std::vector<char> fixa(total);

  for (size_t p = 0; p < total; ++p) atual[p] = LerDouble(in);
  for (size_t p = 0; p < total; ++p) fixa[p] = static_cast<char>(LerInt(in));
  in.close();

  proximo = atual;

  const double r = alfa * dt / (dx * dx);
  if (r > 0.25) {
    std::fprintf(stderr, "aviso: r = %.4f > 0.25, o metodo explicito pode divergir\n", r);
  }

  std::printf("=== difusao de calor 2D - versao SERIAL ===\n");
  std::printf("malha  : %d x %d (%zu celulas)\n", nx, ny, total);
  std::printf("passos : %d\n", passos);
  std::printf("r      : %.4f\n", r);

  const auto t0 = std::chrono::steady_clock::now();

  for (int t = 0; t < passos; ++t) {
    for (int j = 1; j < ny - 1; ++j) {
      const size_t linha = static_cast<size_t>(j) * nx;
      for (int i = 1; i < nx - 1; ++i) {
        const size_t p = linha + i;
        if (fixa[p]) {
          proximo[p] = atual[p];
          continue;
        }
        proximo[p] = atual[p] + r * (atual[p + 1] + atual[p - 1] + atual[p + nx] +
                                     atual[p - nx] - 4.0 * atual[p]);
      }
    }
    atual.swap(proximo);
  }

  const auto t1 = std::chrono::steady_clock::now();
  const double segundos = std::chrono::duration<double>(t1 - t0).count();

  double soma = 0.0;
  double tmin = atual[0];
  double tmax = atual[0];
  for (size_t p = 0; p < total; ++p) {
    soma += atual[p];
    if (atual[p] < tmin) tmin = atual[p];
    if (atual[p] > tmax) tmax = atual[p];
  }

  std::printf("tempo de calculo : %.4f s\n", segundos);
  std::printf("temperatura min  : %.4f C\n", tmin);
  std::printf("temperatura max  : %.4f C\n", tmax);
  std::printf("temperatura media: %.6f C\n", soma / total);
  std::printf("checksum (soma)  : %.6f\n", soma);

  FILE* out = std::fopen(argv[2], "w");
  if (!out) {
    std::fprintf(stderr, "erro: nao foi possivel escrever em %s\n", argv[2]);
    return 1;
  }
  std::fprintf(out, "# campo de temperatura final (C) - versao serial\n");
  std::fprintf(out, "%d %d\n", nx, ny);
  for (int j = 0; j < ny; ++j) {
    for (int i = 0; i < nx; ++i) {
      std::fprintf(out, "%.4f%c", atual[static_cast<size_t>(j) * nx + i],
                   i + 1 == nx ? '\n' : ' ');
    }
  }
  std::fclose(out);
  std::printf("saida escrita em : %s\n", argv[2]);
  return 0;
}
