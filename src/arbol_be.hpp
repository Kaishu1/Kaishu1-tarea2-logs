#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

#include "almacenamiento.hpp"
#include "arbol.hpp"
#include "nodo_be.hpp"

/**
 * Árbol Bε parametrizado para las dos configuraciones de la tarea.
 * C indica el máximo de separadores y LMax el máximo de pares por buffer.
 * Los pares residen en hojas o buffers internos; los separadores son solo llaves.
 * Las inserciones usan RAM; las consultas estáticas usan exclusivamente disco.
 */
template <int C, int LMax>
class ArbolBE : public Arbol {
public:
    using Nodo = NodoBE<C, LMax>;
    static constexpr double alfa = 0.5;  // Acuerdo para ambas configuraciones.

    /** Crea una raíz hoja vacía en 0 y cuenta una escritura. */
    ArbolBE() = default;

    /** Inserta par por la raíz, con divisiones o vaciados si hacen falta; no devuelve valor. */
    void insertar(Registro par) override {
        Nodo raiz = almacenamiento_.leer(0);
        if (raiz.k_buffer == LMax) {
            if (!raiz.es_interno) {
                raiz = dividir_raiz(raiz);
            } else {
                vaciar(raiz);
                if (raiz.k_llaves == C) {
                    raiz = dividir_raiz(raiz);
                }
            }
        }
        mezclar(raiz, &par, 1);
        almacenamiento_.escribir(0, raiz);
    }

    /** Exporta a ruta sin vaciar buffers; requiere carpeta existente y devuelve I/Os propias. */
    ContadorIO guardar(const std::string& ruta) const override {
        return almacenamiento_.guardar(ruta);
    }

    /** Devuelve I/Os de construcción, incluida la escritura inicial de la raíz. */
    ContadorIO metricas() const override { return almacenamiento_.metricas(); }

    /** Devuelve la cantidad de nodos, sin acceder al vector ni contar I/Os. */
    std::size_t cantidad_nodos() const override { return almacenamiento_.cantidad_nodos(); }

    /**
     * Devuelve los pares de [l,u] ordenados, leyendo buffers de hojas e internos.
     * Requiere l <= u. Acumula lecturas en archivo, cuyo contador puede reiniciarse
     * fuera de esta función. No necesita una instancia de construcción en RAM.
     */
    static std::vector<Registro> buscar_rango(ArchivoNodos<Nodo>& archivo, int l, int u) {
        std::vector<Registro> resultado;
        buscar_en_nodo(archivo, 0, l, u, resultado);
        std::sort(resultado.begin(), resultado.end(),
                  [](Registro a, Registro b) { return a.llave < b.llave; });
        return resultado;
    }

private:
    AlmacenamientoRAM<Nodo> almacenamiento_;

    /** Resultado del split: izquierdo y derecho son las mitades; separador es la llave que sube. */
    struct Division {
        Nodo izquierdo;
        Nodo derecho;
        int separador = 0;
    };

    /** Busca llave en nodo; devuelve el índice del primer separador >= llave, o k_llaves. */
    static int posicion_hijo(const Nodo& nodo, int llave) {
        return static_cast<int>(std::lower_bound(nodo.llaves.begin(),
            nodo.llaves.begin() + nodo.k_llaves, llave) - nodo.llaves.begin());
    }

    /**
     * Mezcla cantidad pares ordenados de lote en el buffer de nodo, sin I/Os.
     * Requiere espacio suficiente y un lote que no sea parte del buffer destino.
     * Actualiza nodo y su k_buffer; no devuelve valor.
     */
    static void mezclar(Nodo& nodo, const Registro* lote, int cantidad) {
        int i = nodo.k_buffer - 1;
        int j = cantidad - 1;
        int destino = nodo.k_buffer + cantidad - 1;
        while (j >= 0) {
            if (i >= 0 && nodo.buffer[i].llave > lote[j].llave) {
                nodo.buffer[destino--] = nodo.buffer[i--];
            } else {
                nodo.buffer[destino--] = lote[j--];
            }
        }
        nodo.k_buffer += cantidad;
    }

    /** Quita cantidad pares desde inicio en nodo.buffer y actualiza k_buffer; no devuelve valor. */
    static void retirar(Nodo& nodo, int inicio, int cantidad) {
        for (int i = inicio + cantidad; i < nodo.k_buffer; ++i) {
            nodo.buffer[i - cantidad] = nodo.buffer[i];
        }
        nodo.k_buffer -= cantidad;
    }

    /**
     * Divide una hoja con LMax pares o un interno con C separadores (sección 3.4.3).
     * En hojas, conserva el mediano en la izquierda; en internos, promueve solo su
     * llave y distribuye el buffer por <= separador y > separador.
     * Devuelve las dos mitades y el separador, sin modificar nodo ni efectuar I/Os.
     */
    static Division dividir(const Nodo& nodo) {
        Division division;
        if (!nodo.es_interno) {
            constexpr int mitad = LMax / 2;
            division.separador = nodo.buffer[mitad - 1].llave;
            division.izquierdo.k_buffer = mitad;
            division.derecho.k_buffer = LMax - mitad;
            std::copy_n(nodo.buffer.begin(), mitad, division.izquierdo.buffer.begin());
            std::copy_n(nodo.buffer.begin() + mitad, LMax - mitad, division.derecho.buffer.begin());
        } else {
            constexpr int medio = C / 2 - 1;
            division.separador = nodo.llaves[medio];
            division.izquierdo.es_interno = division.derecho.es_interno = 1;
            division.izquierdo.k_llaves = medio;
            division.derecho.k_llaves = C - medio - 1;
            std::copy_n(nodo.llaves.begin(), medio, division.izquierdo.llaves.begin());
            std::copy_n(nodo.llaves.begin() + medio + 1, C - medio - 1,
                        division.derecho.llaves.begin());
            std::copy_n(nodo.hijos.begin(), medio + 1, division.izquierdo.hijos.begin());
            std::copy_n(nodo.hijos.begin() + medio + 1, C - medio, division.derecho.hijos.begin());
            for (int i = 0; i < nodo.k_buffer; ++i) {
                Nodo& destino = nodo.buffer[i].llave <= division.separador
                              ? division.izquierdo : division.derecho;
                destino.buffer[destino.k_buffer++] = nodo.buffer[i];
            }
        }
        return division;
    }

    /**
     * Agrega ambas mitades de raiz al vector y devuelve una raíz interna con buffer vacío.
     * El llamador inserta el nuevo par y escribe la raíz en 0; cuenta cada agregado.
     */
    Nodo dividir_raiz(const Nodo& raiz) {
        const Division division = dividir(raiz);
        Nodo nueva;
        nueva.es_interno = 1;
        nueva.k_llaves = 1;
        nueva.llaves[0] = division.separador;
        nueva.hijos[0] = almacenamiento_.agregar(division.izquierdo);
        nueva.hijos[1] = almacenamiento_.agregar(division.derecho);
        return nueva;
    }

    /**
     * Divide hijo, ubicado en padre.hijos[posicion], y guarda las dos mitades.
     * Actualiza padre con el separador y el nuevo hijo; requiere menos de C llaves.
     * El llamador guarda padre o propaga su división. Cuenta escrituras y no devuelve valor.
     */
    void dividir_hijo(Nodo& padre, int posicion, const Nodo& hijo) {
        const Division division = dividir(hijo);
        const int derecho = almacenamiento_.agregar(division.derecho);
        almacenamiento_.escribir(padre.hijos[posicion], division.izquierdo);
        for (int j = padre.k_llaves; j > posicion; --j) {
            padre.llaves[j] = padre.llaves[j - 1];
            padre.hijos[j + 1] = padre.hijos[j];
        }
        padre.llaves[posicion] = division.separador;
        padre.hijos[posicion + 1] = derecho;
        ++padre.k_llaves;
    }

    /**
     * Transfiere el grupo seleccionado en [inicio,inicio+cantidad) del buffer padre.
     * Completa hojas parcialmente llenas antes de dividirlas. Tras cada split vuelve
     * a ubicar los pares restantes por sus llaves. Se detiene si padre alcanza C:
     * los pares aún no transferidos permanecen en su buffer para la continuación.
     * Modifica padre local y escribe cada hijo actualizado mediante almacenamiento.
     * No devuelve valor.
     */
    void transferir_grupo(Nodo& padre, int inicio, int cantidad) {
        while (cantidad > 0 && padre.k_llaves < C) {
            const int posicion = posicion_hijo(padre, padre.buffer[inicio].llave);
            int lote = cantidad;
            if (posicion < padre.k_llaves) {
                lote = 0;
                while (lote < cantidad
                       && padre.buffer[inicio + lote].llave <= padre.llaves[posicion]) {
                    ++lote;
                }
            }
            const int indice = padre.hijos[posicion];
            Nodo hijo = almacenamiento_.leer(indice);
            if (hijo.es_interno) {
                while (hijo.k_buffer + lote > LMax && hijo.k_llaves < C) {
                    vaciar(hijo);
                }
                if (hijo.k_llaves == C) {
                    dividir_hijo(padre, posicion, hijo);
                    continue;
                }
            }
            const int enviados = std::min(lote, LMax - hijo.k_buffer);
            mezclar(hijo, padre.buffer.data() + inicio, enviados);
            retirar(padre, inicio, enviados);
            cantidad -= enviados;
            if (enviados < lote) {
                // Solo puede ocurrir en una hoja, ahora llena: repetir si el lote
                // restante tampoco cabe tras esta división, sin ampliar el bloque.
                dividir_hijo(padre, posicion, hijo);
            } else {
                almacenamiento_.escribir(indice, hijo);
            }
        }
    }

    /**
     * Vacía grupos del buffer de nodo, un interno no vacío con menos de C separadores.
     * Procesa de derecha a izquierda los grupos >= ceil(ceil(k_buffer/(k_llaves+1))*alfa).
     * Si alcanza C separadores, se detiene para dividir; deja los pares pendientes en el buffer.
     * Modifica nodo y escribe sus hijos; el llamador guarda nodo. No devuelve valor.
     */
    void vaciar(Nodo& nodo) {
        const int hijos = nodo.k_llaves + 1;
        const int f = (nodo.k_buffer + hijos - 1) / hijos;
        const int umbral = static_cast<int>(std::ceil(f * alfa));
        std::array<int, C + 2> limites{};
        int cursor = 0;
        for (int i = 0; i < nodo.k_llaves; ++i) {
            while (cursor < nodo.k_buffer && nodo.buffer[cursor].llave <= nodo.llaves[i]) {
                ++cursor;
            }
            limites[i + 1] = cursor;
        }
        limites[hijos] = nodo.k_buffer;
        for (int i = hijos - 1; i >= 0; --i) {
            const int cantidad = limites[i + 1] - limites[i];
            if (cantidad >= umbral) {
                transferir_grupo(nodo, limites[i], cantidad);
                if (nodo.k_llaves == C) {
                    return;
                }
            }
        }
    }

    /**
     * Lee desde indice en archivo y busca en el rango inclusivo [l, u].
     * Agrega a resultado los pares de buffers y hojas; cuenta lecturas y no devuelve valor.
     */
    static void buscar_en_nodo(ArchivoNodos<Nodo>& archivo, int indice,
                              int l, int u, std::vector<Registro>& resultado) {
        const Nodo nodo = archivo.leer(indice);
        auto par = std::lower_bound(nodo.buffer.begin(), nodo.buffer.begin() + nodo.k_buffer,
            l, [](Registro registro, int llave) { return registro.llave < llave; });
        while (par != nodo.buffer.begin() + nodo.k_buffer && par->llave <= u) {
            resultado.push_back(*par++);
        }
        if (nodo.es_interno) {
            int i = posicion_hijo(nodo, l);
            buscar_en_nodo(archivo, nodo.hijos[i], l, u, resultado);
            while (i < nodo.k_llaves && u > nodo.llaves[i]) {
                buscar_en_nodo(archivo, nodo.hijos[++i], l, u, resultado);
            }
        }
    }
};

using ArbolBE025 = ArbolBE<4, 506>;
using ArbolBE075 = ArbolBE<126, 384>;
