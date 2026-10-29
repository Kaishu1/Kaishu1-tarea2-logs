CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra -Wpedantic

PRUEBAS = build/pruebas_almacenamiento
CABECERAS = src/registro.hpp src/nodo_b.hpp src/nodo_be.hpp src/metricas.hpp \
            src/almacenamiento.hpp src/serializacion.hpp

.PHONY: all test clean

# Compila las pruebas de registros, nodos y almacenamiento compartido.
all: $(PRUEBAS)

$(PRUEBAS): tests/pruebas_almacenamiento.cpp $(CABECERAS) Makefile | build
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -Isrc $< $(LDFLAGS) $(LDLIBS) -o $@

build:
	mkdir -p $@

test: $(PRUEBAS)
	./$(PRUEBAS)

# Solo elimina el ejecutable producido por este Makefile.
clean:
	rm -f $(PRUEBAS)
