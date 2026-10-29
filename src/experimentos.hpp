#pragma once

#include <cstdint>
#include <filesystem>

/** Parámetros del experimento; los valores por defecto cubren la sección 4 completa. */
struct ConfiguracionExperimento {
    std::filesystem::path datos = "datos/datos.bin";
    int exponente_minimo = 15;
    int exponente_maximo = 26;
    std::uint32_t semilla = 4102;
};

/**
 * Ejecuta los tres árboles para cada N solicitado, con 50 consultas comunes.
 * Crea carpetas nuevas dentro de resultados/ y arboles_generados/, relativas al
 * directorio actual. Devuelve la carpeta de métricas; lanza una excepción ante
 * entradas inválidas o fallos de I/O. No modifica datos ni resultados anteriores.
 */
std::filesystem::path ejecutar_experimentos(const ConfiguracionExperimento& configuracion);
