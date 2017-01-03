AM_CXXFLAGS = -std=c++11 -ggdb -O0

bin_PROGRAMS = test_rng test_fountain test_bipartite_graph

test_rng_SOURCES = src/rng.hpp src/rng.cpp \
		   src/test_rng.cpp

test_fountain_SOURCES = src/rng.hpp src/rng.cpp \
			src/packets.hpp src/packets.cpp \
			src/encoder.cpp src/encoder.hpp \
			src/decoder.hpp src/decoder.cpp src/bipartite_graph.hpp \
			src/test_fountain.cpp

test_bipartite_graph_SOURCES = src/bipartite_graph.hpp \
			       src/test_bipartite_graph.cpp
