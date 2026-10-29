#pragma once

#include <vector>

#include "almacenamiento.hpp"
#include "arbol.hpp"
#include "nodo_b.hpp"

/** Árbol B de la tarea: pares en hojas e internos, raíz en el índice cero. */
class ArbolB : public Arbol {
public:
    /** Crea una hoja raíz vacía en RAM; cuenta una escritura. */
    ArbolB() = default;

    /** Inserta par y divide antes de bajar a un nodo lleno; modifica el árbol sin devolver valor. */
    void insertar(Registro par) override;

    /** Guarda bloques consecutivos en ruta; su carpeta debe existir. Devuelve I/Os propias. */
    ContadorIO guardar(const std::string& ruta) const override;

    /** Devuelve las I/Os acumuladas de construcción, sin incluir exportaciones. */
    ContadorIO metricas() const override;

    /** Devuelve el número de nodos almacenados, sin I/Os. */
    std::size_t cantidad_nodos() const override;

    /**
     * Devuelve los pares de [l, u] ordenados por llave, leyendo solo desde archivo.
     * Requiere l <= u. Acumula las lecturas en archivo; el llamador puede reiniciar
     * sus métricas antes de cada consulta. No requiere una instancia en RAM.
     */
    static std::vector<Registro> buscar_rango(ArchivoNodos<NodoB>& archivo, int l, int u);

private:
    AlmacenamientoRAM<NodoB> almacenamiento_;

    /**
     * Recibe par y una copia del nodo no lleno ubicado en indice.
     * Inserta el par y escribe los cambios, contando los accesos. No devuelve valor.
     */
    void insertar_no_lleno(int indice, NodoB nodo, Registro par);

    /**
     * Lee desde indice en archivo y busca en el rango inclusivo [l, u].
     * Agrega los pares a resultado y acumula lecturas en archivo; no devuelve valor.
     */
    static void buscar_en_nodo(ArchivoNodos<NodoB>& archivo, int indice,
                              int l, int u, std::vector<Registro>& resultado);
};
