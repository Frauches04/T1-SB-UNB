
ifeq ($(OS),Windows_NT)
    # Windows (MinGW / MSYS2)
    EXE_EXT   = .exe
    RM        = del /Q
    RM_FLAGS  =
    FIX_PATH  = $(subst /,\,$1)
    MKDIR     = if not exist "$1" mkdir "$1"
    RUN_MONT  = .\montador.exe
    RUN_LIG   = .\ligador.exe
    
else
    # Linux / macOS / WSL
    EXE_EXT   =
    RM        = rm -f
    RM_FLAGS  =
    FIX_PATH  = $1
    MKDIR     = mkdir -p "$1"
    RUN_MONT  = ./montador
    RUN_LIG   = ./ligador
endif


CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -g -Iinclude


MONTADOR_SRCS = src/main_montador.cpp src/Assembler.cpp src/Fonte.cpp \
                src/Diretivas.cpp src/TabelaSimbolos.cpp src/Instrucao.cpp \
                src/Erro.cpp

LIGADOR_SRCS = src/main_ligador.cpp src/Ligador.cpp src/Erro.cpp

MONTADOR_OBJS = $(MONTADOR_SRCS:.cpp=.o)
LIGADOR_OBJS  = $(LIGADOR_SRCS:.cpp=.o)


HEADERS = $(wildcard include/*.h)


MONTADOR = montador$(EXE_EXT)
LIGADOR  = ligador$(EXE_EXT)


.PHONY: all clean run-montador run-ligador info

all: $(MONTADOR) $(LIGADOR)

$(MONTADOR): $(MONTADOR_OBJS)
	@echo "[LINK] $(MONTADOR)"
	$(CXX) $(CXXFLAGS) -o $@ $^

$(LIGADOR): $(LIGADOR_OBJS)
	@echo "[LINK] $(LIGADOR)"
	$(CXX) $(CXXFLAGS) -o $@ $^


%.o: %.cpp $(HEADERS)
	@echo "[CXX] $<"
	$(CXX) $(CXXFLAGS) -c $< -o $@


run-montador: $(MONTADOR)
	@echo "[RUN] $(RUN_MONT) tests/exemplo2.asm"
	$(RUN_MONT) tests/exemplo2.asm

run-ligador: $(LIGADOR)
	@echo "[RUN] $(RUN_LIG) tests/p1.asm tests/p2.asm tests/p3.asm"
	$(RUN_LIG) tests/p1.asm tests/p2.asm tests/p3.asm


info:
	@echo "OS detectado:      $(if $(OS),Windows ($(OS)),Linux/Unix)"
	@echo "Executável montador: $(MONTADOR)"
	@echo "Executável ligador:  $(LIGADOR)"
	@echo "Compilador:          $(CXX)"
	@echo "Flags:               $(CXXFLAGS)"


clean:
	@echo "[CLEAN] removendo objetos e executaveis"
ifeq ($(OS),Windows_NT)
	-@$(RM) $(call FIX_PATH,$(MONTADOR_OBJS)) $(call FIX_PATH,$(LIGADOR_OBJS)) $(MONTADOR) $(LIGADOR) 2>nul
else
	$(RM) $(MONTADOR_OBJS) $(LIGADOR_OBJS) $(MONTADOR) $(LIGADOR)
endif