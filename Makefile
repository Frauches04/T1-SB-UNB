CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Iinclude

# Montador: main + todas as classes do montador.
MONTADOR_SRCS = src/main_montador.cpp src/Assembler.cpp src/Fonte.cpp \
                src/Diretivas.cpp src/TabelaSimbolos.cpp src/Instrucao.cpp \
                src/Erro.cpp

# Ligador: main + Ligador + Erro. Nao precisa das classes do montador
# porque o Ligador chama montador.exe via system(), nao linka as classes.
LIGADOR_SRCS = src/main_ligador.cpp src/Ligador.cpp src/Erro.cpp

MONTADOR_OBJS = $(MONTADOR_SRCS:.cpp=.o)
LIGADOR_OBJS  = $(LIGADOR_SRCS:.cpp=.o)

all: montador.exe ligador.exe

montador.exe: $(MONTADOR_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

ligador.exe: $(LIGADOR_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(MONTADOR_OBJS) $(LIGADOR_OBJS) montador.exe ligador.exe