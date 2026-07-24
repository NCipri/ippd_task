CXX      ?= g++
MPICXX   ?= mpicxx
CXXFLAGS ?= -O3 -std=c++17 -Wall

BIN = difusao_serial difusao_mpi_omp gerar_entrada

NX     ?= 512
NY     ?= 512
PASSOS ?= 2000
ENTRADA = entrada/die_$(NX)x$(NY).txt

all: $(BIN)

difusao_serial: src/difusao_serial.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

difusao_mpi_omp: src/difusao_mpi_omp.cpp
	$(MPICXX) $(CXXFLAGS) -fopenmp -o $@ $<

gerar_entrada: src/gerar_entrada.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

entrada: gerar_entrada
	@mkdir -p entrada
	./gerar_entrada $(NX) $(NY) $(PASSOS) $(ENTRADA)

teste: all entrada
	@mkdir -p saida
	./difusao_serial $(ENTRADA) saida/serial.txt
	@echo
	OMP_NUM_THREADS=4 mpirun -np 2 ./difusao_mpi_omp $(ENTRADA) saida/paralelo.txt
	@echo
	python3 scripts/comparar.py saida/serial.txt saida/paralelo.txt

visual: saida/serial.txt saida/paralelo.txt
	python3 scripts/visualizar.py --comparar saida/serial.txt saida/paralelo.txt saida/comparacao.png
	python3 scripts/visualizar.py saida/paralelo.txt saida/campo.png

bench: all entrada
	@mkdir -p saida
	bash scripts/benchmark.sh $(ENTRADA) $(REP)

REP ?= 5

clean:
	rm -f $(BIN)

limpar-dados:
	rm -f entrada/*.txt saida/*.txt

.PHONY: all entrada teste visual bench clean limpar-dados
