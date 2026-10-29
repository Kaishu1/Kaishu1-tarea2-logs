#include "experimentos.hpp"
#include "registro.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
namespace fs = std::filesystem;
using Fila = std::vector<std::string>;

/** Lanza un error descriptivo si condicion no se cumple; no modifica datos. */
void exigir(bool condicion, const std::string& mensaje) {
    if (!condicion) throw std::runtime_error(mensaje);
}

/** Directorio de trabajo temporal; restaura el original y limpia solo sus archivos. */
struct EntornoTemporal {
    fs::path original = fs::current_path();
    fs::path temporal;
    /** Crea un directorio exclusivo y lo utiliza como directorio actual de las pruebas. */
    EntornoTemporal() {
        temporal = fs::temp_directory_path() / ("tarea2-experimentos-" + std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count()));
        exigir(fs::create_directory(temporal), "No se pudo crear el entorno temporal.");
        fs::current_path(temporal);
    }
    /** Restaura el directorio original incluso si falla una prueba. */
    ~EntornoTemporal() {
        std::error_code error;
        fs::current_path(original, error);
        fs::remove_all(temporal, error);
    }
};

/** Lee un archivo completo para comprobar estado y conservación entre ejecuciones. */
std::string leer_texto(const fs::path& ruta) {
    std::ifstream archivo(ruta);
    exigir(static_cast<bool>(archivo), "No se pudo abrir " + ruta.string());
    return {std::istreambuf_iterator<char>(archivo), std::istreambuf_iterator<char>()};
}

/** Lee los CSV numéricos de esta aplicación; exige columnas y omite la cabecera. */
std::vector<Fila> leer_csv(const fs::path& ruta, std::size_t columnas) {
    std::ifstream archivo(ruta);
    exigir(static_cast<bool>(archivo), "CSV ausente: " + ruta.string());
    std::string linea;
    std::getline(archivo, linea);
    std::vector<Fila> filas;
    while (std::getline(archivo, linea)) {
        std::istringstream entrada(linea);
        Fila fila;
        std::string valor;
        while (std::getline(entrada, valor, ',')) fila.push_back(valor);
        exigir(fila.size() == columnas, "Cantidad incorrecta de columnas en " + ruta.string());
        filas.push_back(fila);
    }
    return filas;
}

/** Devuelve pares ordenados mediante filtrado directo del prefijo de n registros. */
std::vector<Registro> filtrar(const std::vector<Registro>& datos, std::size_t n, int l, int u) {
    std::vector<Registro> esperado;
    for (std::size_t i = 0; i < n; ++i) {
        if (datos[i].llave >= l && datos[i].llave <= u) esperado.push_back(datos[i]);
    }
    std::sort(esperado.begin(), esperado.end(), [](Registro a, Registro b) { return a.llave < b.llave; });
    return esperado;
}

/**
 * Verifica tamaños, rangos comunes, cantidades de consultas, promedios y muestras
 * contra el prefijo real de datos. Requiere una ejecución con N=2^15 y N=2^16.
 */
void verificar_resultados(const fs::path& carpeta, const std::vector<Registro>& datos) {
    exigir(leer_texto(carpeta / "estado.txt") == "completa_para_el_intervalo_solicitado\n",
           "Estado final incorrecto.");
    const auto entorno = leer_texto(carpeta / "entorno.txt");
    exigir(entorno.find("alcance=parcial") != std::string::npos
           && entorno.find("pares_muestra_verificados=50") != std::string::npos,
           "No se registraron alcance o verificacion de muestra.");
    const auto rangos = leer_csv(carpeta / "rangos.csv", 3);
    const auto consultas = leer_csv(carpeta / "consultas.csv", 11);
    const auto resumen = leer_csv(carpeta / "resumen.csv", 20);
    exigir(rangos.size() == 50 && consultas.size() == 300 && resumen.size() == 6,
           "Cantidad incorrecta de rangos o mediciones.");
    std::map<std::string, double> tiempos, ios;
    std::map<std::string, int> cantidades;
    for (const auto& fila : consultas) {
        const auto n = std::stoull(fila[1]);
        const auto i = std::stoul(fila[4]);
        exigir(i < rangos.size(), "Consulta inexistente.");
        const int l = std::stoi(fila[5]);
        const int u = std::stoi(fila[6]);
        exigir(l >= 1546300800 && l <= 1754006400 && u == l + 604800,
               "Rango fuera del enunciado.");
        exigir(fila[5] == rangos[i][1] && fila[6] == rangos[i][2], "Rangos distintos entre arboles o N.");
        exigir(n == 32768 || n == 65536, "N inesperado.");
        exigir(std::stoull(fila[10]) == filtrar(datos, n, l, u).size(), "Cantidad de pares incorrecta.");
        exigir(fila[9] == "0" && std::stod(fila[7]) >= 0 && std::stoull(fila[8]) > 0,
               "Metricas individuales invalidas.");
        const std::string clave = fila[0] + '_' + fila[1];
        tiempos[clave] += std::stod(fila[7]);
        ios[clave] += std::stod(fila[8]);
        ++cantidades[clave];
    }
    for (const auto& fila : resumen) {
        const std::string base = fila[0] + '_' + fila[1];
        const auto n = std::stoull(fila[1]);
        exigir(cantidades[base] == 50 && fila[7] == "50" && fila[2] == "4102",
               "Faltan consultas o parametros.");
        exigir((fila[0] == "B" && fila[4].empty()) || (fila[0] != "B" && fila[4] == "0.5"),
               "Alpha incorrecto.");
        exigir(std::abs(std::stod(fila[14]) - tiempos[base] / 50) < 1e-12
               && std::abs(std::stod(fila[15]) - ios[base] / 50) < 1e-12, "Promedio incorrecto.");
        const auto nodos = std::stoull(fila[12]);
        exigir(std::stoull(fila[11]) == std::stoull(fila[9]) + std::stoull(fila[10]),
               "Total I/Os incorrecto.");
        exigir(std::stoull(fila[18]) == nodos && std::stoull(fila[19]) == nodos,
               "Serializacion mezclada con inserciones.");
        const auto bytes = fs::file_size(fs::path("arboles_generados") / carpeta.filename() / (base + ".bin"));
        exigir(bytes == nodos * TAMANO_BLOQUE && bytes == std::stoull(fila[13]), "Tamano incorrecto.");
        const auto muestras = leer_csv(fs::path("resultados/muestras") / carpeta.filename() / (base + ".csv"), 5);
        const auto esperado = filtrar(datos, n, std::stoi(rangos[0][1]), std::stoi(rangos[0][2]));
        exigir(!esperado.empty() && muestras.size() == esperado.size(), "Muestra incompleta.");
        for (std::size_t i = 0; i < esperado.size(); ++i) {
            exigir(std::stoi(muestras[i][3]) == esperado[i].llave
                   && std::stof(muestras[i][4]) == esperado[i].valor, "Muestra alterada.");
        }
    }
}
}  // namespace

/** Ejecuta una integración reproducible con datos sintéticos, sin tocar el dataset original. */
int main() {
    try {
        EntornoTemporal entorno;
        fs::create_directory("datos");
        std::vector<Registro> datos;
        std::ofstream binario("datos/datos.bin", std::ios::binary);
        std::ofstream muestra("datos/datos_sample.txt");
        muestra << "timestamp\ttemperatura\n";
        for (int i = 0; i < 65536; ++i) {
            Registro par {1546300800 + static_cast<int>((static_cast<std::uint64_t>(i) * 7919) % 207705601),
                          static_cast<float>(i % 401 - 200) / 8.0F};
            datos.push_back(par);
            binario.write(reinterpret_cast<const char*>(&par), sizeof(par));
            if (i < 50) muestra << par.llave << '\t' << par.valor << '\n';
        }
        binario.close();
        muestra.close();
        ConfiguracionExperimento config;
        bool rechazado = false;
        try { ejecutar_experimentos(config); }
        catch (const std::runtime_error&) { rechazado = true; }
        exigir(rechazado && !fs::exists("resultados"), "No se rechazo el dataset corto antes de ejecutar.");
        config.exponente_maximo = 16;
        const auto primera = ejecutar_experimentos(config);
        verificar_resultados(primera, datos);
        const auto resumen_anterior = leer_texto(primera / "resumen.csv");
        config.exponente_maximo = 15;
        const auto segunda = ejecutar_experimentos(config);
        exigir(segunda != primera && leer_texto(primera / "resumen.csv") == resumen_anterior,
               "La nueva ejecucion sobrescribio los resultados anteriores.");
        exigir(leer_texto(primera / "rangos.csv") == leer_texto(segunda / "rangos.csv"),
               "La semilla no reproduce los rangos.");
        std::cout << "Experimentos: CSV, promedios, rangos, muestras y conservacion de resultados: OK\n";
    } catch (const std::exception& error) {
        std::cerr << "Error en pruebas de experimentos: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
