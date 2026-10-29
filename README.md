# Tarea 2 — Árboles B y Bε

Implementación en C++17 de la Tarea 2 de CC4102, Diseño y Análisis de Algoritmos.
Incluye el árbol B, las dos configuraciones de Bε (ε ≈ 0,25 y ε ≈ 0,75),
pruebas de corrección y el experimento completo de construcción y búsquedas.
Ambos árboles Bε usan α = 0,5. El bonus no está implementado.

## 1. Preparar las herramientas

Se necesita **Linux, GNU Make y g++ con soporte de C++17**. El programa utiliza
únicamente la biblioteca estándar de C++; no requiere librerías externas,
Python ni un entorno virtual.

En Ubuntu/Debian, si faltan las herramientas, instalarlas con:

```bash
sudo apt update
sudo apt install g++ make
```

La compilación y ejecución se comprobaron en Ubuntu 24.04, con GCC 13.3.0 y
GNU Make 4.3. Los archivos binarios utilizan representación nativa little-endian,
con enteros y flotantes de 4 bytes, como en el entorno comprobado.

**Ejecutar todos los comandos siguientes desde la raíz del repositorio**,
es decir, desde la carpeta que contiene este README y el Makefile.

## 2. Colocar los datos

Descargar los datos proporcionados por el equipo docente y colocarlos así:

```text
datos/
├── datos.bin
└── datos_sample.txt
```

- **datos.bin es obligatorio** para los experimentos. Para ejecutar todos los
  tamaños se requieren al menos 2^26 pares; el archivo docente ocupa 512 MiB.
- **datos_sample.txt es opcional**, pero se recomienda incluirlo: si está junto
  al binario, el programa comprueba la lectura de sus primeros 50 registros.

Los datos no se incluyen en el repositorio ni en la entrega. No se necesita
crear manualmente las carpetas de salida.

La última ejecución completa generó aproximadamente **5,7 GiB de árboles**,
además de los CSV. Reservar al menos **7 GiB libres** para una nueva ejecución,
con los datos de entrada ya disponibles. Se mantiene un árbol por vez en RAM;
con poca memoria disponible puede utilizar swap y tardar más.

## 3. Compilar y ejecutar las pruebas

```bash
make
make test
```

`make` crea `build/` y compila el programa `build/tarea2` y los tres ejecutables
de pruebas. `make test` ejecuta las pruebas y muestra sus comprobaciones en
terminal; si alguna falla, termina con error.

Las pruebas cubren almacenamiento en bloques de 4096 bytes, inserciones,
divisiones, vaciados de buffers, búsquedas por rango y exportación de resultados.
Comparan las consultas con filtrado directo de datos sintéticos. **No necesitan
el dataset docente**, no modifican los datos originales y eliminan sus archivos
temporales al terminar.

## 4. Ejecutar la tarea completa

Después de compilar y colocar los datos:

```bash
./build/tarea2
```

Este único comando realiza automáticamente:

1. La construcción de los tres árboles para cada N = 2^15, 2^16, …, 2^26,
   insertando los primeros N pares del dataset en su orden original.
2. El guardado de cada árbol en un archivo binario de bloques de 4096 bytes.
3. La ejecución de 50 búsquedas por rango en cada árbol y tamaño, leyendo
   nodos desde el archivo después de liberar el árbol de construcción.
4. La medición y exportación de tiempos, I/Os, tamaños y pares recuperados
   de una consulta de muestra.

Son **36 construcciones y 1800 consultas**. Todos los árboles y tamaños comparten
los mismos 50 rangos inclusivos de una semana, con semilla 4102. Se realiza una
construcción por árbol y tamaño.

La terminal muestra el avance y un resumen por árbol y N. Al terminar aparece
`Ejecucion terminada: resultados/ejecucion-...`, indicando dónde se guardó la
corrida. Su archivo `estado.txt` debe contener
`completa_para_el_intervalo_solicitado` y `entorno.txt` debe indicar
`alcance=todos_los_tamanos`.

La ejecución completa ya fue realizada con los datos docentes. Su duración
depende del equipo y de la memoria disponible.

### Ejecución parcial opcional

Para comprobar rápidamente el funcionamiento con solo N = 2^15 y N = 2^16:

```bash
./build/tarea2 --min-exp 15 --max-exp 16
```

Esta ejecución también guarda resultados, pero **no reemplaza el experimento
completo**. Para realizarlo después, ejecutar `./build/tarea2` sin opciones.

### Opciones disponibles

```bash
./build/tarea2 --help
```

| Opción | Valor por defecto | Función |
| --- | --- | --- |
| `--datos RUTA` | `datos/datos.bin` | Usar un dataset ubicado en otra ruta. |
| `--min-exp E` | `15` | Primer exponente de N = 2^E. |
| `--max-exp E` | `26` | Último exponente, incluido. |
| `--semilla S` | `4102` | Semilla de los rangos, entero entre 0 y 4294967295. |

Los exponentes deben estar entre 15 y 26, con el mínimo menor o igual al máximo.
Para usar otro dataset, agregar `--datos` seguido de su ruta al comando de
experimentación. Los resultados siempre se guardan en el directorio desde el
que se ejecuta el programa.

## 5. Encontrar e interpretar los resultados

Cada ejecución crea un identificador propio `ejecucion-<marca_temporal>`.
**Volver a ejecutar el programa crea una corrida nueva y conserva las anteriores**;
no hace falta borrar resultados ni volver a compilar si el código no cambió.

| Archivo | Contenido |
| --- | --- |
| `resultados/<id>/resumen.csv` | Una fila por árbol y N: tiempo e I/Os de inserción, nodos, bytes y promedios de búsqueda. Incluye lectura de datos y serialización por separado. |
| `resultados/<id>/consultas.csv` | Mediciones de cada consulta: rango, tiempo, lecturas, escrituras y cantidad de pares recuperados. |
| `resultados/<id>/rangos.csv` | Los 50 rangos inclusivos, identificados de 0 a 49. |
| `resultados/<id>/entorno.txt` | Parámetros, alcance, compilador, sistema operativo, CPU, RAM y cachés. |
| `resultados/<id>/estado.txt` | Indica si la ejecución está en curso, incompleta o terminada para el intervalo solicitado. |
| `resultados/muestras/<id>/<arbol>_<N>.csv` | Todos los pares tiempo–temperatura de la consulta 0, para cada árbol y N. |
| `arboles_generados/<id>/<arbol>_<N>.bin` | Árboles serializados, con la raíz en el primer bloque. |

`<id>` representa el identificador de la corrida. Los nombres de árbol son
`B`, `BE025` y `BE075`. Las columnas terminadas en `_s` expresan segundos;
`tamano_bytes`, bytes; y las temperaturas, grados Celsius.

Los promedios se calculan sobre las 50 consultas de cada árbol y N. Las I/Os
son accesos lógicos a nodos: en inserción se cuentan lecturas y escrituras
simuladas, y en búsqueda se cuentan bloques leídos. No equivalen necesariamente
a transferencias físicas de disco, porque no se vacían las cachés del sistema.

El tiempo de inserción incluye crear la raíz e insertar los pares; excluye la
lectura del dataset y la serialización, que se miden aparte. El tiempo de búsqueda
incluye recuperar y ordenar los pares, pero excluye abrir el archivo. Las
impresiones y exportaciones CSV quedan fuera de los intervalos medidos.

Si la ejecución falla, el programa muestra `Error: ...` y termina con código 1.
Una interrupción puede dejar archivos parciales y un estado `en_curso` o
`incompleta`. Esa corrida no debe considerarse completa; ejecutar nuevamente
el comando inicia otra desde el comienzo, con un identificador nuevo.

## Organización del código

| Ubicación | Contenido |
| --- | --- |
| `src/main.cpp` | Entrada del programa y opciones de ejecución. |
| `src/experimentos.hpp`, `src/experimentos.cpp` | Lectura de datos, construcción, consultas, mediciones y exportación CSV. |
| `src/arbol_b.hpp`, `src/arbol_b.cpp` | Árbol B: inserción, divisiones y búsqueda por rango. |
| `src/arbol_be.hpp` | Árbol Bε parametrizado para las dos configuraciones. |
| `src/arbol.hpp` | Interfaz compartida de los árboles. |
| `src/registro.hpp`, `src/nodo_b.hpp`, `src/nodo_be.hpp` | Registros y estructuras de nodos. |
| `src/almacenamiento.hpp`, `src/serializacion.hpp`, `src/metricas.hpp` | Almacenamiento en RAM y disco, bloques de 4096 bytes y contadores de I/Os. |
| `tests/` | Pruebas de almacenamiento, árboles y experimentos. |
| `datos/` | Ubicación de los datos de entrada. |
| `Makefile` | Compilación, ejecución de pruebas y limpieza de ejecutables. |

## Limpiar los ejecutables

```bash
make clean
```

Elimina únicamente el programa y los tres ejecutables de pruebas producidos
por Make. Conserva los datos, resultados y árboles generados. Para volver a
compilar, ejecutar `make`.

Los comandos de compilación, pruebas, ayuda y ejecución parcial se han verificado,
así como la ejecución completa con los datos docentes. La instalación mediante
`apt` se proporciona para equipos sin las herramientas; no fue necesario
repetirla en el entorno de trabajo.

## Detalles de implementación

### Almacenamiento compartido

Las clases son plantillas parametrizadas por `NodoB`, `NodoBE025` o `NodoBE075`.

| Operación | Comportamiento y conteo |
| --- | --- |
| Crear `AlmacenamientoRAM<Nodo>` | Crea una hoja raíz en el índice 0; cuenta una escritura. |
| `leer(indice)` en RAM | Devuelve una copia del nodo y cuenta una lectura. |
| `escribir(indice, nodo)` | Reemplaza un nodo existente y cuenta una escritura. |
| `agregar(nodo)` | Agrega al final, cuenta una escritura y devuelve el índice. |
| `guardar(ruta)` | Exporta los nodos secuencialmente; devuelve un contador separado con una lectura de RAM y una escritura a archivo por nodo. |
| Crear `ArchivoNodos<Nodo>(ruta)` | Abre un archivo existente y determina su cantidad de nodos, sin cargar los bloques. |
| `leer(indice)` desde archivo | Lee 4096 bytes en `indice × 4096` y cuenta una lectura. |
| `metricas()` | Devuelve una copia de los contadores de esa instancia. |
| `reiniciar_metricas()` en archivo | Reinicia los contadores para medir una nueva consulta. |

El vector es privado. Modificar la copia devuelta por `leer` no actualiza el
almacenamiento: hay que llamar a `escribir`. Los contadores registran operaciones
completadas; los índices inválidos no cuentan como accesos. Las métricas de
construcción no cambian al exportar. Las reasignaciones internas del vector no
cuentan como I/Os, pues son operaciones de memoria principal.

Cada archivo contiene únicamente bloques de nodos, sin cabecera; el primero
es la raíz. `guardar` requiere que la carpeta de destino exista y reemplaza
el archivo indicado. El lector no conserva un vector ni una caché de nodos:
cada llamada recupera un bloque, incluso si ya fue leído. El conteo corresponde
a accesos lógicos del programa, no a transferencias físicas del dispositivo.

La serialización usa la representación binaria nativa de las estructuras, cuyo
tamaño y disposición están comprobados. El archivo debe abrirse con el mismo
tipo de nodo y una representación compatible (orden de bytes y disposición de
campos); no contiene metadatos para identificar su configuración. Este formato
está destinado a los experimentos locales de la tarea.

### Árbol B

`ArbolB` implementa la interfaz `Arbol`: `insertar(Registro)`, `guardar(ruta)`,
`metricas()` y `cantidad_nodos()`. Cada inserción recibe un par; no hay borrado.
La división promueve el par de la posición 170 (contando desde uno), dejando
169 pares a la izquierda y 170 a la derecha. La raíz siempre ocupa el índice 0
y puede contener menos pares que el mínimo exigido a los demás nodos.

Las copias locales ya leídas se reutilizan durante una inserción: no se cuenta
otra lectura por pasarlas a una función. Todas las lecturas y escrituras del
vector pasan por el almacenamiento compartido.

Para consultar se abre `ArchivoNodos<NodoB>` con el binario guardado y se llama
a `ArbolB::buscar_rango(archivo, l, u)`, con `l <= u`. Devuelve un vector de pares
ordenados por llave, incluidos los almacenados en nodos internos; ambos límites
son inclusivos. La función es estática y no necesita conservar el árbol en RAM.
El lector puede reutilizarse entre consultas: `archivo.reiniciar_metricas()`
reinicia el conteo y `archivo.metricas()` devuelve las lecturas de bloques.

### Árbol Bε

`ArbolBE<C, LMax>` comparte la interfaz de construcción y el almacenamiento con B.
Se utilizan `ArbolBE025` (C = 4, LMax = 506) y `ArbolBE075` (C = 126, LMax = 384).
Ambas configuraciones usan `alfa = 0.5`, accesible como constante de la clase para
registrarlo en los resultados de la experimentación.

Las inserciones entran al buffer de la raíz. Cuando falta espacio, el vaciado
agrupa los pares por hijo y transfiere los grupos de tamaño al menos
`ceil(ceil(k_buffer / (k_llaves + 1)) * alfa)`, de derecha a izquierda.
Los buffers se mantienen ordenados. En hojas el par mediano permanece en la
izquierda; los internos solo promueven la llave separadora y reparten sus buffers.

Se aplican estas decisiones para resolver las transferencias por lotes:

- Si una hoja parcialmente llena no puede recibir todo el lote, se completa
  con una parte, se divide llena y se continúa con los pares restantes.
- Si un interno alcanza C separadores durante un vaciado, se interrumpe ese
  vaciado y se propaga su división. Los pares pendientes se conservan en los
  buffers de las mitades; se reagrupan al necesitar espacio para continuar.
  Nunca se agrega un separador fuera de la capacidad fija del nodo.

`guardar(ruta)` conserva los buffers tal como están; no fuerza su vaciado.
Para consultar se abre `ArchivoNodos<NodoBE025>` o `ArchivoNodos<NodoBE075>` y se
llama a `ArbolBE025::buscar_rango(archivo, l, u)` o a su equivalente `ArbolBE075`.
Con `l <= u`, recupera los pares de hojas y buffers internos en el rango inclusivo,
sin tratar los separadores como datos. Ordena el resultado por llave antes de
devolverlo; ese trabajo forma parte de la consulta. La instancia en RAM puede
destruirse antes de consultar, y las métricas de lectura permanecen en el lector.
