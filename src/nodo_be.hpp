#pragma once

#include <array>

#include "registro.hpp"

/**
 * Nodo del árbol Bε con el formato de la sección 3.2 del enunciado.
 * C: máximo de llaves divisorias; LMax: máximo de pares en el buffer.
 * Los parámetros determinan los arreglos en compilación y no ocupan
 * espacio en cada nodo. Las llaves divisorias no guardan temperaturas.
 * El buffer contiene datos reales, tanto en hojas como en nodos internos.
 * Se inicializa como hoja vacía; solo se usan las posiciones activas.
 */
template <int C, int LMax>
struct NodoBE {
    static_assert(C > 0 && LMax > 0 && C + LMax == 510,
                  "Las capacidades deben ser positivas y sumar 510.");

    static constexpr int c = C;
    static constexpr int lmax = LMax;

    int es_interno = 0;  // 0: hoja; 1: nodo interno.
    int k_llaves = 0;    // Cantidad de separadores activos; cero en hojas.
    int k_buffer = 0;    // Cantidad de pares activos, ordenados por llave.
    std::array<int, c> llaves{};
    std::array<int, c + 1> hijos{};
    std::array<Registro, lmax> buffer{};
};

// Las dos configuraciones exigidas en la sección 3.2.
using NodoBE025 = NodoBE<4, 506>;
using NodoBE075 = NodoBE<126, 384>;

static_assert(sizeof(NodoBE025) == TAMANO_BLOQUE,
              "El nodo B epsilon 0.25 debe ocupar exactamente 4096 bytes.");
static_assert(sizeof(NodoBE075) == TAMANO_BLOQUE,
              "El nodo B epsilon 0.75 debe ocupar exactamente 4096 bytes.");
