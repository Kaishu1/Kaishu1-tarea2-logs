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
#include "arbol_be.hpp"

namespace {

/** Comprueba condicion y lanza runtime_error con mensaje si falla; no devuelve valor. */
void exigir(bool condicion, const std::string& mensaje) {
    if (!condicion) {
        throw std::runtime_error(mensaje);
    }
}

/** Guarda en ruta el directorio temporal de pruebas, separado de los datos originales. */
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

/**
 * Compara obtenido con esperado, incluidos valores y orden.
 * No devuelve valor; lanza un error si encuentra diferencias.
 */
void comparar(const std::vector<Registro>& obtenido, const std::vector<Registro>& esperado) {
    exigir(obtenido.size() == esperado.size(), "Cantidad incorrecta de pares en consulta.");
    for (std::size_t i = 0; i < esperado.size(); ++i) {
        exigir(obtenido[i].llave == esperado[i].llave && obtenido[i].valor == esperado[i].valor,
               "La consulta perdio, duplico o cambio un par.");
    }
}

/**
 * Revisa el subárbol de archivo desde indice, a la profundidad indicada.
 * inferior y superior delimitan su rango (inferior, superior].
 * Marca visitados y actualiza profundidad_hoja (inicialmente -1).
 * Devuelve el total de pares; lanza un error si fallan ocupación, orden, rangos o enlaces.
 * También comprueba que todas las hojas tengan la misma profundidad.
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

/**
 * Busca [l,u] en archivo y compara con el filtrado de entrada; comprueba las lecturas.
 * No devuelve valor; lanza un error si falla una comprobación.
 */
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
 * No devuelve valor; muestra el resultado y lanza un error si falla una comprobación.
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

/**
 * Revisa el subárbol Bε de archivo desde indice, a la profundidad indicada.
 * inferior y superior delimitan su rango (inferior, superior]; Nodo indica el formato.
 * Marca visitados, actualiza profundidad_hoja (inicialmente -1) y agrega a muestras
 * una llave por buffer interno no vacío. Devuelve la cantidad de pares de datos.
 * Lanza un error si fallan capacidades, orden, rangos, enlaces o profundidad de hojas.
 */
template <typename Nodo>
std::size_t verificar_nodo_be(ArchivoNodos<Nodo>& archivo, int indice,
                            std::int64_t inferior, std::int64_t superior,
                            int profundidad, int& profundidad_hoja,
                            std::vector<bool>& visitados, std::vector<int>& muestras) {
    exigir(indice >= 0 && static_cast<std::size_t>(indice) < visitados.size(), "Hijo BE inexistente.");
    exigir(!visitados[indice], "Ciclo o hijo BE compartido.");
    visitados[indice] = true;
    const Nodo nodo = archivo.leer(indice);
    exigir(nodo.es_interno == 0 || nodo.es_interno == 1, "Tipo BE incorrecto.");
    exigir(nodo.k_buffer >= 0 && nodo.k_buffer <= Nodo::lmax, "Buffer BE fuera de capacidad.");
    for (int i = 0; i < nodo.k_buffer; ++i) {
        exigir(nodo.buffer[i].llave > inferior && nodo.buffer[i].llave <= superior,
               "Dato BE fuera del rango de su subarbol.");
        if (i > 0) {
            exigir(nodo.buffer[i - 1].llave <= nodo.buffer[i].llave, "Buffer BE desordenado.");
        }
    }
    std::size_t pares = nodo.k_buffer;
    if (!nodo.es_interno) {
        exigir(nodo.k_llaves == 0, "Hoja BE con separadores.");
        exigir(indice == 0 || nodo.k_buffer >= Nodo::lmax / 2, "Hoja BE bajo media capacidad.");
        if (profundidad_hoja < 0) {
            profundidad_hoja = profundidad;
        }
        exigir(profundidad == profundidad_hoja, "Hojas BE a distintas profundidades.");
    } else {
        const int minimo = indice == 0 ? 1 : Nodo::c / 2 - 1;
        exigir(nodo.k_llaves >= minimo && nodo.k_llaves < Nodo::c,
               "Ocupacion interna BE incorrecta o division no propagada.");
        for (int i = 0; i < nodo.k_llaves; ++i) {
            exigir(nodo.llaves[i] > inferior && nodo.llaves[i] <= superior,
                   "Separador BE fuera de rango.");
            if (i > 0) {
                exigir(nodo.llaves[i - 1] < nodo.llaves[i], "Separadores BE desordenados.");
            }
        }
        if (nodo.k_buffer > 0) {
            muestras.push_back(nodo.buffer[0].llave);
        }
        for (int i = 0; i <= nodo.k_llaves; ++i) {
            const std::int64_t desde = i == 0 ? inferior : nodo.llaves[i - 1];
            const std::int64_t hasta = i == nodo.k_llaves ? superior : nodo.llaves[i];
            pares += verificar_nodo_be(archivo, nodo.hijos[i], desde, hasta,
                                       profundidad + 1, profundidad_hoja, visitados, muestras);
        }
    }
    return pares;
}

/**
 * Consulta TipoArbol desde archivo en [l,u] y compara con el filtrado de entrada.
 * Comprueba que solo haya lecturas; no devuelve valor y lanza un error si falla.
 */
template <typename TipoArbol>
void verificar_consulta_be(ArchivoNodos<typename TipoArbol::Nodo>& archivo,
                           const std::vector<Registro>& entrada, int l, int u) {
    std::vector<Registro> esperado;
    for (Registro par : entrada) {
        if (par.llave >= l && par.llave <= u) {
            esperado.push_back(par);
        }
    }
    std::sort(esperado.begin(), esperado.end(),
              [](Registro a, Registro b) { return a.llave < b.llave; });
    archivo.reiniciar_metricas();
    comparar(TipoArbol::buscar_rango(archivo, l, u), esperado);
    exigir(archivo.metricas().lecturas >= 1 && archivo.metricas().lecturas <= archivo.cantidad_nodos()
           && archivo.metricas().escrituras == 0, "Conteo incorrecto en consulta BE.");
}

/**
 * Inserta entrada en TipoArbol, lo guarda en ruta y lo comprueba después desde disco.
 * profundidad_minima es la profundidad que debe alcanzar (0 corresponde a la raíz).
 * No devuelve valor; muestra el resultado y lanza un error si falla una comprobación.
 */
template <typename TipoArbol>
void probar_caso_be(const std::filesystem::path& ruta, const std::vector<Registro>& entrada,
                    int profundidad_minima) {
    using Nodo = typename TipoArbol::Nodo;
    std::size_t nodos = 0;
    {
        TipoArbol arbol;
        Arbol& interfaz = arbol;
        for (Registro par : entrada) {
            interfaz.insertar(par);
        }
        const auto antes = interfaz.metricas();
        nodos = interfaz.cantidad_nodos();
        const auto exportacion = interfaz.guardar(ruta.string());
        exigir(exportacion.lecturas == nodos && exportacion.escrituras == nodos,
               "Exportacion BE incompleta.");
        exigir(interfaz.metricas().lecturas == antes.lecturas
               && interfaz.metricas().escrituras == antes.escrituras, "Exportar altero I/Os BE.");
        if (entrada.size() <= Nodo::lmax) {
            exigir(antes.lecturas == entrada.size() && antes.escrituras == entrada.size() + 1,
                   "I/Os BE incorrectas antes del primer split.");
        } else if (entrada.size() <= 2 * Nodo::lmax) {
            exigir(antes.lecturas == entrada.size() && antes.escrituras == entrada.size() + 3,
                   "Insercion en buffer raiz hizo I/Os innecesarias.");
        }
    }
    exigir(std::filesystem::file_size(ruta) == nodos * TAMANO_BLOQUE, "Tamano BE incorrecto.");
    ArchivoNodos<Nodo> archivo(ruta.string());
    std::vector<bool> visitados(nodos, false);
    std::vector<int> muestras;
    int profundidad_hoja = -1;
    constexpr int minimo = std::numeric_limits<int>::min();
    constexpr int maximo = std::numeric_limits<int>::max();
    exigir(verificar_nodo_be(archivo, 0, static_cast<std::int64_t>(minimo) - 1, maximo,
                            0, profundidad_hoja, visitados, muestras) == entrada.size(),
           "Perdida o duplicacion de datos BE.");
    exigir(std::all_of(visitados.begin(), visitados.end(), [](bool v) { return v; }),
           "Nodo BE inaccesible.");
    exigir(profundidad_hoja >= profundidad_minima, "El caso BE no alcanzo la altura prevista.");
    verificar_consulta_be<TipoArbol>(archivo, entrada, minimo, maximo);
    exigir(archivo.metricas().lecturas == nodos, "Consulta BE completa no leyo cada nodo una vez.");
    for (int limite : {minimo, maximo, 1}) {
        verificar_consulta_be<TipoArbol>(archivo, entrada, limite, limite);
    }
    for (std::size_t i = 0; i < std::min<std::size_t>(muestras.size(), 12); ++i) {
        verificar_consulta_be<TipoArbol>(archivo, entrada, muestras[i], muestras[i]);
        exigir(archivo.metricas().lecturas == static_cast<std::uint64_t>(profundidad_hoja + 1),
               "Consulta BE puntual no podo las ramas.");
    }
    const Nodo raiz = archivo.leer(0);
    if (raiz.es_interno) {
        const int separador = raiz.llaves[0];
        verificar_consulta_be<TipoArbol>(archivo, entrada, separador, separador);
        verificar_consulta_be<TipoArbol>(archivo, entrada, separador - 1, separador + 1);
    }
    std::mt19937 generador(4102);
    std::uniform_int_distribution<int> limites(-10, static_cast<int>(entrada.size()) * 2 + 10);
    for (int i = 0; i < 30; ++i) {
        int l = limites(generador);
        int u = limites(generador);
        if (l > u) {
            std::swap(l, u);
        }
        verificar_consulta_be<TipoArbol>(archivo, entrada, l, u);
    }
    std::cout << ruta.filename().string() << ": OK (" << entrada.size() << " pares, "
              << nodos << " nodos, " << profundidad_hoja + 1 << " niveles)\n";
}

/**
 * Crea un TipoArbol en ruta con grupos a ambos lados del primer separador.
 * bajo_umbral elige si el grupo menor queda justo bajo el umbral o igual a él.
 * Comprueba qué grupos se transfieren, el split y la conservación de los pares.
 * No devuelve valor; lanza un error si falla y muestra el resultado si pasa.
 */
template <typename TipoArbol>
void probar_umbral_be(const std::filesystem::path& ruta, bool bajo_umbral) {
    using Nodo = typename TipoArbol::Nodo;
    constexpr int lmax = Nodo::lmax;
    const int umbral = ((lmax + 1) / 2 + 1) / 2;  // ceil(ceil(lmax/2)*0.5).
    const int izquierdos = umbral - (bajo_umbral ? 1 : 0);
    std::vector<Registro> entrada;
    {
        TipoArbol arbol;
        for (int i = 0; i < lmax; ++i) {
            entrada.push_back({100 * i, 1.25F});
            arbol.insertar(entrada.back());
        }
        for (int i = 0; i < lmax; ++i) {
            const int llave = i < izquierdos ? 100 * i + 1 : 100 * lmax + 2 * i;
            entrada.push_back({llave, 2.5F});
            arbol.insertar(entrada.back());
        }
        entrada.push_back({200 * lmax, 3.75F});
        arbol.insertar(entrada.back());  // Dispara el flush del buffer raíz lleno.
        arbol.guardar(ruta.string());
    }
    ArchivoNodos<Nodo> archivo(ruta.string());
    const Nodo raiz = archivo.leer(0);
    exigir(raiz.es_interno && raiz.k_llaves == 2, "No se dividio la hoja BE llena por el lote.");
    exigir(raiz.k_buffer == (bajo_umbral ? izquierdos : 0) + 1, "Umbral alpha mal aplicado.");
    const Nodo izquierda = archivo.leer(raiz.hijos[0]);
    exigir(izquierda.k_buffer == lmax / 2 + (bajo_umbral ? 0 : izquierdos),
           "Se transfirio un grupo bajo el umbral o se omitio uno elegible.");
    verificar_consulta_be<TipoArbol>(archivo, entrada,
                                    std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
    std::cout << ruta.filename().string() << ": OK (seleccion de grupos con alpha=0.5)\n";
}

/**
 * Prueba la configuración TipoArbol; guarda archivos en directorio con el prefijo nombre.
 * Ejecuta los mismos casos y semillas para ambas variantes; no devuelve valor.
 */
template <typename TipoArbol>
void probar_configuracion_be(const std::filesystem::path& directorio, const std::string& nombre) {
    using Nodo = typename TipoArbol::Nodo;
    constexpr int lmax = Nodo::lmax;
    for (int cantidad : {0, 1, lmax, lmax + 1, 2 * lmax, 2 * lmax + 1, 4 * lmax + 1}) {
        probar_caso_be<TipoArbol>(directorio / (nombre + "-" + std::to_string(cantidad) + ".bin"),
                                 crear_datos(cantidad), cantidad > lmax ? 1 : 0);
    }
    probar_caso_be<TipoArbol>(directorio / (nombre + "-extremos.bin"),
        {{std::numeric_limits<int>::max(), 1.0F}, {0, 2.0F},
         {std::numeric_limits<int>::min(), 3.0F}}, 0);
    probar_umbral_be<TipoArbol>(directorio / (nombre + "-bajo-umbral.bin"), true);
    probar_umbral_be<TipoArbol>(directorio / (nombre + "-en-umbral.bin"), false);
    auto datos = crear_datos(100000);
    probar_caso_be<TipoArbol>(directorio / (nombre + "-ascendente.bin"), datos, 2);
    std::reverse(datos.begin(), datos.end());
    probar_caso_be<TipoArbol>(directorio / (nombre + "-descendente.bin"), datos, 2);
    std::mt19937 generador(4102);
    std::shuffle(datos.begin(), datos.end(), generador);
    probar_caso_be<TipoArbol>(directorio / (nombre + "-desordenado.bin"), datos, 2);
}

}  // namespace

/** Sin parámetros: ejecuta las pruebas de los tres árboles; devuelve 0 si pasan o 1 si fallan. */
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
        probar_configuracion_be<ArbolBE025>(temporal.ruta, "be025");
        probar_configuracion_be<ArbolBE075>(temporal.ruta, "be075");
    } catch (const std::exception& error) {
        std::cerr << "Error en pruebas de arboles: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
