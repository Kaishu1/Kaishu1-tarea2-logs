#pragma once

#include <cstddef>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "metricas.hpp"
#include "serializacion.hpp"

/**
 * Disco simulado mediante un vector privado de nodos (enunciado, sección 3.3).
 * Nodo: NodoB, NodoBE025 o NodoBE075. La raíz ocupa siempre el índice cero.
 * Cada lectura devuelve una copia y cada escritura debe ser explícita.
 * Las reasignaciones internas del vector son trabajo de RAM, no I/Os simuladas.
 */
template <typename Nodo>
class AlmacenamientoRAM {
public:
    /** Sin parámetros: crea una raíz vacía en el índice cero y cuenta una escritura. */
    AlmacenamientoRAM() {
        agregar(Nodo{});
    }

    /**
     * Devuelve una copia del nodo en indice y cuenta una lectura.
     * Lanza out_of_range si el índice no existe; en ese caso no cuenta una I/O.
     */
    Nodo leer(int indice) {
        Nodo nodo = nodos_.at(static_cast<std::size_t>(indice));
        ++metricas_.lecturas;
        return nodo;
    }

    /**
     * Reemplaza el nodo en indice por nodo y cuenta una escritura; no devuelve valor.
     * Lanza out_of_range si el índice no existe, sin modificar nodos ni contadores.
     */
    void escribir(int indice, const Nodo& nodo) {
        nodos_.at(static_cast<std::size_t>(indice)) = nodo;
        ++metricas_.escrituras;
    }

    /**
     * Agrega nodo al final, cuenta una escritura y devuelve su índice.
     * Los índices int son suficientes para los tamaños de entrada de la tarea.
     */
    int agregar(const Nodo& nodo) {
        const int indice = static_cast<int>(nodos_.size());
        nodos_.push_back(nodo);
        ++metricas_.escrituras;
        return indice;
    }

    /** Sin parámetros: devuelve el número de nodos, sin leerlos ni contar I/Os. */
    std::size_t cantidad_nodos() const {
        return nodos_.size();
    }

    /** Sin parámetros: devuelve una copia de los contadores de construcción. */
    ContadorIO metricas() const {
        return metricas_;
    }

    /**
     * Guarda todos los nodos en ruta, en orden y sin cabecera, un bloque por nodo.
     * La carpeta de destino debe existir. Reemplaza el archivo si ya existe.
     * Devuelve los contadores de esta exportación: una lectura de RAM y una
     * escritura a archivo por nodo, sin alterar las métricas de construcción.
     * Lanza runtime_error si falla abrir, escribir o cerrar el archivo.
     */
    ContadorIO guardar(const std::string& ruta) const {
        std::ofstream salida(ruta, std::ios::binary | std::ios::trunc);
        if (!salida) {
            throw std::runtime_error("No se pudo abrir el archivo para guardar: " + ruta);
        }
        ContadorIO exportacion;
        for (const Nodo& nodo : nodos_) {
            ++exportacion.lecturas;
            escribir_nodo(salida, nodo);
            ++exportacion.escrituras;
        }
        salida.close();
        if (!salida) {
            throw std::runtime_error("No se pudo cerrar el archivo guardado: " + ruta);
        }
        return exportacion;
    }

private:
    std::vector<Nodo> nodos_;
    ContadorIO metricas_;
};

/**
 * Lector de nodos desde un archivo binario, sin vector ni caché de nodos.
 * Nodo debe ser el mismo tipo con el que se guardó el archivo.
 * Mantiene abierto el archivo y cuenta una lectura por bloque recuperado;
 * son accesos lógicos del programa, no mediciones de la caché del sistema operativo.
 */
template <typename Nodo>
class ArchivoNodos {
public:
    /**
     * Abre ruta para lectura binaria y calcula su cantidad de nodos.
     * No lee nodos ni cuenta I/Os. Lanza runtime_error si no se puede abrir
     * o si su tamaño no corresponde a uno o más bloques completos.
     */
    explicit ArchivoNodos(const std::string& ruta)
        : entrada_(ruta, std::ios::binary | std::ios::ate) {
        if (!entrada_) {
            throw std::runtime_error("No se pudo abrir el archivo de nodos: " + ruta);
        }
        const std::streamoff bytes = entrada_.tellg();
        if (bytes <= 0 || bytes % static_cast<std::streamoff>(TAMANO_BLOQUE) != 0) {
            throw std::runtime_error("El archivo no contiene bloques completos: " + ruta);
        }
        cantidad_ = static_cast<std::size_t>(bytes / TAMANO_BLOQUE);
    }

    /**
     * Lee y devuelve el nodo en indice, desde indice * 4096 bytes.
     * Cuenta una lectura tras recuperar el bloque completo, incluso si ya se leyó.
     * Lanza out_of_range para índices inválidos o runtime_error ante un fallo de I/O.
     */
    Nodo leer(int indice) {
        if (indice < 0 || static_cast<std::size_t>(indice) >= cantidad_) {
            throw std::out_of_range("Indice de nodo fuera del archivo.");
        }
        // Convertir antes de multiplicar evita desbordar un int con archivos grandes.
        const std::streamoff posicion = static_cast<std::streamoff>(indice)
                                     * static_cast<std::streamoff>(TAMANO_BLOQUE);
        entrada_.clear();
        entrada_.seekg(posicion, std::ios::beg);
        Nodo nodo = leer_nodo<Nodo>(entrada_);
        ++metricas_.lecturas;
        return nodo;
    }

    /** Sin parámetros: devuelve el número de bloques del archivo, sin I/Os. */
    std::size_t cantidad_nodos() const {
        return cantidad_;
    }

    /** Sin parámetros: devuelve una copia de las métricas de lectura desde disco. */
    ContadorIO metricas() const {
        return metricas_;
    }

    /** Sin parámetros ni retorno: pone los contadores en cero para una nueva consulta. */
    void reiniciar_metricas() {
        metricas_ = {};
    }

private:
    std::ifstream entrada_;
    std::size_t cantidad_ = 0;
    ContadorIO metricas_;
};
