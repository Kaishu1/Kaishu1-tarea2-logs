#pragma once

#include <istream>
#include <ostream>
#include <stdexcept>
#include <type_traits>

#include "registro.hpp"

/**
 * Escribe un nodo como un bloque de 4096 bytes en la posición de salida actual.
 * salida: flujo binario abierto; nodo: estructura que se desea guardar.
 * No devuelve un valor ni modifica el nodo. Lanza runtime_error si falla.
 * Usa la representación nativa de los tipos: lectura y escritura deben
 * realizarse con la misma disposición de campos y orden de bytes.
 */
template <typename Nodo>
void escribir_nodo(std::ostream& salida, const Nodo& nodo) {
    static_assert(sizeof(Nodo) == TAMANO_BLOQUE);
    static_assert(std::is_trivially_copyable_v<Nodo>);
    salida.write(reinterpret_cast<const char*>(&nodo), TAMANO_BLOQUE);
    if (!salida) {
        throw std::runtime_error("No se pudo escribir un bloque completo.");
    }
}

/**
 * Lee un bloque de 4096 bytes desde la posición actual y devuelve su nodo.
 * entrada: flujo binario abierto con el formato nativo usado por escribir_nodo.
 * Avanza el flujo un bloque. Lanza runtime_error si faltan bytes o falla la
 * lectura; nunca devuelve un nodo parcialmente leído. No cuenta I/Os.
 */
template <typename Nodo>
Nodo leer_nodo(std::istream& entrada) {
    static_assert(sizeof(Nodo) == TAMANO_BLOQUE);
    static_assert(std::is_trivially_copyable_v<Nodo>);
    Nodo nodo;
    entrada.read(reinterpret_cast<char*>(&nodo), TAMANO_BLOQUE);
    if (!entrada) {
        throw std::runtime_error("No se pudo leer un bloque completo.");
    }
    return nodo;
}
