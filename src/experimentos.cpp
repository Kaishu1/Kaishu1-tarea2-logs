#include "experimentos.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <random>
#include <stdexcept>
#include <string>
#include <sys/utsname.h>
#include <vector>

#include "arbol_b.hpp"
#include "arbol_be.hpp"

#ifndef OPCIONES_COMPILACION
#define OPCIONES_COMPILACION "no registradas"
#endif

namespace {
namespace fs = std::filesystem;
using Reloj = std::chrono::steady_clock;
constexpr int CONSULTAS = 50;
constexpr int TIEMPO_MINIMO = 1546300800;
constexpr int TIEMPO_MAXIMO = 1754006400;
constexpr int SEMANA = 604800;
constexpr std::size_t PARES_POR_LOTE = 65536;

/** Rango de consulta: l es el límite inferior y u el superior, ambos inclusivos y en segundos Unix. */
struct Rango { int l; int u; };

/** Devuelve segundos transcurridos desde inicio con reloj monótono. */
double segundos(Reloj::time_point inicio) {
    return std::chrono::duration<double>(Reloj::now() - inicio).count();
}

/** Abre ruta y devuelve un flujo de escritura con precisión numérica; lanza un error si falla. */
std::ofstream salida(const fs::path& ruta) {
    std::ofstream archivo;
    archivo.exceptions(std::ios::failbit | std::ios::badbit);
    archivo.open(ruta);
    archivo << std::setprecision(std::numeric_limits<double>::max_digits10);
    return archivo;
}

/**
 * Lee cantidad registros de archivo y los guarda en destino, que debe tener espacio.
 * Avanza el archivo; no devuelve valor y lanza un error si faltan datos.
 */
void leer_registros(std::ifstream& archivo, Registro* destino, std::size_t cantidad) {
    archivo.read(reinterpret_cast<char*>(destino),
                 static_cast<std::streamsize>(cantidad * sizeof(Registro)));
    if (!archivo) throw std::runtime_error("No se pudo leer el lote completo del dataset.");
}

/**
 * Valida configuración y tamaño del dataset antes de crear resultados.
 * config contiene la ruta de entrada y los exponentes que se van a ejecutar.
 * El formato de entrada es little-endian, como el dataset comprobado de la tarea.
 * Si existe datos_sample.txt junto al binario, compara sus registros (texto redondeado).
 * Devuelve cuántos registros de muestra se verificaron; no mide ni modifica datos.
 */
std::size_t validar_entrada(const ConfiguracionExperimento& config) {
    if (config.exponente_minimo < 15 || config.exponente_maximo > 26
        || config.exponente_minimo > config.exponente_maximo) {
        throw std::invalid_argument("Se requiere 15 <= min-exp <= max-exp <= 26.");
    }
    const int uno = 1;
    if (*reinterpret_cast<const unsigned char*>(&uno) != 1) {
        throw std::runtime_error("El lector del dataset requiere una plataforma little-endian.");
    }
    const auto bytes = fs::file_size(config.datos);
    if (bytes % sizeof(Registro) != 0
        || bytes / sizeof(Registro) < (std::uint64_t{1} << config.exponente_maximo)) {
        throw std::runtime_error("El dataset esta incompleto o no contiene suficientes pares.");
    }
    std::ifstream binario(config.datos, std::ios::binary);
    if (!binario) throw std::runtime_error("No se pudo abrir el dataset.");
    const auto ruta_muestra = config.datos.parent_path() / "datos_sample.txt";
    if (!fs::exists(ruta_muestra)) return 0;
    std::ifstream muestra(ruta_muestra);
    if (!muestra) throw std::runtime_error("No se pudo abrir la muestra de texto.");
    std::string cabecera;
    std::getline(muestra, cabecera);
    std::size_t comprobados = 0;
    int llave;
    double valor;
    while (muestra >> llave) {
        if (!(muestra >> valor)) throw std::runtime_error("Muestra de texto incompleta.");
        Registro par;
        leer_registros(binario, &par, 1);
        if (llave != par.llave || !std::isfinite(valor)
            || !std::isfinite(par.valor) || std::abs(valor - par.valor) > 0.0001) {
            throw std::runtime_error("La muestra no coincide con el dataset binario.");
        }
        ++comprobados;
    }
    if (!muestra.eof() || comprobados == 0) throw std::runtime_error("Muestra de texto invalida.");
    return comprobados;
}

/** Copia el archivo de ruta al flujo destino; si no existe, lo indica. No devuelve valor. */
void informacion_sistema(std::ostream& destino, const fs::path& ruta) {
    destino << "\n[" << ruta.string() << "]\n";
    std::ifstream entrada(ruta);
    if (entrada) destino << entrada.rdbuf();
    else destino << "No disponible\n";
}

/**
 * Escribe en ruta los parámetros de config y el entorno del equipo.
 * muestra es la cantidad de registros verificados contra el archivo de texto.
 * Guarda también compilador, SO, CPU, cachés y RAM; no devuelve valor.
 */
void guardar_entorno(const fs::path& ruta, const ConfiguracionExperimento& config,
                     std::size_t muestra) {
    auto archivo = salida(ruta);
    archivo << "dataset=" << fs::absolute(config.datos).string()
            << "\ndataset_bytes=" << fs::file_size(config.datos)
            << "\nmin_exp=" << config.exponente_minimo << "\nmax_exp=" << config.exponente_maximo
            << "\nsemilla=" << config.semilla << "\nconsultas=" << CONSULTAS
            << "\nlimite_inferior_min=" << TIEMPO_MINIMO << "\nlimite_inferior_max=" << TIEMPO_MAXIMO
            << "\nancho_rango_s=" << SEMANA << "\nalfa_be025=" << ArbolBE025::alfa
            << "\nalfa_be075=" << ArbolBE075::alfa << "\npares_muestra_verificados=" << muestra
            << "\nrepeticiones_por_arbol_y_N=1\npares_por_lote=" << PARES_POR_LOTE
            << "\ncompilador=" << __VERSION__ << "\nopciones=" << OPCIONES_COMPILACION
            << "\ncompilacion=" << __DATE__ << ' ' << __TIME__
            << "\nalcance=" << (config.exponente_minimo == 15 && config.exponente_maximo == 26
                                ? "todos_los_tamanos" : "parcial")
            << "\norden_arboles=B,BE025,BE075\nconsulta_exportada=0\n"
            << "insercion_s incluye crear raiz e insertar; excluye lectura del dataset.\n"
            << "consulta_s incluye recuperar y ordenar pares; excluye abrir archivo y exportar.\n"
            << "No se vacian caches: lecturas de bloques logicas, no transferencias fisicas.\n"
            << "CPU, caches y RAM son las visibles en el sistema invitado si se usa una VM.\n";
    struct utsname sistema {};
    if (uname(&sistema) == 0) {
        archivo << "sistema=" << sistema.sysname << ' ' << sistema.release << ' ' << sistema.machine << '\n';
    }
    informacion_sistema(archivo, "/etc/os-release");
    informacion_sistema(archivo, "/proc/meminfo");
    std::ifstream cpu("/proc/cpuinfo");
    std::string linea;
    bool modelo = false;
    while (std::getline(cpu, linea)) {
        if (linea.rfind("model name", 0) == 0) {
            archivo << '\n' << linea << '\n';
            modelo = true;
            break;
        }
    }
    if (!modelo) archivo << "\nModelo CPU no disponible\n";
    const fs::path caches = "/sys/devices/system/cpu/cpu0/cache";
    if (fs::exists(caches)) {
        for (const auto& entrada : fs::directory_iterator(caches)) {
            if (entrada.is_directory() && entrada.path().filename().string().rfind("index", 0) == 0) {
                for (const char* campo : {"level", "type", "size", "coherency_line_size", "shared_cpu_list"}) {
                    informacion_sistema(archivo, entrada.path() / campo);
                }
            }
        }
    } else archivo << "\nCaches CPU no disponibles\n";
    archivo.close();
}

/**
 * Guarda pares y los límites de rango en el CSV indicado por ruta.
 * Corresponde a la consulta 0 y se ejecuta fuera de la medición; no devuelve valor.
 */
void guardar_muestra(const fs::path& ruta, Rango rango, const std::vector<Registro>& pares) {
    auto archivo = salida(ruta);
    archivo << "consulta,l_unix_s,u_unix_s,tiempo_unix_s,temperatura_c\n";
    for (Registro par : pares) {
        archivo << "0," << rango.l << ',' << rango.u << ',' << par.llave << ',' << par.valor << '\n';
    }
    archivo.close();
}

/**
 * Construye TipoArbol con n pares y lo guarda usando bloques de tipo Nodo.
 * config indica los datos y la semilla; nombre, epsilon y alfa identifican el árbol.
 * capacidad y buffer indican sus máximos de llaves y pares en buffer.
 * rangos contiene las 50 consultas; arboles y muestras son las carpetas de salida.
 * Escribe las mediciones en resumen y consultas y muestra el avance en terminal.
 * Libera el árbol en RAM antes de buscar; imprime y exporta fuera de los tiempos medidos.
 * No devuelve valor.
 */
template <typename TipoArbol, typename Nodo>
void experimentar_arbol(const ConfiguracionExperimento& config, std::uint64_t n,
                        const char* nombre, const char* epsilon, const char* alfa,
                        int capacidad, int buffer, const std::array<Rango, CONSULTAS>& rangos,
                        const fs::path& arboles, const fs::path& muestras,
                        std::ofstream& resumen, std::ofstream& consultas) {
    const std::string base = std::string(nombre) + "_" + std::to_string(n);
    const fs::path binario = arboles / (base + ".bin");
    std::cout << '\n' << nombre << " N=" << n << ": construccion\n" << std::flush;
    double insercion_s = 0, lectura_datos_s = 0, serializacion_s = 0;
    ContadorIO construccion, exportacion;
    std::size_t nodos = 0;
    std::ifstream datos(config.datos, std::ios::binary);
    if (!datos) throw std::runtime_error("No se pudo reabrir el dataset.");
    std::vector<Registro> lote(PARES_POR_LOTE);
    const auto comienzo = Reloj::now();
    {
        TipoArbol arbol;
        insercion_s = segundos(comienzo);
        std::uint64_t insertados = 0;
        int progreso = -1;
        while (insertados < n) {
            const auto cantidad = static_cast<std::size_t>(std::min<std::uint64_t>(lote.size(), n - insertados));
            auto inicio = Reloj::now();
            leer_registros(datos, lote.data(), cantidad);
            lectura_datos_s += segundos(inicio);
            inicio = Reloj::now();
            for (std::size_t i = 0; i < cantidad; ++i) arbol.insertar(lote[i]);
            insercion_s += segundos(inicio);
            insertados += cantidad;
            const int avance = static_cast<int>(100 * insertados / n);
            if (avance / 10 != progreso) {
                std::cout << "  Insertados " << insertados << '/' << n << " (" << avance << "%)\n" << std::flush;
                progreso = avance / 10;
            }
        }
        construccion = arbol.metricas();
        nodos = arbol.cantidad_nodos();
        std::cout << "  Guardando " << nodos << " nodos...\n" << std::flush;
        const auto inicio = Reloj::now();
        exportacion = arbol.guardar(binario.string());
        serializacion_s = segundos(inicio);
    }
    const auto bytes = fs::file_size(binario);
    if (bytes != nodos * TAMANO_BLOQUE) throw std::runtime_error("Tamano serializado incorrecto.");
    ArchivoNodos<Nodo> archivo(binario.string());
    double tiempo_consultas = 0;
    std::uint64_t io_consultas = 0;
    for (int i = 0; i < CONSULTAS; ++i) {
        archivo.reiniciar_metricas();
        const auto inicio = Reloj::now();
        const auto pares = TipoArbol::buscar_rango(archivo, rangos[i].l, rangos[i].u);
        const double tiempo = segundos(inicio);
        const auto io = archivo.metricas();
        tiempo_consultas += tiempo;
        io_consultas += io.total();
        consultas << nombre << ',' << n << ',' << config.semilla << ',' << alfa << ',' << i << ','
                  << rangos[i].l << ',' << rangos[i].u << ',' << tiempo << ',' << io.lecturas
                  << ',' << io.escrituras << ',' << pares.size() << '\n';
        if (i == 0) guardar_muestra(muestras / (base + ".csv"), rangos[i], pares);
        if ((i + 1) % 10 == 0) std::cout << "  Consultas " << i + 1 << '/' << CONSULTAS << '\n' << std::flush;
    }
    const double promedio_s = tiempo_consultas / CONSULTAS;
    const double promedio_io = static_cast<double>(io_consultas) / CONSULTAS;
    resumen << nombre << ',' << n << ',' << config.semilla << ',' << epsilon << ',' << alfa << ','
            << capacidad << ',' << buffer << ',' << CONSULTAS << ',' << insercion_s << ','
            << construccion.lecturas << ',' << construccion.escrituras << ',' << construccion.total()
            << ',' << nodos << ',' << bytes << ',' << promedio_s << ',' << promedio_io << ','
            << lectura_datos_s << ',' << serializacion_s << ',' << exportacion.lecturas
            << ',' << exportacion.escrituras << '\n';
    resumen.flush();
    consultas.flush();
    std::cout << std::setprecision(6) << "  Resumen " << nombre << " N=" << n
              << ": insercion=" << insercion_s << " s, I/Os=" << construccion.total()
              << ", tamano=" << bytes << " bytes; consulta promedio=" << promedio_s
              << " s, I/Os=" << promedio_io << '\n' << std::flush;
}
}  // namespace

std::filesystem::path ejecutar_experimentos(const ConfiguracionExperimento& config) {
    const auto muestra = validar_entrada(config);
    const std::string id = "ejecucion-" + std::to_string(
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
    fs::create_directories("resultados");
    const fs::path carpeta = fs::path("resultados") / id;
    if (!fs::create_directory(carpeta)) throw std::runtime_error("La carpeta de ejecucion ya existe.");
    const auto estado = carpeta / "estado.txt";
    try {
        { auto archivo = salida(estado); archivo << "en_curso\n"; archivo.close(); }
        const fs::path arboles = fs::path("arboles_generados") / id;
        const fs::path muestras = fs::path("resultados/muestras") / id;
        fs::create_directories(arboles.parent_path());
        fs::create_directories(muestras.parent_path());
        if (!fs::create_directory(arboles) || !fs::create_directory(muestras)) {
            throw std::runtime_error("Ya existe una carpeta de archivos de esta ejecucion.");
        }
        guardar_entorno(carpeta / "entorno.txt", config, muestra);
        std::mt19937 generador(config.semilla);
        std::uniform_int_distribution<int> distribucion(TIEMPO_MINIMO, TIEMPO_MAXIMO);
        std::array<Rango, CONSULTAS> rangos{};
        auto archivo_rangos = salida(carpeta / "rangos.csv");
        archivo_rangos << "consulta,l_unix_s,u_unix_s\n";
        for (int i = 0; i < CONSULTAS; ++i) {
            const int l = distribucion(generador);
            rangos[i] = {l, l + SEMANA};
            archivo_rangos << i << ',' << l << ',' << l + SEMANA << '\n';
        }
        archivo_rangos.close();
        auto resumen = salida(carpeta / "resumen.csv");
        resumen << "arbol,n,semilla,epsilon,alfa,capacidad_llaves,capacidad_buffer,consultas,"
                   "insercion_s,insercion_lecturas,insercion_escrituras,insercion_io,nodos,tamano_bytes,"
                   "consulta_promedio_s,consulta_promedio_io,lectura_datos_s,serializacion_s,"
                   "serializacion_lecturas,serializacion_escrituras\n";
        auto consultas = salida(carpeta / "consultas.csv");
        consultas << "arbol,n,semilla,alfa,consulta,l_unix_s,u_unix_s,tiempo_s,lecturas,escrituras,pares\n";
        std::cout << "Resultados: " << carpeta.string() << "\nMuestra verificada: " << muestra
                  << " pares\nSemilla: " << config.semilla << "; alpha BE: 0.5; 50 rangos comunes\n"
                  << "Alcance: " << (config.exponente_minimo == 15 && config.exponente_maximo == 26
                                     ? "todos los tamanos" : "ejecucion parcial") << '\n';
        for (int exponente = config.exponente_minimo; exponente <= config.exponente_maximo; ++exponente) {
            const auto n = std::uint64_t{1} << exponente;
            experimentar_arbol<ArbolB, NodoB>(config, n, "B", "", "", NodoB::b, 0,
                                               rangos, arboles, muestras, resumen, consultas);
            experimentar_arbol<ArbolBE025, NodoBE025>(config, n, "BE025", "0.25", "0.5", 4, 506,
                                                      rangos, arboles, muestras, resumen, consultas);
            experimentar_arbol<ArbolBE075, NodoBE075>(config, n, "BE075", "0.75", "0.5", 126, 384,
                                                      rangos, arboles, muestras, resumen, consultas);
        }
        resumen.close();
        consultas.close();
        { auto archivo = salida(estado); archivo << "completa_para_el_intervalo_solicitado\n"; archivo.close(); }
        std::cout << "\nEjecucion terminada: " << carpeta.string() << '\n';
    } catch (const std::exception& error) {
        try { auto archivo = salida(estado); archivo << "incompleta\n" << error.what() << '\n'; archivo.close(); }
        catch (...) { /* Conservar la excepción original si tampoco se puede escribir el estado. */ }
        throw;
    }
    return carpeta;
}
