#include "arbol_b.hpp"

#include <algorithm>

namespace {

/** Dos mitades de un nodo lleno y el par que se promueve; no efectúa I/Os. */
struct DivisionB {
    NodoB izquierdo;
    NodoB derecho;
    Registro mediano;
};

/**
 * Divide nodo (k == 340) según 3.4.1: promueve la posición 170 contando desde 1.
 * Devuelve 169 pares a la izquierda y 170 a la derecha; si es interno, reparte
 * 170 y 171 hijos. El par promovido se retira de ambas mitades.
 */
DivisionB dividir(const NodoB& nodo) {
    constexpr int medio = NodoB::b / 2 - 1;
    DivisionB division;
    division.mediano = nodo.llaves_valores[medio];
    division.izquierdo.es_interno = nodo.es_interno;
    division.derecho.es_interno = nodo.es_interno;
    division.izquierdo.k = medio;
    division.derecho.k = NodoB::b - medio - 1;
    std::copy_n(nodo.llaves_valores.begin(), medio,
                division.izquierdo.llaves_valores.begin());
    std::copy_n(nodo.llaves_valores.begin() + medio + 1, division.derecho.k,
                division.derecho.llaves_valores.begin());
    if (nodo.es_interno) {
        std::copy_n(nodo.hijos.begin(), medio + 1, division.izquierdo.hijos.begin());
        std::copy_n(nodo.hijos.begin() + medio + 1, division.derecho.k + 1,
                    division.derecho.hijos.begin());
    }
    return division;
}

/** Devuelve la primera posición con llave >= llave, o k; no modifica nodo. */
int posicion(const NodoB& nodo, int llave) {
    const auto inicio = nodo.llaves_valores.begin();
    return static_cast<int>(std::lower_bound(
        inicio, inicio + nodo.k, llave,
        [](const Registro& par, int buscada) { return par.llave < buscada; }) - inicio);
}

}  // namespace

void ArbolB::insertar(Registro par) {
    NodoB raiz = almacenamiento_.leer(0);
    if (raiz.k < NodoB::b) {
        insertar_no_lleno(0, raiz, par);
        return;
    }

    DivisionB division = dividir(raiz);
    const int izquierdo = almacenamiento_.agregar(division.izquierdo);
    const int derecho = almacenamiento_.agregar(division.derecho);
    NodoB nueva_raiz;
    nueva_raiz.es_interno = 1;
    nueva_raiz.k = 1;
    nueva_raiz.llaves_valores[0] = division.mediano;
    nueva_raiz.hijos[0] = izquierdo;
    nueva_raiz.hijos[1] = derecho;
    almacenamiento_.escribir(0, nueva_raiz);
    if (par.llave <= division.mediano.llave) {
        insertar_no_lleno(izquierdo, division.izquierdo, par);
    } else {
        insertar_no_lleno(derecho, division.derecho, par);
    }
}

void ArbolB::insertar_no_lleno(int indice, NodoB nodo, Registro par) {
    const int i = posicion(nodo, par.llave);
    if (!nodo.es_interno) {
        for (int j = nodo.k; j > i; --j) {
            nodo.llaves_valores[j] = nodo.llaves_valores[j - 1];
        }
        nodo.llaves_valores[i] = par;
        ++nodo.k;
        almacenamiento_.escribir(indice, nodo);
        return;
    }

    int indice_hijo = nodo.hijos[i];
    NodoB hijo = almacenamiento_.leer(indice_hijo);
    if (hijo.k == NodoB::b) {
        DivisionB division = dividir(hijo);
        const int derecho = almacenamiento_.agregar(division.derecho);
        almacenamiento_.escribir(indice_hijo, division.izquierdo);
        for (int j = nodo.k; j > i; --j) {
            nodo.llaves_valores[j] = nodo.llaves_valores[j - 1];
            nodo.hijos[j + 1] = nodo.hijos[j];
        }
        nodo.llaves_valores[i] = division.mediano;
        nodo.hijos[i + 1] = derecho;
        ++nodo.k;
        almacenamiento_.escribir(indice, nodo);
        if (par.llave <= division.mediano.llave) {
            hijo = division.izquierdo;
        } else {
            indice_hijo = derecho;
            hijo = division.derecho;
        }
    }
    insertar_no_lleno(indice_hijo, hijo, par);
}

ContadorIO ArbolB::guardar(const std::string& ruta) const {
    return almacenamiento_.guardar(ruta);
}

ContadorIO ArbolB::metricas() const {
    return almacenamiento_.metricas();
}

std::size_t ArbolB::cantidad_nodos() const {
    return almacenamiento_.cantidad_nodos();
}

std::vector<Registro> ArbolB::buscar_rango(ArchivoNodos<NodoB>& archivo, int l, int u) {
    std::vector<Registro> resultado;
    buscar_en_nodo(archivo, 0, l, u, resultado);
    return resultado;
}

void ArbolB::buscar_en_nodo(ArchivoNodos<NodoB>& archivo, int indice,
                          int l, int u, std::vector<Registro>& resultado) {
    const NodoB nodo = archivo.leer(indice);
    int i = posicion(nodo, l);
    if (nodo.es_interno) {
        buscar_en_nodo(archivo, nodo.hijos[i], l, u, resultado);
    }
    while (i < nodo.k && nodo.llaves_valores[i].llave <= u) {
        resultado.push_back(nodo.llaves_valores[i]);
        // El hijo derecho solo puede aportar si u es mayor que el separador.
        if (nodo.es_interno && u > nodo.llaves_valores[i].llave) {
            buscar_en_nodo(archivo, nodo.hijos[i + 1], l, u, resultado);
        }
        ++i;
    }
}
