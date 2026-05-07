CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O3 -DNDEBUG -march=native -flto -pipe -Wno-unused-result
LDFLAGS  ?= -flto

build:
	$(CXX) $(CXXFLAGS) -I include -o pace26.out cpp/*.cpp $(LDFLAGS)

debug:
	$(CXX) -std=c++17 -O0 -g -I include -o pace26.dbg cpp/*.cpp