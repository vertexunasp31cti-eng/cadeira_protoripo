# Testes da logica de controle, executados no PC (sem hardware).
#
#   make          compila e roda os testes
#   make limpar   apaga os arquivos gerados

CXX      ?= g++
CXXFLAGS ?= -std=c++11 -Wall -Wextra -O1
DIR      := build
ALVO     := $(DIR)/test_controle
FONTES   := test_controle.cpp ../firmware/cadeira_voz/controle.cpp

TRE_ALVO   := $(DIR)/compila_treinador
TRE_FONTES := compila_treinador.cpp stub_arduino/Arduino.cpp

VER_ALVO   := $(DIR)/compila_firmware
VER_FONTES := compila_firmware.cpp stub_arduino/Arduino.cpp \
              ../firmware/cadeira_voz/controle.cpp \
              ../firmware/cadeira_voz/motores.cpp \
              ../firmware/cadeira_voz/sensor_obstaculo.cpp \
              ../firmware/cadeira_voz/vr3.cpp

.PHONY: all rodar verificar limpar

all: rodar verificar

$(ALVO): $(FONTES) ../firmware/cadeira_voz/controle.h
	@mkdir -p $(DIR)
	$(CXX) $(CXXFLAGS) -o $@ $(FONTES)

rodar: $(ALVO)
	@./$(ALVO)

# Compila o firmware inteiro no PC com o stub da API do Arduino.
$(VER_ALVO): $(VER_FONTES)
	@mkdir -p $(DIR)
	$(CXX) $(CXXFLAGS) -I stub_arduino -x c++ -o $@ $(VER_FONTES)

$(TRE_ALVO): $(TRE_FONTES) ../firmware/treinar_voz/treinar_voz.ino
	@mkdir -p $(DIR)
	$(CXX) $(CXXFLAGS) -I stub_arduino -x c++ -o $@ $(TRE_FONTES)

verificar: $(VER_ALVO) $(TRE_ALVO)
	@./$(VER_ALVO)
	@./$(TRE_ALVO)

limpar:
	rm -rf $(DIR)
