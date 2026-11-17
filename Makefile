CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wno-sign-compare
SOURCES = main.cpp tokenizer.cpp table.cpp database.cpp utils.cpp btree.cpp

all: minidb benchmark

minidb: $(SOURCES) $(wildcard *.h)
	$(CXX) $(CXXFLAGS) $(SOURCES) -o minidb

benchmark: benchmark.cpp table.cpp btree.cpp utils.cpp $(wildcard *.h)
	$(CXX) $(CXXFLAGS) benchmark.cpp table.cpp btree.cpp utils.cpp -o benchmark

clean:
	rm -f minidb benchmark minidb.dat minidb.dat.tmp

.PHONY: all clean