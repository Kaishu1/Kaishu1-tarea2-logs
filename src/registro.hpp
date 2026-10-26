#pragma once

#include <climits>
#include <cstddef>
#include <limits>

// Tamaño de un bloque y, por tanto, de un nodo (enunciado, sección 3).
inline constexpr std::size_t TAMANO_BLOQUE = 4096;

/**
 * Par llave-valor de una medición (sección 2 del enunciado).
 * llave: tiempo Unix en segundos; valor: temperatura en ese instante.
 * Se inicializa en cero y no realiza operaciones de entrada/salida.
 */
struct Registro {
    int llave = 0;
    float valor = 0.0F;
};

static_assert(CHAR_BIT == 8, "Se requieren bytes de 8 bits.");
static_assert(sizeof(int) == 4, "Las llaves e indices requieren int de 4 bytes.");
static_assert(sizeof(float) == 4 && std::numeric_limits<float>::is_iec559,
              "Las temperaturas requieren float IEEE 754 de 4 bytes.");
static_assert(sizeof(Registro) == 8, "Cada registro debe ocupar 8 bytes.");
