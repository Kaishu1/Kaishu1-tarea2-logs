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

    /** Inserta par, dividiendo antes de descender a un nodo lleno (sección 3.4.2). */
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
     * Inserta par en el nodo no lleno ya leído en indice. Escribe las modificaciones
     * y cuenta mediante el almacenamiento cada acceso a los descendientes.
     */
    void insertar_no_lleno(int indice, NodoB nodo, Registro par);

    /** Recorre desde indice los subárboles relevantes y agrega sus pares a resultado. */
    static void buscar_en_nodo(ArchivoNodos<NodoB>& archivo, int indice,
                              int l, int u, std::vector<Registro>& resultado);
};
