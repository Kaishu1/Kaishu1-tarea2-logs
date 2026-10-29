#pragma once

#include <cstddef>
#include <string>

#include "metricas.hpp"
#include "registro.hpp"

/** Interfaz común de construcción en RAM y exportación de los árboles. */
class Arbol {
public:
    /** Destruye la implementación concreta y libera sus nodos. */
    virtual ~Arbol() = default;

    /** Recibe par, lo inserta y acumula sus I/Os de construcción. No devuelve valor. */
    virtual void insertar(Registro par) = 0;

    /** Exporta a ruta; devuelve I/Os de exportación sin alterar las de construcción. */
    virtual ContadorIO guardar(const std::string& ruta) const = 0;

    /** Devuelve los contadores de construcción, incluida la creación de la raíz. */
    virtual ContadorIO metricas() const = 0;

    /** Devuelve la cantidad de nodos, sin acceder a ellos ni contar I/Os. */
    virtual std::size_t cantidad_nodos() const = 0;
};
