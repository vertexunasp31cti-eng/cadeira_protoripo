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

VR3_ALVO   := $(DIR)/test_vr3
VR3_FONTES := test_vr3.cpp stub_arduino/Arduino.cpp ../firmware/cadeira_voz/vr3.cpp

TRT_ALVO   := $(DIR)/test_treinador
TRT_FONTES := test_treinador.cpp stub_arduino/Arduino.cpp

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

rodar: $(ALVO) $(VR3_ALVO) $(TRT_ALVO)
	@./$(ALVO)
	@./$(VR3_ALVO)
	@./$(TRT_ALVO)

# Ferramenta de treinamento contra um V3 simulado no ritmo real de 9600 bps.
$(TRT_ALVO): $(TRT_FONTES) ../firmware/treinar_voz/treinar_voz.ino
	@mkdir -p $(DIR)
	$(CXX) $(CXXFLAGS) -I stub_arduino -o $@ $(TRT_FONTES)

# Driver do modulo de voz contra um modulo V3 simulado.
$(VR3_ALVO): $(VR3_FONTES) ../firmware/cadeira_voz/vr3.h
	@mkdir -p $(DIR)
	$(CXX) $(CXXFLAGS) -I stub_arduino -o $@ $(VR3_FONTES)

# Compila o firmware inteiro no PC com o stub da API do Arduino.
$(VER_ALVO): $(VER_FONTES) $(wildcard ../firmware/cadeira_voz/*.ino ../firmware/cadeira_voz/*.h)
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
