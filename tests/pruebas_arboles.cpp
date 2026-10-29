#include <algorithm>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <limits>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

#include "arbol_b.hpp"

namespace {

/** Lanza runtime_error con mensaje cuando condicion es falsa; no modifica datos. */
void exigir(bool condicion, const std::string& mensaje) {
    if (!condicion) {
        throw std::runtime_error(mensaje);
    }
}

/** Directorio temporal exclusivo; evita modificar datos o árboles de la usuaria. */
struct DirectorioTemporal {
    std::filesystem::path ruta;

    /** Crea un directorio nuevo para los binarios de prueba, sin parámetros. */
    DirectorioTemporal() {
        const auto marca = std::chrono::steady_clock::now().time_since_epoch().count();
        ruta = std::filesystem::temp_directory_path()
             / ("tarea2-arboles-" + std::to_string(marca));
        exigir(std::filesystem::create_directory(ruta), "No se pudo crear directorio temporal.");
    }

    /** Elimina únicamente el directorio de esta prueba, incluso al lanzar excepciones. */
    ~DirectorioTemporal() {
        std::error_code error;
        std::filesystem::remove_all(ruta, error);
    }
};

/** Comprueba igualdad de llaves, valores, orden y cantidad, sin modificar los vectores. */
void comparar(const std::vector<Registro>& obtenido, const std::vector<Registro>& esperado) {
    exigir(obtenido.size() == esperado.size(), "Cantidad incorrecta de pares en consulta.");
    for (std::size_t i = 0; i < esperado.size(); ++i) {
        exigir(obtenido[i].llave == esperado[i].llave && obtenido[i].valor == esperado[i].valor,
               "La consulta perdio, duplico o cambio un par.");
    }
}

/**
 * Verifica recursivamente ocupación, orden, rangos (inferior, superior], enlaces
 * únicos y profundidad uniforme de hojas. Marca visitados, actualiza profundidad_hoja
 * (inicialmente -1) y devuelve el total de pares del subárbol desde indice.
 */
std::size_t verificar_nodo(ArchivoNodos<NodoB>& archivo, int indice,
                           std::int64_t inferior, std::int64_t superior,
                           int profundidad, int& profundidad_hoja,
                           std::vector<bool>& visitados) {
    exigir(indice >= 0 && static_cast<std::size_t>(indice) < visitados.size(), "Hijo inexistente.");
    exigir(!visitados[indice], "Ciclo o nodo compartido por dos padres.");
    visitados[indice] = true;
    const NodoB nodo = archivo.leer(indice);
    exigir(nodo.es_interno == 0 || nodo.es_interno == 1, "Tipo de nodo incorrecto.");
    exigir(nodo.padding == 0, "Padding distinto de cero.");
    const int minimo = indice == 0 ? (nodo.es_interno ? 1 : 0) : NodoB::b / 2 - 1;
    exigir(nodo.k >= minimo && nodo.k <= NodoB::b, "Ocupacion fuera de los limites.");
    for (int i = 0; i < nodo.k; ++i) {
        const int llave = nodo.llaves_valores[i].llave;
        exigir(llave > inferior && llave <= superior, "Par fuera del rango de su subarbol.");
        if (i > 0) {
            exigir(nodo.llaves_valores[i - 1].llave <= llave, "Pares desordenados.");
        }
    }
    std::size_t pares = nodo.k;
    if (!nodo.es_interno) {
        if (profundidad_hoja < 0) {
            profundidad_hoja = profundidad;
        }
        exigir(profundidad == profundidad_hoja, "Hojas a distintas profundidades.");
    } else {
        for (int i = 0; i <= nodo.k; ++i) {
            const std::int64_t desde = i == 0 ? inferior : nodo.llaves_valores[i - 1].llave;
            const std::int64_t hasta = i == nodo.k ? superior : nodo.llaves_valores[i].llave;
            pares += verificar_nodo(archivo, nodo.hijos[i], desde, hasta,
                                    profundidad + 1, profundidad_hoja, visitados);
        }
    }
    return pares;
}

/** Compara [l,u] con filtrado directo de entrada; comprueba I/Os de solo lectura. */
void verificar_consulta(ArchivoNodos<NodoB>& archivo, const std::vector<Registro>& entrada,
                        int l, int u) {
    std::vector<Registro> esperado;
    for (Registro par : entrada) {
        if (par.llave >= l && par.llave <= u) {
            esperado.push_back(par);
        }
    }
    std::sort(esperado.begin(), esperado.end(),
              [](Registro a, Registro b) { return a.llave < b.llave; });
    archivo.reiniciar_metricas();
    comparar(ArbolB::buscar_rango(archivo, l, u), esperado);
    exigir(archivo.metricas().lecturas >= 1
           && archivo.metricas().lecturas <= archivo.cantidad_nodos()
           && archivo.metricas().escrituras == 0, "Conteo de consulta incorrecto.");
}

/**
 * Construye entrada, guarda en ruta y destruye el árbol RAM. Verifica después
 * estructura y consultas solo desde disco. profundidad_minima exige que el caso
 * alcance las divisiones previstas (0 hoja, 1 raíz interna, 2 internos divididos).
 */
void probar_caso(const std::filesystem::path& ruta, const std::vector<Registro>& entrada,
                 int profundidad_minima) {
    std::size_t nodos = 0;
    {
        ArbolB arbol;
        Arbol& interfaz = arbol;
        for (Registro par : entrada) {
            interfaz.insertar(par);
        }
        nodos = interfaz.cantidad_nodos();
        const ContadorIO antes = interfaz.metricas();
        const ContadorIO exportacion = interfaz.guardar(ruta.string());
        exigir(exportacion.lecturas == nodos && exportacion.escrituras == nodos,
               "Exportacion incompleta.");
        exigir(interfaz.metricas().lecturas == antes.lecturas
               && interfaz.metricas().escrituras == antes.escrituras,
               "Exportar altero las metricas de construccion.");
        if (entrada.size() <= 340) {
            exigir(antes.lecturas == entrada.size() && antes.escrituras == entrada.size() + 1,
                   "I/Os incorrectas para inserciones en una hoja.");
        } else if (entrada.size() == 341) {
            exigir(antes.lecturas == 341 && antes.escrituras == 345,
                   "I/Os incorrectas en la primera division de raiz.");
        }
    }
    exigir(std::filesystem::file_size(ruta) == nodos * TAMANO_BLOQUE, "Tamano de archivo incorrecto.");
    ArchivoNodos<NodoB> archivo(ruta.string());
    std::vector<bool> visitados(nodos, false);
    int profundidad_hoja = -1;
    const int minimo = std::numeric_limits<int>::min();
    const int maximo = std::numeric_limits<int>::max();
    const auto pares = verificar_nodo(archivo, 0, static_cast<std::int64_t>(minimo) - 1,
                                     maximo, 0, profundidad_hoja, visitados);
    exigir(pares == entrada.size(), "La estructura no conserva todos los pares.");
    exigir(std::all_of(visitados.begin(), visitados.end(), [](bool v) { return v; }),
           "Hay nodos inaccesibles desde la raiz.");
    exigir(profundidad_hoja >= profundidad_minima, "El caso no alcanzo la altura esperada.");

    verificar_consulta(archivo, entrada, minimo, maximo);
    exigir(archivo.metricas().lecturas == nodos, "El rango completo no leyo cada nodo una vez.");
    verificar_consulta(archivo, entrada, minimo, minimo);
    verificar_consulta(archivo, entrada, maximo, maximo);
    verificar_consulta(archivo, entrada, 1, 1);  // Hueco entre llaves pares.

    const NodoB raiz = archivo.leer(0);
    if (entrada.size() == 341) {
        exigir(raiz.k == 1 && raiz.llaves_valores[0].llave == 338,
               "No se promovio el par mediano de la posicion 170.");
        exigir(archivo.leer(raiz.hijos[0]).k == 169 && archivo.leer(raiz.hijos[1]).k == 171,
               "Reparto incorrecto en el split de la raiz (incluida la nueva insercion).");
    }
    if (raiz.es_interno) {
        const int separador = raiz.llaves_valores[0].llave;
        verificar_consulta(archivo, entrada, separador, separador);
        exigir(archivo.metricas().lecturas == static_cast<std::uint64_t>(profundidad_hoja + 1),
               "La busqueda puntual visito ramas que no corresponden.");
        verificar_consulta(archivo, entrada, separador - 1, separador);
        verificar_consulta(archivo, entrada, separador, separador + 1);
    }
    std::mt19937 generador(4102);
    std::uniform_int_distribution<int> limites(-10, static_cast<int>(entrada.size()) * 2 + 10);
    for (int i = 0; i < 30; ++i) {
        int l = limites(generador);
        int u = limites(generador);
        if (l > u) {
            std::swap(l, u);
        }
        verificar_consulta(archivo, entrada, l, u);
    }
    std::cout << ruta.filename().string() << ": OK (" << entrada.size() << " pares, "
              << nodos << " nodos, " << profundidad_hoja + 1 << " niveles)\n";
}

/** Devuelve cantidad de pares con llaves distintas pares y valores reconocibles. */
std::vector<Registro> crear_datos(int cantidad) {
    std::vector<Registro> datos;
    for (int i = 0; i < cantidad; ++i) {
        datos.push_back({2 * i, static_cast<float>(i % 101) * 0.25F - 12.5F});
    }
    return datos;
}

}  // namespace

/** Ejecuta pruebas deterministas del árbol B; devuelve 0 si pasan o 1 si fallan. */
int main() {
    try {
        DirectorioTemporal temporal;
        for (int cantidad : {0, 1, 340, 341, 510, 511}) {
            probar_caso(temporal.ruta / ("b-" + std::to_string(cantidad) + ".bin"),
                        crear_datos(cantidad), cantidad > 340 ? 1 : 0);
        }
        probar_caso(temporal.ruta / "extremos.bin",
                    {{std::numeric_limits<int>::max(), 1.0F}, {0, 2.0F},
                     {std::numeric_limits<int>::min(), 3.0F}}, 0);
        auto datos = crear_datos(100000);
        probar_caso(temporal.ruta / "ascendente.bin", datos, 2);
        std::reverse(datos.begin(), datos.end());
        probar_caso(temporal.ruta / "descendente.bin", datos, 2);
        std::mt19937 generador(4102);
        std::shuffle(datos.begin(), datos.end(), generador);
        probar_caso(temporal.ruta / "desordenado.bin", datos, 2);
    } catch (const std::exception& error) {
        std::cerr << "Error en pruebas de arboles: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
