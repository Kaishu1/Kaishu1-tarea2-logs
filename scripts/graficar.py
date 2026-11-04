#!/usr/bin/env python3
"""Genera las seis figuras de la tarea desde una ejecución completa, sin repetirla."""

import argparse
import csv
import filecmp
import math
from pathlib import Path

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.ticker import FuncFormatter

RAIZ = Path(__file__).resolve().parents[1]
ARBOLES = {
    "B": ("B", "#0072B2", "o"),
    "BE025": (r"B$^{\varepsilon}$, $\varepsilon \approx 0{,}25$", "#D55E00", "s"),
    "BE075": (r"B$^{\varepsilon}$, $\varepsilon \approx 0{,}75$", "#009E73", "^"),
}
TAMANOS = [2**e for e in range(15, 27)]
METRICAS = [
    ("01_tiempo_insercion", "insercion_s", 1, "Tiempo de inserción", "Tiempo (s)"),
    ("02_io_insercion", "insercion_io", 1, "I/Os durante la inserción", "Lecturas + escrituras simuladas"),
    ("03_tamano_arbol", "nodos", 1, "Tamaño de los árboles", "Cantidad de nodos de 4096 bytes"),
    ("04_tiempo_busqueda", "consulta_promedio_s", 1000, "Tiempo promedio de búsqueda", "Tiempo por consulta (ms)"),
    ("05_io_busqueda", "consulta_promedio_io", 1, "I/Os promedio de búsqueda", "Lecturas de bloques por consulta"),
]


def leer_csv(ruta):
    """Devuelve las filas de ruta como diccionarios; no modifica el archivo."""
    with ruta.open(newline="", encoding="utf-8") as archivo:
        return list(csv.DictReader(archivo))


def comprobar(condicion, mensaje):
    """Interrumpe la generación con un error explicativo si no se cumple condicion."""
    if not condicion:
        raise ValueError(mensaje)


def leer_mediciones(carpeta):
    """Valida corrida completa, parámetros y medias; devuelve series ordenadas y semilla."""
    comprobar((carpeta / "estado.txt").read_text().strip()
              == "completa_para_el_intervalo_solicitado", "La ejecución no terminó correctamente.")
    filas = leer_csv(carpeta / "resumen.csv")
    claves = {(fila["arbol"], int(fila["n"])) for fila in filas}
    comprobar(len(filas) == 36 and claves == {(a, n) for a in ARBOLES for n in TAMANOS},
              "Se requieren los tres árboles para los doce tamaños de 2^15 a 2^26.")
    semillas = {fila["semilla"] for fila in filas}
    comprobar(len(semillas) == 1, "El resumen mezcla semillas.")
    series = {a: sorted((f for f in filas if f["arbol"] == a), key=lambda f: int(f["n"]))
              for a in ARBOLES}
    consultas = leer_csv(carpeta / "consultas.csv")
    rangos = leer_csv(carpeta / "rangos.csv")
    comprobar(len(consultas) == 1800 and len(rangos) == 50, "Faltan consultas o rangos.")
    for fila in filas:
        a, n = fila["arbol"], int(fila["n"])
        comprobar(int(fila["consultas"]) == 50, "No hay 50 consultas por árbol y N.")
        if a != "B":
            comprobar(float(fila["alfa"]) == 0.5, "Esta generación espera α = 0,5.")
        for _, columna, _, _, _ in METRICAS:
            valor = float(fila[columna])
            comprobar(math.isfinite(valor) and valor > 0,
                      f"Métrica no positiva o no finita para escala logarítmica: {a}, {n}, {columna}.")
        grupo = [f for f in consultas if f["arbol"] == a and int(f["n"]) == n]
        comprobar(len(grupo) == 50 and {int(f["consulta"]) for f in grupo} == set(range(50)),
                  "Consultas duplicadas o ausentes.")
        for consulta in grupo:
            rango = rangos[int(consulta["consulta"])]
            comprobar(all(consulta[c] == rango[c] for c in ("l_unix_s", "u_unix_s")),
                      "Los árboles o tamaños no comparten los mismos rangos.")
            comprobar(int(consulta["escrituras"]) == 0, "Una consulta registra escrituras.")
        for destino, origen in (("consulta_promedio_s", "tiempo_s"),
                                ("consulta_promedio_io", "lecturas")):
            media = sum(float(f[origen]) for f in grupo) / 50
            comprobar(math.isclose(media, float(fila[destino]), rel_tol=1e-10),
                      f"El promedio no coincide con las consultas: {a}, {n}, {destino}.")
    return series, next(iter(semillas))


def estilo():
    """Configura tipografía y ejes legibles para figuras que se incorporarán al informe."""
    plt.rcParams.update({
        "font.family": "DejaVu Sans", "font.size": 11,
        "axes.titlesize": 15, "axes.titleweight": "bold", "axes.labelsize": 11,
        "axes.spines.top": False, "axes.spines.right": False,
        "axes.edgecolor": "#67717A", "text.color": "#20262C",
        "axes.labelcolor": "#20262C", "xtick.color": "#434C55", "ytick.color": "#434C55",
        "legend.frameon": False, "legend.fontsize": 10,
        "savefig.facecolor": "white", "pdf.fonttype": 42,
    })


def guardar(figura, carpeta, nombre):
    """Guarda la figura en PNG de 220 dpi y PDF; cierra la figura para liberar memoria."""
    for extension in ("png", "pdf"):
        figura.savefig(carpeta / f"{nombre}.{extension}", dpi=220, bbox_inches="tight")
    plt.close(figura)
    print(f"Generado: {carpeta / nombre} (.png y .pdf)")


def graficar_metricas(series, semilla, destino):
    """Grafica cinco métricas con los doce puntos originales por árbol, sin suavizado."""
    for nombre, columna, factor, titulo, unidad in METRICAS:
        figura, eje = plt.subplots(figsize=(8.1, 5.0))
        figura.subplots_adjust(left=0.13, right=0.98, top=0.88, bottom=0.22)
        for arbol, (etiqueta, color, marca) in ARBOLES.items():
            valores = [float(f[columna]) * factor for f in series[arbol]]
            eje.plot(TAMANOS, valores, color=color, marker=marca, markersize=5,
                     linewidth=1.7, label=etiqueta)
        eje.set_xscale("log", base=2)
        eje.set_yscale("log", base=10)
        eje.set_xticks(TAMANOS, [rf"$2^{{{e}}}$" for e in range(15, 27)])
        eje.set_xlabel("Cantidad de pares insertados, N (escala log₂)", labelpad=9)
        eje.set_ylabel(unidad + " (escala log₁₀)", labelpad=8)
        eje.set_title(titulo, loc="left", pad=15)
        eje.grid(axis="both", which="major", color="#DDE2E6", linewidth=0.7)
        eje.set_axisbelow(True)
        eje.legend(loc="upper left")
        figura.text(0.13, 0.055, f"Semilla {semilla} · α = 0,5 en Bε · 50 consultas por árbol y N",
                    fontsize=9, color="#59636D")
        guardar(figura, destino, nombre)


def leer_muestras(corrida):
    """Lee N mínimo/máximo para consulta 0 y verifica que los tres árboles coincidan."""
    carpeta = RAIZ / "resultados" / "muestras" / corrida
    muestras, limites = [], None
    for n in (TAMANOS[0], TAMANOS[-1]):
        ruta = carpeta / f"B_{n}.csv"
        for arbol in ("BE025", "BE075"):
            comprobar(filecmp.cmp(ruta, carpeta / f"{arbol}_{n}.csv", shallow=False),
                      f"Las muestras no coinciden entre árboles para N={n}.")
        filas = leer_csv(ruta)
        comprobar(bool(filas), f"La muestra de N={n} no contiene pares.")
        rangos = {(int(f["consulta"]), int(f["l_unix_s"]), int(f["u_unix_s"])) for f in filas}
        comprobar(len(rangos) == 1, "Una muestra mezcla consultas.")
        actual = next(iter(rangos))
        comprobar(actual[0] == 0 and actual[2] - actual[1] == 604800, "Rango de muestra incorrecto.")
        if limites is not None:
            comprobar(actual == limites, "Los dos N no corresponden al mismo rango.")
        limites = actual
        tiempos = [int(f["tiempo_unix_s"]) for f in filas]
        temperaturas = [float(f["temperatura_c"]) for f in filas]
        comprobar(all(math.isfinite(t) for t in temperaturas), "Temperatura no finita.")
        comprobar(all(limites[1] <= t <= limites[2] for t in tiempos), "Par fuera del rango.")
        comprobar(all(a <= b for a, b in zip(tiempos, tiempos[1:])), "Muestra desordenada.")
        muestras.append((n, tiempos, temperaturas))
    return muestras, limites


def graficar_temperaturas(muestras, limites, destino):
    """Compara todos los pares de dos N en paneles con los mismos límites, sin muestreo."""
    figura, ejes = plt.subplots(1, 2, figsize=(11.5, 4.8), sharex=True, sharey=True)
    figura.subplots_adjust(left=0.07, right=0.98, bottom=0.25, top=0.75, wspace=0.3)
    minimo = min(min(m[2]) for m in muestras)
    maximo = max(max(m[2]) for m in muestras)
    margen = max(0.5, (maximo - minimo) * 0.06)
    for eje, (n, tiempos, temperaturas) in zip(ejes, muestras):
        exponente = n.bit_length() - 1
        eje.scatter(tiempos, temperaturas, s=10 if exponente == 15 else 1.2,
                    color="#0072B2", alpha=0.8 if exponente == 15 else 0.4,
                    linewidths=0, rasterized=True)
        cantidad = f"{len(tiempos):,}".replace(",", " ")
        eje.set_title(rf"$N = 2^{{{exponente}}}$" + f" · {cantidad} pares", fontsize=12, loc="left")
        eje.set_xlim(limites[1], limites[2])
        eje.set_ylim(minimo - margen, maximo + margen)
        eje.set_xticks([limites[1], (limites[1] + limites[2]) // 2, limites[2]])
        eje.xaxis.set_major_formatter(FuncFormatter(lambda valor, _: f"{int(valor):,}".replace(",", " ")))
        eje.tick_params(axis="x", labelsize=9)
        eje.set_xlabel("Tiempo Unix (s)", labelpad=9)
        eje.grid(color="#DDE2E6", linewidth=0.7)
        eje.set_axisbelow(True)
    ejes[0].set_ylabel("Temperatura (°C)")
    figura.suptitle("Temperaturas en el mismo rango de una semana", x=0.07, y=0.94,
                    ha="left", fontsize=16, fontweight="bold")
    figura.text(0.07, 0.85, f"Consulta 0 · [{limites[1]}, {limites[2]}] · Muestras del árbol B",
                fontsize=10, color="#59636D")
    figura.text(0.07, 0.07, "Mismos límites en ambos paneles · Cada punto es un par recuperado · Sin agregación ni recorte",
                fontsize=9, color="#59636D")
    guardar(figura, destino, "06_temperaturas")


def main():
    """Recibe la carpeta de una corrida; genera figuras y registra su procedencia local."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("ejecucion", type=Path, help="Carpeta resultados/ejecucion-... completa")
    args = parser.parse_args()
    carpeta = args.ejecucion.resolve()
    series, semilla = leer_mediciones(carpeta)
    muestras, limites = leer_muestras(carpeta.name)
    destino = RAIZ / "graficos" / carpeta.name
    destino.mkdir(parents=True, exist_ok=True)
    estilo()
    graficar_metricas(series, semilla, destino)
    graficar_temperaturas(muestras, limites, destino)
    (destino / "fuentes.txt").write_text(
        f"Ejecución: {carpeta}\nMatplotlib: {matplotlib.__version__}\n"
        f"Semilla: {semilla}\nAlfa Bε: 0.5\n"
        "Métricas: resumen.csv, medias comprobadas contra consultas.csv.\n"
        "Cinco métricas: ejes X log2 e Y log10; valores originales sin suavizado.\n"
        "Tiempo de búsqueda convertido de segundos a milisegundos. Tamaño en nodos.\n"
        "Temperaturas: consulta 0, N=2^15 y 2^26, árbol B; muestras iguales en los tres árboles.\n"
        f"Rango inclusivo: [{limites[1]}, {limites[2]}].\n"
        "Se dibujan todos los pares. Marcadores rasterizados en el PDF de temperaturas.\n"
        "Una sola ejecución; no se mezclan corridas ni se descartan valores atípicos.\n",
        encoding="utf-8")
    print(f"Listo: seis figuras, cada una en PNG y PDF, en {destino}")


if __name__ == "__main__":
    main()
