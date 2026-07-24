#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

namespace {

constexpr double kAlfaSilicio = 148.0 / (2329.0 * 700.0);
constexpr double kLadoDie = 0.020;
constexpr double kFatorEstabilidade = 0.20;

constexpr double kTempBorda = 45.0;
constexpr double kTempInicial = 45.0;

struct Bloco {
  const char* nome;
  double x0, y0, x1, y1;
  double temperatura;
};

const Bloco kFloorplan[] = {
    {"nucleo 0", 0.08, 0.10, 0.30, 0.34, 95.0},
    {"nucleo 1", 0.08, 0.40, 0.30, 0.64, 95.0},
    {"nucleo 2", 0.36, 0.10, 0.58, 0.34, 92.0},
    {"nucleo 3", 0.36, 0.40, 0.58, 0.64, 92.0},
    {"cache L3", 0.08, 0.70, 0.58, 0.88, 70.0},
    {"iGPU", 0.64, 0.10, 0.92, 0.55, 85.0},
    {"controlador de memoria", 0.64, 0.62, 0.92, 0.88, 75.0},
};
constexpr int kNumBlocos = sizeof(kFloorplan) / sizeof(kFloorplan[0]);

}

int main(int argc, char** argv) {
  if (argc != 5) {
    std::fprintf(stderr, "uso: %s <nx> <ny> <passos> <arquivo_saida>\n", argv[0]);
    std::fprintf(stderr, "exemplo: %s 512 512 2000 entrada/die_512.txt\n", argv[0]);
    return 1;
  }

  const int nx = std::atoi(argv[1]);
  const int ny = std::atoi(argv[2]);
  const int passos = std::atoi(argv[3]);
  const std::string caminho = argv[4];

  if (nx < 8 || ny < 8 || passos < 1) {
    std::fprintf(stderr, "erro: use nx,ny >= 8 e passos >= 1\n");
    return 1;
  }

  const double dx = kLadoDie / (nx - 1);
  const double dt = kFatorEstabilidade * dx * dx / kAlfaSilicio;

  std::vector<double> temp(static_cast<size_t>(nx) * ny, kTempInicial);
  std::vector<char> fixa(static_cast<size_t>(nx) * ny, 0);

  for (int i = 0; i < nx; ++i) {
    temp[i] = kTempBorda;
    fixa[i] = 1;
    temp[static_cast<size_t>(ny - 1) * nx + i] = kTempBorda;
    fixa[static_cast<size_t>(ny - 1) * nx + i] = 1;
  }
  for (int j = 0; j < ny; ++j) {
    temp[static_cast<size_t>(j) * nx] = kTempBorda;
    fixa[static_cast<size_t>(j) * nx] = 1;
    temp[static_cast<size_t>(j) * nx + nx - 1] = kTempBorda;
    fixa[static_cast<size_t>(j) * nx + nx - 1] = 1;
  }

  for (int b = 0; b < kNumBlocos; ++b) {
    const Bloco& bl = kFloorplan[b];
    const int i0 = static_cast<int>(bl.x0 * (nx - 1));
    const int i1 = static_cast<int>(bl.x1 * (nx - 1));
    const int j0 = static_cast<int>(bl.y0 * (ny - 1));
    const int j1 = static_cast<int>(bl.y1 * (ny - 1));
    for (int j = j0; j <= j1; ++j) {
      for (int i = i0; i <= i1; ++i) {
        const size_t p = static_cast<size_t>(j) * nx + i;
        if (fixa[p]) continue;
        temp[p] = bl.temperatura;
        fixa[p] = 1;
      }
    }
  }

  FILE* f = std::fopen(caminho.c_str(), "w");
  if (!f) {
    std::fprintf(stderr, "erro: nao foi possivel escrever em %s\n", caminho.c_str());
    return 1;
  }

  std::fprintf(f, "# Difusao de calor 2D no die de um processador (%d x %d celulas)\n", nx, ny);
  std::fprintf(f, "# Die de silicio de %.0f mm de lado; borda presa a %.1f C (dissipador).\n",
               kLadoDie * 1000.0, kTempBorda);
  std::fprintf(f, "# Blocos funcionais mantidos na temperatura de operacao:\n");
  for (int b = 0; b < kNumBlocos; ++b) {
    std::fprintf(f, "#   %-24s %.1f C\n", kFloorplan[b].nome, kFloorplan[b].temperatura);
  }
  std::fprintf(f, "# Cabecalho: nx ny passos alfa dx dt\n");
  std::fprintf(f, "# r = alfa*dt/dx^2 = %.4f (estavel enquanto r <= 0.25)\n",
               kAlfaSilicio * dt / (dx * dx));
  std::fprintf(f, "%d %d %d %.10e %.10e %.10e\n", nx, ny, passos, kAlfaSilicio, dx, dt);

  std::fprintf(f, "# temperaturas iniciais (C)\n");
  for (int j = 0; j < ny; ++j) {
    for (int i = 0; i < nx; ++i) {
      std::fprintf(f, "%.2f%c", temp[static_cast<size_t>(j) * nx + i], i + 1 == nx ? '\n' : ' ');
    }
  }

  std::fprintf(f, "# mascara de celulas fixas (1 = fixa, 0 = livre)\n");
  for (int j = 0; j < ny; ++j) {
    for (int i = 0; i < nx; ++i) {
      std::fprintf(f, "%d%c", fixa[static_cast<size_t>(j) * nx + i], i + 1 == nx ? '\n' : ' ');
    }
  }

  std::fclose(f);

  size_t livres = 0;
  for (size_t p = 0; p < fixa.size(); ++p) livres += (fixa[p] == 0);
  std::printf("gerado: %s\n", caminho.c_str());
  std::printf("  malha       : %d x %d (%zu celulas, %zu livres)\n", nx, ny,
              static_cast<size_t>(nx) * ny, livres);
  std::printf("  passos      : %d\n", passos);
  std::printf("  dx          : %.3e m\n", dx);
  std::printf("  dt          : %.3e s\n", dt);
  std::printf("  tempo total : %.3e s de simulacao fisica\n", dt * passos);
  return 0;
}
