ACLOCAL_AMFLAGS = -I m4
AM_CXXFLAGS = -std=c++11 -ggdb -O0 -pg

bin_PROGRAMS = test_rng \
               test_fountain \
               test_bipartite_graph

#lib_LTLIBRARIES = libtestpy.la

test_rng_SOURCES = src/rng.hpp src/rng.cpp \
		   src/test_rng.cpp

test_fountain_SOURCES = src/rng.hpp src/rng.cpp \
			src/packets.hpp src/packets.cpp \
			src/encoder.cpp src/encoder.hpp \
			src/decoder.hpp src/decoder.cpp src/bipartite_graph.hpp \
			src/test_fountain.cpp

test_bipartite_graph_SOURCES = src/bipartite_graph.hpp \
			       src/test_bipartite_graph.cpp

# libtestpy_la_SOURCES = src/test_python.cpp \
# 		      src/rng.hpp \
# 		      src/rng.cpp \
# 		      src/test_rng.cpp

# libtestpy_la_LDFLAGS = -lboost_python-py34 -lpython3.4m

# libtestpy_la_CXXFLAGS = $(AM_CXXFLAGS) -I/usr/include/python3.4/
