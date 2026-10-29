#pragma once

#include <cstdint>
#include <filesystem>

/** Datos, tamaños y semilla del experimento; los valores iniciales cubren toda la tarea. */
struct ConfiguracionExperimento {
    std::filesystem::path datos = "datos/datos.bin";  // Ruta del dataset de entrada.
    int exponente_minimo = 15;  // Primer exponente de N = 2^exponente.
    int exponente_maximo = 26;  // Último exponente, incluido.
    std::uint32_t semilla = 4102;  // Semilla para generar los rangos de búsqueda.
};

/**
 * Ejecuta los tres árboles para cada N solicitado, con 50 consultas comunes.
 * configuracion indica la ruta de los datos, los exponentes y la semilla.
 * Crea carpetas nuevas dentro de resultados/ y arboles_generados/, relativas al
 * directorio actual. Devuelve la carpeta de métricas; lanza una excepción ante
 * entradas inválidas o fallos de I/O. No modifica datos ni resultados anteriores.
 */
std::filesystem::path ejecutar_experimentos(const ConfiguracionExperimento& configuracion);
