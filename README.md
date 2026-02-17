# Proyecto de Computación de Alto Rendimiento (HPC) en C

Este repositorio contiene implementaciones de algoritmos y soluciones optimizadas para computación de alto rendimiento utilizando el lenguaje C. El objetivo principal es maximizar la eficiencia computacional y el aprovechamiento de recursos de hardware modernos.

## 🚀 Tecnologías Utilizadas

- **Lenguaje C**: Utilizado por su bajo nivel y eficiencia en el manejo de memoria.
- **OpenMP**: Para paralelismo de memoria compartida mediante directivas de compilador.
- **MPI (Message Passing Interface)**: Para paralelismo de memoria distribuida en clusters.
- **POSIX Threads (pthreads)**: Para un control granular del multi-threading.
- **SIMD (Single Instruction, Multiple Data)**: Uso de extensiones como AVX2 o AVX-512 para paralelismo a nivel de datos.

## 🛠 Técnicas de Optimización

Se han aplicado diversas técnicas para reducir los cuellos de botella y mejorar el rendimiento:

1.  **Paralelismo Multi-nivel**: Combinación de MPI para la comunicación entre nodos y OpenMP para el paralelismo interno en cada nodo.
2.  **Optimización de Caché**:
    - Técnicas de *Loop Tiling* (Bloqueo) para mejorar la localidad temporal y espacial.
    - Acceso a memoria *stride-1* para maximizar la eficiencia del prefetcher.
3.  **Vectorización**: Uso de intrínsecos y pragmas para que el compilador genere instrucciones vectoriales eficientes.
4.  **Reducción de Overhead**: Minimización de las regiones críticas y uso de algoritmos *lock-free* donde es posible.
5.  **Perfilamiento (Profiling)**: Uso de herramientas como `gprof`, `Valgrind` y `Intel VTune` para identificar puntos críticos del código.

## 📦 Instalación y Compilación

### Requisitos

- Compilador GCC (versión 9+) o Intel OneAPI (icc/icx).
- Implementación de MPI (OpenMPI o MPICH).
- `make` para la automatización de la compilación.

### Compilación

Para compilar el proyecto utilizando el `Makefile` incluido:

```bash
make
```

### Ejecución

Para ejecutar una versión con OpenMP (ajustando el número de hilos):

```bash
export OMP_NUM_THREADS=4
./build/hpc_app
```

Para ejecutar con MPI:

```bash
mpirun -np 4 ./build/hpc_app
```

## 📊 Medición de Rendimiento

El proyecto incluye scripts para medir el **Speedup** y la **Eficiencia** de las soluciones, permitiendo analizar la escalabilidad tanto fuerte (*strong scaling*) como débil (*weak scaling*).

---
*Desarrollado para la exploración de arquitecturas avanzadas y computación científica.*


# Como compilar lo que hizo WalviZ

## Manualmente
```bash 
```
gcc BasicMatrixSolver.c -o output && ./output 4 4 
```
```

## Correr es sh de testing

Solo para para hacer el sh ejecutable, correr :
```bash 
```
chmod +x testing.sh
```
```
```

```
Una vez creado el ejecutable, de ahora en adelante solo se corre:

```bash 
```
./testing.sh
```
```
