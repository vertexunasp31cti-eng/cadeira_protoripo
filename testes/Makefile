# Testes da logica de controle, executados no PC (sem hardware).
#
#   make          compila e roda os testes (cadeira, app e compilacao)
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
              ../firmware/cadeira_voz/protocolo_app.cpp \
              ../firmware/cadeira_voz/bateria.cpp \
              ../firmware/cadeira_voz/motores.cpp \
              ../firmware/cadeira_voz/sensor_obstaculo.cpp \
              ../firmware/cadeira_voz/vr3.cpp

BAT_ALVO   := $(DIR)/test_bateria
BAT_FONTES := test_bateria.cpp ../firmware/cadeira_voz/bateria.cpp

# Ligacao app -> ESP8266 -> Mega, com stubs do Wi-Fi e do servidor HTTP.
APP_ALVO   := $(DIR)/test_app
APP_FONTES := test_app.cpp stub_arduino/Arduino.cpp \
              ../firmware/cadeira_voz/controle.cpp \
              ../firmware/cadeira_voz/protocolo_app.cpp

.PHONY: all rodar bateria app verificar limpar

all: rodar bateria app verificar

$(ALVO): $(FONTES) ../firmware/cadeira_voz/controle.h
	@mkdir -p $(DIR)
	$(CXX) $(CXXFLAGS) -o $@ $(FONTES)

rodar: $(ALVO)
	@./$(ALVO)

$(BAT_ALVO): $(BAT_FONTES) ../firmware/cadeira_voz/bateria.h
	@mkdir -p $(DIR)
	$(CXX) $(CXXFLAGS) -o $@ $(BAT_FONTES)

bateria: $(BAT_ALVO)
	@./$(BAT_ALVO)

$(APP_ALVO): $(APP_FONTES) ../firmware/app_wifi/app_wifi.ino stub_esp8266/*.h \
             ../firmware/app_wifi/pagina.h ../firmware/cadeira_voz/protocolo_app.h
	@mkdir -p $(DIR)
	$(CXX) $(CXXFLAGS) -I stub_arduino -I stub_esp8266 -o $@ $(APP_FONTES)

app: $(APP_ALVO)
	@./$(APP_ALVO)

# Compila o firmware inteiro no PC com o stub da API do Arduino.
$(VER_ALVO): $(VER_FONTES) ../firmware/cadeira_voz/cadeira_voz.ino
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
