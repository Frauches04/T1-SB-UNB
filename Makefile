CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -g
TARGET = montador
SRCS = src/main_montador.cpp src/Assembler.cpp src/Fonte.cpp src/Diretivas.cpp src/TabelaSimbolos.cpp src/Instrucao.cpp src/Erro.cpp
OBJS = $(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -Iinclude -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)