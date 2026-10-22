# Testes do prototipo, executados no PC (sem hardware).
#
#   make            compila e roda todos os testes
#   make simulador  compila os dois firmwares para o simulador no PC
#   make limpar     apaga os arquivos gerados

CXX      ?= g++
CXXFLAGS ?= -std=c++11 -Wall -Wextra -O1
PYTHON   ?= python3
DIR      := build
MEGA     := ../firmware/cadeira_voz
ESP      := ../firmware/esp_wifi

# Maquina de estados.
ALVO     := $(DIR)/test_controle
FONTES   := test_controle.cpp $(MEGA)/controle.cpp

# Protocolo do app, lado do Mega.
PRO_ALVO   := $(DIR)/test_protocolo
PRO_FONTES := test_protocolo.cpp $(MEGA)/protocolo_app.cpp $(MEGA)/controle.cpp

# Ponte serial e API do ESP8266, sem o hardware do ESP.
PON_ALVO   := $(DIR)/test_ponte
PON_FONTES := test_ponte.cpp $(ESP)/ponte.cpp

# Os dois lados juntos: o que o Mega escreve, o ESP entende, e vice-versa.
INT_ALVO   := $(DIR)/test_integracao
INT_FONTES := test_integracao.cpp $(MEGA)/protocolo_app.cpp \
              $(MEGA)/controle.cpp $(ESP)/ponte.cpp

TRE_ALVO   := $(DIR)/compila_treinador
TRE_FONTES := compila_treinador.cpp stub_arduino/Arduino.cpp

VER_ALVO   := $(DIR)/compila_firmware
VER_FONTES := compila_firmware.cpp stub_arduino/Arduino.cpp \
              $(MEGA)/controle.cpp \
              $(MEGA)/motores.cpp \
              $(MEGA)/sensor_obstaculo.cpp \
              $(MEGA)/vr3.cpp \
              $(MEGA)/protocolo_app.cpp

# Ensaios do sketch inteiro do Mega, em tres configuracoes.
ENS_FONTES := ensaio_firmware.cpp stub_arduino/Arduino.cpp \
              $(MEGA)/controle.cpp \
              $(MEGA)/motores.cpp \
              $(MEGA)/sensor_obstaculo.cpp \
              $(MEGA)/vr3.cpp \
              $(MEGA)/protocolo_app.cpp
ENS_ALVO   := $(DIR)/ensaio_firmware
ENS_SO_APP := $(DIR)/ensaio_so_acompanha
ENS_SEM_ESP := $(DIR)/ensaio_sem_esp

# O ESP usa substitutos proprios (stub_esp8266), sem o stub_arduino do Mega.
ESP_ALVO   := $(DIR)/compila_esp
ESP_FONTES := compila_esp.cpp $(wildcard stub_esp8266/*.cpp) $(ESP)/ponte.cpp

# Simulador: os dois firmwares rodando no PC (testes/simulador/).
ESIM_ALVO   := $(DIR)/esp_sim
ESIM_FONTES := simulador/esp_sim.cpp $(wildcard stub_esp8266/*.cpp) \
               $(ESP)/ponte.cpp

SIM_ALVO   := $(DIR)/mega_sim
SIM_SO_APP := $(DIR)/mega_sim_so_acompanha
SIM_FONTES := simulador/mega_sim.cpp stub_arduino/Arduino.cpp \
              $(MEGA)/controle.cpp \
              $(MEGA)/motores.cpp \
              $(MEGA)/sensor_obstaculo.cpp \
              $(MEGA)/vr3.cpp \
              $(MEGA)/protocolo_app.cpp

MEGA_H := $(wildcard $(MEGA)/*.h) $(MEGA)/cadeira_voz.ino
ESP_H  := $(wildcard $(ESP)/*.h) $(ESP)/esp_wifi.ino
STUB_H := $(wildcard stub_arduino/*.h) $(wildcard stub_esp8266/*.h)

.PHONY: all rodar verificar pagina simulador limpar

all: rodar verificar pagina

$(ALVO): $(FONTES) $(MEGA)/controle.h
	@mkdir -p $(DIR)
	$(CXX) $(CXXFLAGS) -o $@ $(FONTES)

$(PRO_ALVO): $(PRO_FONTES) $(MEGA_H)
	@mkdir -p $(DIR)
	$(CXX) $(CXXFLAGS) -o $@ $(PRO_FONTES)

$(PON_ALVO): $(PON_FONTES) $(ESP_H)
	@mkdir -p $(DIR)
	$(CXX) $(CXXFLAGS) -o $@ $(PON_FONTES)

$(INT_ALVO): $(INT_FONTES) $(MEGA_H) $(ESP_H)
	@mkdir -p $(DIR)
	$(CXX) $(CXXFLAGS) -o $@ $(INT_FONTES)

rodar: $(ALVO) $(PRO_ALVO) $(PON_ALVO) $(INT_ALVO)
	@./$(ALVO)
	@./$(PRO_ALVO)
	@./$(PON_ALVO)
	@./$(INT_ALVO)

# Compila os firmwares inteiros no PC com substitutos das bibliotecas.
$(VER_ALVO): $(VER_FONTES) $(MEGA_H) $(STUB_H)
	@mkdir -p $(DIR)
	$(CXX) $(CXXFLAGS) -I stub_arduino -x c++ -o $@ $(VER_FONTES)

$(TRE_ALVO): $(TRE_FONTES) ../firmware/treinar_voz/treinar_voz.ino
	@mkdir -p $(DIR)
	$(CXX) $(CXXFLAGS) -I stub_arduino -x c++ -o $@ $(TRE_FONTES)

$(ESP_ALVO): $(ESP_FONTES) $(ESP_H) $(STUB_H)
	@mkdir -p $(DIR)
	$(CXX) $(CXXFLAGS) -I stub_esp8266 -x c++ -o $@ $(ESP_FONTES)

$(ENS_ALVO): $(ENS_FONTES) $(MEGA_H) $(STUB_H)
	@mkdir -p $(DIR)
	$(CXX) $(CXXFLAGS) -I stub_arduino -x c++ -o $@ $(ENS_FONTES)

$(ENS_SO_APP): $(ENS_FONTES) $(MEGA_H) $(STUB_H)
	@mkdir -p $(DIR)
	$(CXX) $(CXXFLAGS) -DPERMITIR_COMANDO_APP=0 -I stub_arduino -x c++ -o $@ $(ENS_FONTES)

$(ENS_SEM_ESP): $(ENS_FONTES) $(MEGA_H) $(STUB_H)
	@mkdir -p $(DIR)
	$(CXX) $(CXXFLAGS) -DUSAR_ESP8266=0 -I stub_arduino -x c++ -o $@ $(ENS_FONTES)

verificar: $(VER_ALVO) $(TRE_ALVO) $(ESP_ALVO) $(ENS_ALVO) $(ENS_SO_APP) $(ENS_SEM_ESP)
	@./$(VER_ALVO)
	@./$(TRE_ALVO)
	@./$(ESP_ALVO)
	@./$(ENS_ALVO)
	@./$(ENS_SO_APP)
	@./$(ENS_SEM_ESP)

# O app embutido no ESP (pagina.h) precisa ser igual ao app/index.html.
pagina:
	@if command -v $(PYTHON) >/dev/null 2>&1; then \
	  $(PYTHON) ../ferramentas/gerar_pagina.py --verificar; \
	else \
	  echo "python3 nao encontrado: verificacao do pagina.h pulada"; \
	fi

# Os dois firmwares rodando no PC, para o simulador (testes/simulador/).
simulador: $(SIM_ALVO) $(ESIM_ALVO) $(SIM_SO_APP)

$(SIM_ALVO): $(SIM_FONTES) $(MEGA_H) $(STUB_H)
	@mkdir -p $(DIR)
	$(CXX) $(CXXFLAGS) -I stub_arduino -x c++ -o $@ $(SIM_FONTES)

# Mega com PERMITIR_COMANDO_APP 0, para simular o app so acompanhando:
#   python3 simulador/servidor.py --mega build/mega_sim_so_acompanha
$(SIM_SO_APP): $(SIM_FONTES) $(MEGA_H) $(STUB_H)
	@mkdir -p $(DIR)
	$(CXX) $(CXXFLAGS) -DPERMITIR_COMANDO_APP=0 -I stub_arduino -x c++ -o $@ $(SIM_FONTES)

$(ESIM_ALVO): $(ESIM_FONTES) $(ESP_H) $(STUB_H)
	@mkdir -p $(DIR)
	$(CXX) $(CXXFLAGS) -I stub_esp8266 -x c++ -o $@ $(ESIM_FONTES)

limpar:
	rm -rf $(DIR)
