#include <mpi.h>
#include <omp.h>

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
    MPI_Abort(MPI_COMM_WORLD, 1);
  }
  return std::atof(tok.c_str());
}

int LerInt(std::istream& in) {
  std::string tok;
  if (!ProximoToken(in, tok)) {
    std::cerr << "erro: arquivo de entrada terminou antes do esperado\n";
    MPI_Abort(MPI_COMM_WORLD, 1);
  }
  return std::atoi(tok.c_str());
}

}

int main(int argc, char** argv) {
  int fornecido = 0;
  MPI_Init_thread(&argc, &argv, MPI_THREAD_FUNNELED, &fornecido);

  int rank = 0, nprocs = 1;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

  if (argc != 3) {
    if (rank == 0) {
      std::fprintf(stderr, "uso: mpirun -np <P> %s <arquivo_entrada> <arquivo_saida>\n", argv[0]);
    }
    MPI_Finalize();
    return 1;
  }

  int cabecalho[3] = {0, 0, 0};
  double parametros[3] = {0, 0, 0};
  std::vector<double> malha_global;
  std::vector<char> fixa_global;

  if (rank == 0) {
    std::ifstream in(argv[1]);
    if (!in) {
      std::fprintf(stderr, "erro: nao foi possivel abrir %s\n", argv[1]);
      MPI_Abort(MPI_COMM_WORLD, 1);
    }
    cabecalho[0] = LerInt(in);
    cabecalho[1] = LerInt(in);
    cabecalho[2] = LerInt(in);
    parametros[0] = LerDouble(in);
    parametros[1] = LerDouble(in);
    parametros[2] = LerDouble(in);

    const size_t total = static_cast<size_t>(cabecalho[0]) * cabecalho[1];
    malha_global.resize(total);
    fixa_global.resize(total);
    for (size_t p = 0; p < total; ++p) malha_global[p] = LerDouble(in);
    for (size_t p = 0; p < total; ++p) fixa_global[p] = static_cast<char>(LerInt(in));
  }

  MPI_Bcast(cabecalho, 3, MPI_INT, 0, MPI_COMM_WORLD);
  MPI_Bcast(parametros, 3, MPI_DOUBLE, 0, MPI_COMM_WORLD);

  const int nx = cabecalho[0];
  const int ny = cabecalho[1];
  const int passos = cabecalho[2];
  const double alfa = parametros[0];
  const double dx = parametros[1];
  const double dt = parametros[2];
  const double r = alfa * dt / (dx * dx);

  if (nprocs > ny) {
    if (rank == 0) {
      std::fprintf(stderr, "erro: %d processos para apenas %d linhas de malha\n", nprocs, ny);
    }
    MPI_Abort(MPI_COMM_WORLD, 1);
  }

  std::vector<int> linhas(nprocs), contagem(nprocs), deslocamento(nprocs);
  int acumulado = 0;
  for (int p = 0; p < nprocs; ++p) {
    linhas[p] = ny / nprocs + (p < ny % nprocs ? 1 : 0);
    contagem[p] = linhas[p] * nx;
    deslocamento[p] = acumulado;
    acumulado += contagem[p];
  }
  const int linhas_locais = linhas[rank];
  const int primeira_linha = deslocamento[rank] / nx;

  const size_t tam_local = static_cast<size_t>(linhas_locais + 2) * nx;
  std::vector<double> atual(tam_local, 0.0), proximo(tam_local, 0.0);
  std::vector<char> fixa(tam_local, 1);

  MPI_Scatterv(rank == 0 ? malha_global.data() : nullptr, contagem.data(), deslocamento.data(),
               MPI_DOUBLE, atual.data() + nx, contagem[rank], MPI_DOUBLE, 0, MPI_COMM_WORLD);
  MPI_Scatterv(rank == 0 ? fixa_global.data() : nullptr, contagem.data(), deslocamento.data(),
               MPI_CHAR, fixa.data() + nx, contagem[rank], MPI_CHAR, 0, MPI_COMM_WORLD);

  proximo = atual;

  const int vizinho_cima = (rank == 0) ? MPI_PROC_NULL : rank - 1;
  const int vizinho_baixo = (rank == nprocs - 1) ? MPI_PROC_NULL : rank + 1;

  int threads = 1;
#pragma omp parallel
  {
#pragma omp master
    threads = omp_get_num_threads();
  }

  if (rank == 0) {
    std::printf("=== difusao de calor 2D - versao PARALELA (MPI + OpenMP) ===\n");
    std::printf("malha     : %d x %d (%zu celulas)\n", nx, ny,
                static_cast<size_t>(nx) * ny);
    std::printf("passos    : %d\n", passos);
    std::printf("r         : %.4f%s\n", r, r > 0.25 ? "  (AVISO: > 0.25, pode divergir)" : "");
    std::printf("processos : %d\n", nprocs);
    std::printf("threads   : %d por processo (%d threads no total)\n", threads, threads * nprocs);
  }
  MPI_Barrier(MPI_COMM_WORLD);
  std::printf("  rank %2d: linhas %d..%d (%d linhas)\n", rank, primeira_linha,
              primeira_linha + linhas_locais - 1, linhas_locais);
  std::fflush(stdout);
  MPI_Barrier(MPI_COMM_WORLD);

  const double t0 = MPI_Wtime();
  double tempo_comunicacao = 0.0;

  for (int t = 0; t < passos; ++t) {
    const double tc0 = MPI_Wtime();
    MPI_Sendrecv(atual.data() + nx, nx, MPI_DOUBLE, vizinho_cima, 0,
                 atual.data() + static_cast<size_t>(linhas_locais + 1) * nx, nx, MPI_DOUBLE,
                 vizinho_baixo, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    MPI_Sendrecv(atual.data() + static_cast<size_t>(linhas_locais) * nx, nx, MPI_DOUBLE,
                 vizinho_baixo, 1, atual.data(), nx, MPI_DOUBLE, vizinho_cima, 1,
                 MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    tempo_comunicacao += MPI_Wtime() - tc0;

#pragma omp parallel for schedule(static)
    for (int j = 1; j <= linhas_locais; ++j) {
      const size_t linha = static_cast<size_t>(j) * nx;
      for (int i = 1; i < nx - 1; ++i) {
        const size_t p = linha + i;
        if (fixa[p]) continue;
        proximo[p] = atual[p] + r * (atual[p + 1] + atual[p - 1] + atual[p + nx] +
                                     atual[p - nx] - 4.0 * atual[p]);
      }
    }

    atual.swap(proximo);
  }

  const double tempo_local = MPI_Wtime() - t0;

  double soma_local = 0.0;
  double min_local = atual[nx];
  double max_local = atual[nx];
  for (int j = 1; j <= linhas_locais; ++j) {
    for (int i = 0; i < nx; ++i) {
      const double v = atual[static_cast<size_t>(j) * nx + i];
      soma_local += v;
      if (v < min_local) min_local = v;
      if (v > max_local) max_local = v;
    }
  }

  double soma = 0.0, tmin = 0.0, tmax = 0.0, tempo_total = 0.0, comm_max = 0.0;
  MPI_Reduce(&soma_local, &soma, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
  MPI_Reduce(&min_local, &tmin, 1, MPI_DOUBLE, MPI_MIN, 0, MPI_COMM_WORLD);
  MPI_Reduce(&max_local, &tmax, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
  MPI_Reduce(&tempo_local, &tempo_total, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
  MPI_Reduce(&tempo_comunicacao, &comm_max, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

  MPI_Gatherv(atual.data() + nx, contagem[rank], MPI_DOUBLE,
              rank == 0 ? malha_global.data() : nullptr, contagem.data(), deslocamento.data(),
              MPI_DOUBLE, 0, MPI_COMM_WORLD);

  if (rank == 0) {
    const size_t total = static_cast<size_t>(nx) * ny;
    std::printf("tempo de calculo : %.4f s\n", tempo_total);
    std::printf("  comunicacao    : %.4f s (%.1f%% do total)\n", comm_max,
                tempo_total > 0 ? 100.0 * comm_max / tempo_total : 0.0);
    std::printf("temperatura min  : %.4f C\n", tmin);
    std::printf("temperatura max  : %.4f C\n", tmax);
    std::printf("temperatura media: %.6f C\n", soma / total);
    std::printf("checksum (soma)  : %.6f\n", soma);

    FILE* out = std::fopen(argv[2], "w");
    if (!out) {
      std::fprintf(stderr, "erro: nao foi possivel escrever em %s\n", argv[2]);
      MPI_Abort(MPI_COMM_WORLD, 1);
    }
    std::fprintf(out, "# campo de temperatura final (C) - %d processos MPI x %d threads OpenMP\n",
                 nprocs, threads);
    std::fprintf(out, "%d %d\n", nx, ny);
    for (int j = 0; j < ny; ++j) {
      for (int i = 0; i < nx; ++i) {
        std::fprintf(out, "%.4f%c", malha_global[static_cast<size_t>(j) * nx + i],
                     i + 1 == nx ? '\n' : ' ');
      }
    }
    std::fclose(out);
    std::printf("saida escrita em : %s\n", argv[2]);
  }

  MPI_Finalize();
  return 0;
}
