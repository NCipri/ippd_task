set -u

ENTRADA="${1:-entrada/die_512x512.txt}"
REP="${2:-3}"
MPIRUN_FLAGS="${MPIRUN_FLAGS:---oversubscribe}"

TXT="saida/desempenho.txt"
PNG="saida/desempenho.png"
TMP="saida/.bench_tmp.txt"

if [ ! -x ./difusao_serial ] || [ ! -x ./difusao_mpi_omp ]; then
  echo "erro: binarios nao encontrados, rode 'make' primeiro" >&2
  exit 1
fi
if [ ! -f "$ENTRADA" ]; then
  echo "erro: entrada '$ENTRADA' nao existe, rode 'make entrada' primeiro" >&2
  exit 1
fi

NUCLEOS=$(nproc 2>/dev/null || echo 4)

if [ -z "${CONFIGS:-}" ]; then
  CONFIGS="1:1"
  [ "$NUCLEOS" -ge 2 ] && CONFIGS="$CONFIGS 1:2 2:1"
  [ "$NUCLEOS" -ge 4 ] && CONFIGS="$CONFIGS 1:4 2:2 4:1"
  [ "$NUCLEOS" -ge 8 ] && CONFIGS="$CONFIGS 1:$NUCLEOS 2:$((NUCLEOS/2)) 4:$((NUCLEOS/4)) $NUCLEOS:1"
fi

extrair_tempo() {
  grep -m1 "tempo de calculo" | grep -oE "[0-9]+\.[0-9]+" | head -1
}

mediana() {
  sort -n | awk '{a[NR]=$1} END {
    if (NR == 0) { print "0"; exit }
    print (NR % 2) ? a[(NR+1)/2] : (a[NR/2] + a[NR/2+1]) / 2
  }'
}

echo "entrada     : $ENTRADA"
echo "repeticoes  : $REP (tempo reportado = mediana)"
echo "nucleos     : $NUCLEOS"
echo "configuracoes: $CONFIGS"
echo

printf 'serial (sem MPI, sem OpenMP) ... '
: > "$TMP"
for _ in $(seq 1 "$REP"); do
  ./difusao_serial "$ENTRADA" saida/.bench_saida.txt | extrair_tempo >> "$TMP"
done
T_SERIAL=$(mediana < "$TMP")
echo "${T_SERIAL}s"

DIMS=$(awk '!/^#/ {print $1 "x" $2; exit}' "$ENTRADA")
PASSOS=$(awk '!/^#/ {print $3; exit}' "$ENTRADA")

{
  echo "# comparacao de tempo: versao serial vs versao MPI + OpenMP"
  echo "# entrada: $ENTRADA  malha: $DIMS  passos: $PASSOS"
  echo "# repeticoes por configuracao: $REP (valor reportado = mediana)"
  echo "# nucleos disponiveis na maquina: $NUCLEOS"
  echo "# tempo do serial usado como base do speedup"
  echo "serial $T_SERIAL"
  echo "# processos threads tempo_s speedup eficiencia"
} > "$TXT"

for cfg in $CONFIGS; do
  NP="${cfg%%:*}"
  TH="${cfg##*:}"
  printf 'mpi=%-3s omp=%-3s ... ' "$NP" "$TH"
  : > "$TMP"
  for _ in $(seq 1 "$REP"); do
    OMP_NUM_THREADS="$TH" mpirun $MPIRUN_FLAGS -np "$NP" \
      ./difusao_mpi_omp "$ENTRADA" saida/.bench_saida.txt | extrair_tempo >> "$TMP"
  done
  T=$(mediana < "$TMP")
  awk -v np="$NP" -v th="$TH" -v t="$T" -v ts="$T_SERIAL" 'BEGIN {
    total = np * th
    sp = (t > 0) ? ts / t : 0
    ef = (total > 0) ? sp / total : 0
    printf "%s %s %.4f %.3f %.3f\n", np, th, t, sp, ef
  }' >> "$TXT"
  awk -v t="$T" -v ts="$T_SERIAL" 'BEGIN { printf "%.4fs  speedup %.2fx\n", t, ts/t }'
done

rm -f "$TMP" saida/.bench_saida.txt

echo
echo "=== $TXT ==="
cat "$TXT"

if command -v python3 >/dev/null 2>&1; then
  echo
  python3 scripts/grafico.py "$TXT" "$PNG"
fi
