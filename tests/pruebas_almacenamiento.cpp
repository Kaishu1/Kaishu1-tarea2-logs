#include <cstddef>
#include <cstdint>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>

#include "almacenamiento.hpp"
#include "metricas.hpp"
#include "nodo_b.hpp"
#include "nodo_be.hpp"

// Comprobar el orden y los desplazamientos del formato del enunciado.
// Estas comprobaciones se ejecutan en compilación, incluso con NDEBUG.
static_assert(std::is_standard_layout_v<Registro>);
static_assert(std::is_standard_layout_v<NodoB>);
static_assert(std::is_standard_layout_v<NodoBE025>);
static_assert(std::is_standard_layout_v<NodoBE075>);

static_assert(offsetof(Registro, llave) == 0);
static_assert(offsetof(Registro, valor) == 4);

static_assert(offsetof(NodoB, es_interno) == 0);
static_assert(offsetof(NodoB, k) == 4);
static_assert(offsetof(NodoB, llaves_valores) == 8);
static_assert(offsetof(NodoB, hijos) == 2728);
static_assert(offsetof(NodoB, padding) == 4092);

static_assert(offsetof(NodoBE025, es_interno) == 0);
static_assert(offsetof(NodoBE025, k_llaves) == 4);
static_assert(offsetof(NodoBE025, k_buffer) == 8);
static_assert(offsetof(NodoBE025, llaves) == 12);
static_assert(offsetof(NodoBE025, hijos) == 28);
static_assert(offsetof(NodoBE025, buffer) == 48);

static_assert(offsetof(NodoBE075, es_interno) == 0);
static_assert(offsetof(NodoBE075, k_llaves) == 4);
static_assert(offsetof(NodoBE075, k_buffer) == 8);
static_assert(offsetof(NodoBE075, llaves) == 12);
static_assert(offsetof(NodoBE075, hijos) == 516);
static_assert(offsetof(NodoBE075, buffer) == 1024);

/** Comprueba condicion; no devuelve valor y lanza runtime_error con mensaje si falla. */
void exigir(bool condicion, const std::string& mensaje) {
    if (!condicion) {
        throw std::runtime_error(mensaje);
    }
}

/** Guarda en ruta el directorio temporal de pruebas y elimina sus archivos al terminar. */
struct DirectorioTemporal {
    std::filesystem::path ruta;

    /** Sin parámetros: crea un directorio temporal nuevo o lanza una excepción. */
    DirectorioTemporal() {
        const auto marca = std::chrono::steady_clock::now().time_since_epoch().count();
        ruta = std::filesystem::temp_directory_path()
             / ("tarea2-almacenamiento-" + std::to_string(marca));
        exigir(std::filesystem::create_directory(ruta), "No se pudo crear el directorio de prueba.");
    }

    /** Sin parámetros ni retorno: limpia el directorio propio, incluso si falla una prueba. */
    ~DirectorioTemporal() {
        std::error_code error;
        std::filesystem::remove_all(ruta, error);
    }
};

/**
 * Devuelve un Nodo con valores de prueba en sus campos, calculados a partir de base.
 * Los índices son datos de prueba: estos nodos no representan un árbol válido.
 * Permite detectar pérdida de campos o confusión entre bloques al serializar.
 */
template <typename Nodo>
Nodo crear_nodo(int base) {
    Nodo nodo;
    nodo.es_interno = 1;
    for (std::size_t i = 0; i < nodo.hijos.size(); ++i) {
        nodo.hijos[i] = base + static_cast<int>(i);
    }
    if constexpr (std::is_same_v<Nodo, NodoB>) {
        nodo.k = NodoB::b;
        for (int i = 0; i < NodoB::b; ++i) {
            nodo.llaves_valores[i] = {base + i, static_cast<float>(i) - 10.5F};
        }
    } else {
        nodo.k_llaves = Nodo::c;
        nodo.k_buffer = Nodo::lmax;
        for (int i = 0; i < Nodo::c; ++i) {
            nodo.llaves[i] = base + i;
        }
        for (int i = 0; i < Nodo::lmax; ++i) {
            nodo.buffer[i] = {base + i, static_cast<float>(i) - 10.5F};
        }
    }
    return nodo;
}

/**
 * Prueba RAM, contadores, exportación y lectura independiente para Nodo.
 * ruta: archivo temporal que puede reemplazarse. No devuelve valor;
 * lanza una excepción si falla una comprobación y escribe el resultado.
 */
template <typename Nodo>
void probar_almacenamiento(const std::filesystem::path& ruta) {
    const Nodo esperado0 = crear_nodo<Nodo>(1000);
    const Nodo esperado1 = crear_nodo<Nodo>(2000);
    const Nodo esperado2 = crear_nodo<Nodo>(3000);

    // Dejar un bloque extra para comprobar que guardar reemplaza el archivo.
    {
        std::ofstream salida(ruta, std::ios::binary);
        for (int i = 0; i < 4; ++i) {
            escribir_nodo(salida, esperado0);
        }
    }
    {
        AlmacenamientoRAM<Nodo> ram;
        exigir(ram.cantidad_nodos() == 1, "La raiz debe ocupar el primer nodo.");
        exigir(ram.metricas().lecturas == 0 && ram.metricas().escrituras == 1,
               "La creacion de la raiz debe contar una escritura.");
        Nodo copia = ram.leer(0);
        copia.es_interno = 1;
        exigir(copia.es_interno == 1 && ram.leer(0).es_interno == 0,
               "Modificar una copia no debe modificar el vector.");
        exigir(ram.agregar(esperado1) == 1 && ram.agregar(esperado2) == 2,
               "Los indices deben ser consecutivos.");
        ram.escribir(0, esperado0);
        copia = ram.leer(1);
        exigir(std::memcmp(&copia, &esperado1, sizeof(Nodo)) == 0,
               "El nodo agregado no se recupero correctamente.");

        bool error_lectura = false;
        bool error_escritura = false;
        try { ram.leer(-1); } catch (const std::out_of_range&) { error_lectura = true; }
        try { ram.escribir(3, esperado0); } catch (const std::out_of_range&) { error_escritura = true; }
        exigir(error_lectura && error_escritura, "Se acepto un indice inexistente en RAM.");
        exigir(ram.metricas().lecturas == 3 && ram.metricas().escrituras == 4,
               "Conteo incorrecto de operaciones en RAM.");

        const auto exportacion = ram.guardar(ruta.string());
        exigir(exportacion.lecturas == 3 && exportacion.escrituras == 3,
               "La exportacion debe leer y escribir cada nodo una vez.");
        exigir(ram.metricas().lecturas == 3 && ram.metricas().escrituras == 4,
               "La exportacion altero las metricas de construccion.");
    }  // Destruir el vector antes de volver a leer desde disco.

    exigir(std::filesystem::file_size(ruta) == 3 * TAMANO_BLOQUE,
           "El archivo debe contener exactamente tres bloques, sin cabecera.");
    ArchivoNodos<Nodo> archivo(ruta.string());
    exigir(archivo.cantidad_nodos() == 3 && archivo.metricas().total() == 0,
           "Abrir el archivo no debe leer nodos ni contar I/Os.");
    for (const int indice : {2, 0, 1, 2}) {
        const Nodo leido = archivo.leer(indice);
        const Nodo& esperado = indice == 0 ? esperado0 : (indice == 1 ? esperado1 : esperado2);
        exigir(std::memcmp(&leido, &esperado, sizeof(Nodo)) == 0,
               "Los campos del nodo leido no coinciden con los guardados.");
    }
    exigir(archivo.metricas().lecturas == 4 && archivo.metricas().escrituras == 0,
           "Cada acceso al archivo debe contar una lectura, incluso si se repite.");
    for (const int indice : {-1, 3}) {
        bool rechazado = false;
        try { archivo.leer(indice); } catch (const std::out_of_range&) { rechazado = true; }
        exigir(rechazado, "Se acepto un indice inexistente en disco.");
    }
    exigir(archivo.metricas().lecturas == 4, "Se conto una lectura fallida.");
    archivo.reiniciar_metricas();
    exigir(archivo.metricas().total() == 0, "No se reiniciaron las metricas.");
    const Nodo raiz = archivo.leer(0);
    exigir(std::memcmp(&raiz, &esperado0, sizeof(Nodo)) == 0
           && archivo.metricas().lecturas == 1, "Fallo la lectura tras reiniciar metricas.");
    std::cout << "RAM, serializacion y lectura por bloques (" << ruta.filename().string()
              << "): OK\n";
}

/**
 * Comprueba que los fallos básicos de archivo no se aceptan como lecturas válidas.
 * directorio: carpeta temporal de pruebas. Sin retorno; lanza excepción si falla.
 */
void probar_errores_archivo(const std::filesystem::path& directorio) {
    bool rechazado = false;
    try {
        ArchivoNodos<NodoB> archivo((directorio / "inexistente.bin").string());
    } catch (const std::runtime_error&) { rechazado = true; }
    exigir(rechazado, "Se acepto un archivo inexistente.");

    const auto ruta = directorio / "incompleto.bin";
    {
        std::ofstream salida(ruta, std::ios::binary);
        salida << "abc";
    }
    rechazado = false;
    try {
        ArchivoNodos<NodoB> archivo(ruta.string());
    } catch (const std::runtime_error&) { rechazado = true; }
    exigir(rechazado, "Se acepto un archivo con un bloque incompleto.");

    std::istringstream entrada("abc");
    rechazado = false;
    try { leer_nodo<NodoB>(entrada); } catch (const std::runtime_error&) { rechazado = true; }
    exigir(rechazado, "Se devolvio un nodo parcialmente leido.");

    std::ostringstream salida;
    salida.setstate(std::ios::badbit);
    rechazado = false;
    try { escribir_nodo(salida, NodoB{}); } catch (const std::runtime_error&) { rechazado = true; }
    exigir(rechazado, "No se detecto un fallo de escritura.");
    std::cout << "Errores basicos de archivo: OK\n";
}

/**
 * Ejecuta las comprobaciones de representación, métricas y almacenamiento.
 * No recibe parámetros ni requiere el dataset. Devuelve 0 si pasan
 * las comprobaciones y 1 si falla alguna; escribe el resultado en consola.
 * Usa archivos temporales propios; los algoritmos se prueban en pruebas_arboles.cpp.
 */
int main() {
    const NodoB nodo_b;
    const NodoBE025 nodo_be025;
    const NodoBE075 nodo_be075;

    if (nodo_b.es_interno != 0 || nodo_b.k != 0 || nodo_b.padding != 0 ||
        nodo_be025.es_interno != 0 || nodo_be025.k_llaves != 0 ||
        nodo_be025.k_buffer != 0 || nodo_be075.es_interno != 0 ||
        nodo_be075.k_llaves != 0 || nodo_be075.k_buffer != 0) {
        std::cerr << "Error: los nodos deben comenzar como hojas vacias.\n";
        return 1;
    }

    ContadorIO contador;
    if (contador.total() != 0) {
        std::cerr << "Error: el contador debe comenzar en cero.\n";
        return 1;
    }
    // El total debe poder superar el máximo de un entero de 32 bits.
    contador.lecturas = std::uint64_t{1} << 32;
    contador.escrituras = 7;
    if (contador.total() != 4294967303ULL) {
        std::cerr << "Error: el total de I/Os no se acumula en 64 bits.\n";
        return 1;
    }

    std::cout << "Registro: " << sizeof(Registro) << " bytes\n"
              << "NodoB: " << sizeof(NodoB) << " bytes\n"
              << "NodoBE025: " << sizeof(NodoBE025) << " bytes\n"
              << "NodoBE075: " << sizeof(NodoBE075) << " bytes\n"
              << "Representacion de nodos y contadores: OK\n";
    try {
        DirectorioTemporal temporal;
        probar_almacenamiento<NodoB>(temporal.ruta / "b.bin");
        probar_almacenamiento<NodoBE025>(temporal.ruta / "be025.bin");
        probar_almacenamiento<NodoBE075>(temporal.ruta / "be075.bin");
        probar_errores_archivo(temporal.ruta);
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
