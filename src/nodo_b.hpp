#pragma once

#include <array>

#include "registro.hpp"

/**
 * Nodo del árbol B con el formato de la sección 3.1 del enunciado.
 * Contiene hasta b pares ordenados por llave y, si es interno, k + 1
 * índices de hijos. Solo las primeras k posiciones de llaves_valores
 * están activas. Se inicializa como hoja vacía, con padding en cero.
 * Los índices de hijos de una hoja y las posiciones inactivas se ignoran.
 */
struct NodoB {
    static constexpr int b = 340;

    int es_interno = 0;  // 0: hoja; 1: nodo interno.
    int k = 0;          // Cantidad de pares activos.
    std::array<Registro, b> llaves_valores{};
    std::array<int, b + 1> hijos{};
    int padding = 0;
};

static_assert(sizeof(NodoB) == TAMANO_BLOQUE,
              "El nodo B debe ocupar exactamente 4096 bytes.");
