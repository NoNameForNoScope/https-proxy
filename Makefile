CXX := g++
#CXXFLAGS := -Wall -Wextra -std=c++17 -Iinclude
CXXFLAGS := -Wall -Wextra -std=c++17 -Iinclude -pthread#R
TARGET := proxy_server
TEST_TARGET := parse_test#R
#SRCS := src/main.cpp src/core/HTTPRequest.cpp
SRCS := src/main.cpp src/core/HTTPRequest.cpp src/core/HTTPResponse.cpp src/config/Config.cpp src/log/Logger.cpp src/pool/ThreadPool.cpp src/forward/SocketIO.cpp src/server/Server.cpp src/server/ClientHandler.cpp#R
#HPPS := include/core/HTTPRequest.hpp
HPPS := $(wildcard include/*/*.hpp)#R
TEST_SRCS := tests/parse_test.cpp src/core/HTTPRequest.cpp src/core/HTTPResponse.cpp#R

#.PHONY: all clean
.PHONY: all clean test tsan #R

all: $(TARGET) 

$(TARGET): $(SRCS) $(HPPS)
	$(CXX) $(CXXFLAGS) $(SRCS) -o $@
	#daca include si HPPS incearca sa faca un binar foarte mare *gch

$(TEST_TARGET): $(TEST_SRCS) $(HPPS) #R
	$(CXX) $(CXXFLAGS) $(TEST_SRCS) -o $@ #R

# componenta http se testeaza fara retea (CNF6.2) #R
test: $(TEST_TARGET) #R
	./$(TEST_TARGET) #R

# CNF2.2: build cu ThreadSanitizer #R
tsan: #R
	$(CXX) $(CXXFLAGS) -g -fsanitize=thread $(SRCS) -o $(TARGET)_tsan #R

clean:
#rm -rf proxy_server
	rm -rf proxy_server $(TEST_TARGET) $(TARGET)_tsan #R
