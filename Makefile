CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -g
TARGET = montador
SRCS = src/main.cpp src/Assembler.cpp
OBJS = $(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -Iinclude -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
