#include "experimentos.hpp"

#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

namespace {
/** Muestra las opciones existentes, sus valores por defecto y el alcance de ejecución. */
void ayuda() {
    std::cout << "Uso: ./build/tarea2 [opciones]\n"
              << "  --datos RUTA      Dataset binario (datos/datos.bin)\n"
              << "  --min-exp E       Primer exponente de N=2^E (15)\n"
              << "  --max-exp E       Ultimo exponente de N=2^E (26)\n"
              << "  --semilla S       Semilla entera de 32 bits (4102)\n"
              << "  --help            Mostrar esta ayuda\n"
              << "Sin opciones ejecuta los 12 tamanos y los tres arboles, con 50 consultas.\n"
              << "Prueba parcial: ./build/tarea2 --min-exp 15 --max-exp 16\n";
}

/** Convierte texto decimal no negativo a uint32; rechaza texto inválido y desbordes. */
std::uint32_t numero(const std::string& texto) {
    if (texto.empty() || texto.find_first_not_of("0123456789") != std::string::npos) {
        throw std::invalid_argument("Se esperaba un entero no negativo: " + texto);
    }
    const auto valor = std::stoull(texto);
    if (valor > std::numeric_limits<std::uint32_t>::max()) {
        throw std::invalid_argument("Entero fuera del rango de 32 bits: " + texto);
    }
    return static_cast<std::uint32_t>(valor);
}
}  // namespace

/** Lee opciones, ejecuta construcción y consultas; devuelve 0 si termina o 1 ante error. */
int main(int argc, char** argv) {
    try {
        ConfiguracionExperimento configuracion;
        for (int i = 1; i < argc; ++i) {
            const std::string opcion = argv[i];
            if (opcion == "--help") {
                ayuda();
                return 0;
            }
            if (opcion != "--datos" && opcion != "--min-exp"
                && opcion != "--max-exp" && opcion != "--semilla") {
                throw std::invalid_argument("Opcion desconocida: " + opcion);
            }
            if (++i == argc) {
                throw std::invalid_argument("Falta el valor de " + opcion);
            }
            if (opcion == "--datos") {
                configuracion.datos = argv[i];
            } else if (opcion == "--semilla") {
                configuracion.semilla = numero(argv[i]);
            } else {
                const auto valor = numero(argv[i]);
                if (valor < 15 || valor > 26) {
                    throw std::invalid_argument("Los exponentes deben estar entre 15 y 26.");
                }
                if (opcion == "--min-exp") configuracion.exponente_minimo = static_cast<int>(valor);
                else configuracion.exponente_maximo = static_cast<int>(valor);
            }
        }
        ejecutar_experimentos(configuracion);
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
