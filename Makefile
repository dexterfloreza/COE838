CXX = g++
CXXFLAGS = -std=c++11 -I/sysclibs/include
LDFLAGS = -L/sysclibs/lib-cygwin64 -lsystemc

all: sim

sim: sc_main.cpp top.cpp
	$(CXX) $(CXXFLAGS) sc_main.cpp top.cpp -o sim $(LDFLAGS)

clean:
	rm -f sim
