CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra -Wpedantic

PROGRAMA = build/tarea2
PRUEBAS = build/pruebas_almacenamiento build/pruebas_arboles build/pruebas_experimentos
CABECERAS = src/registro.hpp src/nodo_b.hpp src/nodo_be.hpp src/metricas.hpp \
            src/almacenamiento.hpp src/serializacion.hpp src/arbol.hpp src/arbol_b.hpp \
            src/arbol_be.hpp src/experimentos.hpp

.PHONY: all test clean

# Compila el programa principal y las pruebas.
all: $(PROGRAMA) $(PRUEBAS)

$(PROGRAMA): src/main.cpp src/experimentos.cpp src/arbol_b.cpp $(CABECERAS) Makefile | build
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -DOPCIONES_COMPILACION='"$(CXXFLAGS)"' -Isrc src/main.cpp src/experimentos.cpp src/arbol_b.cpp $(LDFLAGS) $(LDLIBS) -o $@

build/pruebas_experimentos: tests/pruebas_experimentos.cpp src/experimentos.cpp src/arbol_b.cpp $(CABECERAS) Makefile | build
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -DOPCIONES_COMPILACION='"$(CXXFLAGS)"' -Isrc tests/pruebas_experimentos.cpp src/experimentos.cpp src/arbol_b.cpp $(LDFLAGS) $(LDLIBS) -o $@

build/pruebas_almacenamiento: tests/pruebas_almacenamiento.cpp $(CABECERAS) Makefile | build
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -Isrc $< $(LDFLAGS) $(LDLIBS) -o $@

build/pruebas_arboles: tests/pruebas_arboles.cpp src/arbol_b.cpp $(CABECERAS) Makefile | build
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -Isrc tests/pruebas_arboles.cpp src/arbol_b.cpp $(LDFLAGS) $(LDLIBS) -o $@

build:
	mkdir -p $@

test: $(PRUEBAS)
	./build/pruebas_almacenamiento
	./build/pruebas_arboles
	./build/pruebas_experimentos

# Solo elimina los ejecutables producidos por este Makefile.
clean:
	rm -f $(PROGRAMA) $(PRUEBAS)
