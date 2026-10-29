CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra -Wpedantic

PRUEBAS = build/pruebas_almacenamiento build/pruebas_arboles
CABECERAS = src/registro.hpp src/nodo_b.hpp src/nodo_be.hpp src/metricas.hpp \
            src/almacenamiento.hpp src/serializacion.hpp src/arbol.hpp src/arbol_b.hpp

.PHONY: all test clean

# Compila las pruebas de almacenamiento y árboles implementados.
all: $(PRUEBAS)

build/pruebas_almacenamiento: tests/pruebas_almacenamiento.cpp $(CABECERAS) Makefile | build
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -Isrc $< $(LDFLAGS) $(LDLIBS) -o $@

build/pruebas_arboles: tests/pruebas_arboles.cpp src/arbol_b.cpp $(CABECERAS) Makefile | build
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -Isrc tests/pruebas_arboles.cpp src/arbol_b.cpp $(LDFLAGS) $(LDLIBS) -o $@

build:
	mkdir -p $@

test: $(PRUEBAS)
	./build/pruebas_almacenamiento
	./build/pruebas_arboles

# Solo elimina los ejecutables producidos por este Makefile.
clean:
	rm -f $(PRUEBAS)
