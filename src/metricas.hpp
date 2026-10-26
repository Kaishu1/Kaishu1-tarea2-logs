#pragma once

#include <cstdint>

/**
 * Cuenta lecturas y escrituras de nodos, fuera de los bloques del árbol.
 * Se inicia en cero. Se usará una instancia separada para cada etapa
 * medida; estos campos por sí solos no interceptan ni cuentan accesos.
 * Se usan 64 bits para acumular las I/Os de los experimentos grandes.
 */
struct ContadorIO {
    std::uint64_t lecturas = 0;
    std::uint64_t escrituras = 0;

    /** Devuelve lecturas + escrituras, sin parámetros ni efectos laterales. */
    std::uint64_t total() const {
        return lecturas + escrituras;
    }
};
