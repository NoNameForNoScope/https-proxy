CXX := g++
CXXFLAGS := -Wall -Wextra -std=c++17 -Iinclude
TARGET := proxy_server
SRCS := src/main.cpp src/core/HTTPRequest.cpp
HPPS := include/core/HTTPRequest.hpp

.PHONY: all clean

all: $(TARGET) 

$(TARGET): $(SRCS) $(HPPS)
	$(CXX) $(CXXFLAGS) $(SRCS) -o $@
	#daca include si HPPS incearca sa faca un binar foarte mare *gch

clean:
	rm -rf proxy_server