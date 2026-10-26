#include <cstddef>
#include <cstdint>
#include <iostream>
#include <type_traits>

#include "metricas.hpp"
#include "nodo_b.hpp"
#include "nodo_be.hpp"

// Comprobar el orden y los desplazamientos del formato del enunciado.
// Estas comprobaciones se ejecutan en compilación, incluso con NDEBUG.
static_assert(std::is_standard_layout_v<Registro>);
static_assert(std::is_standard_layout_v<NodoB>);
static_assert(std::is_standard_layout_v<NodoBE025>);
static_assert(std::is_standard_layout_v<NodoBE075>);

static_assert(offsetof(Registro, llave) == 0);
static_assert(offsetof(Registro, valor) == 4);

static_assert(offsetof(NodoB, es_interno) == 0);
static_assert(offsetof(NodoB, k) == 4);
static_assert(offsetof(NodoB, llaves_valores) == 8);
static_assert(offsetof(NodoB, hijos) == 2728);
static_assert(offsetof(NodoB, padding) == 4092);

static_assert(offsetof(NodoBE025, es_interno) == 0);
static_assert(offsetof(NodoBE025, k_llaves) == 4);
static_assert(offsetof(NodoBE025, k_buffer) == 8);
static_assert(offsetof(NodoBE025, llaves) == 12);
static_assert(offsetof(NodoBE025, hijos) == 28);
static_assert(offsetof(NodoBE025, buffer) == 48);

static_assert(offsetof(NodoBE075, es_interno) == 0);
static_assert(offsetof(NodoBE075, k_llaves) == 4);
static_assert(offsetof(NodoBE075, k_buffer) == 8);
static_assert(offsetof(NodoBE075, llaves) == 12);
static_assert(offsetof(NodoBE075, hijos) == 516);
static_assert(offsetof(NodoBE075, buffer) == 1024);

/**
 * Ejecuta las comprobaciones básicas de representación y métricas.
 * No recibe parámetros ni requiere el dataset. Devuelve 0 si pasan
 * las comprobaciones y 1 si falla alguna; escribe el resultado en consola.
 * Todavía no prueba lecturas/escrituras en disco ni algoritmos de árboles.
 */
int main() {
    const NodoB nodo_b;
    const NodoBE025 nodo_be025;
    const NodoBE075 nodo_be075;

    if (nodo_b.es_interno != 0 || nodo_b.k != 0 || nodo_b.padding != 0 ||
        nodo_be025.es_interno != 0 || nodo_be025.k_llaves != 0 ||
        nodo_be025.k_buffer != 0 || nodo_be075.es_interno != 0 ||
        nodo_be075.k_llaves != 0 || nodo_be075.k_buffer != 0) {
        std::cerr << "Error: los nodos deben comenzar como hojas vacias.\n";
        return 1;
    }

    ContadorIO contador;
    if (contador.total() != 0) {
        std::cerr << "Error: el contador debe comenzar en cero.\n";
        return 1;
    }
    // El total debe poder superar el máximo de un entero de 32 bits.
    contador.lecturas = std::uint64_t{1} << 32;
    contador.escrituras = 7;
    if (contador.total() != 4294967303ULL) {
        std::cerr << "Error: el total de I/Os no se acumula en 64 bits.\n";
        return 1;
    }

    std::cout << "Registro: " << sizeof(Registro) << " bytes\n"
              << "NodoB: " << sizeof(NodoB) << " bytes\n"
              << "NodoBE025: " << sizeof(NodoBE025) << " bytes\n"
              << "NodoBE075: " << sizeof(NodoBE075) << " bytes\n"
              << "Representacion de nodos y contadores: OK\n";
    return 0;
}
