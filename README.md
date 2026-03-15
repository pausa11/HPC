# HPC - Multiplicación de Matrices

Programa en C que multiplica dos matrices cuadradas de enteros aleatorios (0–99) y mide el tiempo de CPU en modo usuario mediante `getrusage()`.

## Estructura del proyecto

```
src/
├── SecuentialMatrixSolver.c
├── MemoryMatrixSolver.c
├── ThreadsMatrixSolver.c
└── MultiprocessingMatrixSolver.c
output/
scripts/
└── RunAll.sh
```

## Compilar

### Secuencial
```bash
gcc src/SecuentialMatrixSolver.c -o output/secuential
```

### Memory
```bash
gcc src/MemoryMatrixSolver.c -O3 -o output/memory
```

### Threads
```bash
gcc src/ThreadsMatrixSolver.c -o output/threads -lpthread
```

### Multiprocessing
```bash
gcc src/MultiprocessingMatrixSolver.c -o output/multiprocessing
```

## Ejecutar manualmente

```bash
./output/<variante> <filas> <columnas>
```

Ejemplo con una matriz 4×4:

```bash
./output/secuential 4 4
```

La salida es el tiempo de CPU del usuario en segundos (6 decimales).

## Script de testing

`scripts/RunAll.sh` ejecuta todos los programas para cada tamaño de matriz y guarda los tiempos en sus respectivos archivos de salida.

### Uso

1. Compilar todos los programas (ver sección **Compilar** arriba).

2. Dar permisos de ejecución al script (solo la primera vez):

   ```bash
   chmod +x scripts/RunAll.sh
   ```

3. Ejecutar el script:

   ```bash
   ./scripts/RunAll.sh
   ```

## Utilidades

### Ver los threads del proceso en Linux
```bash
top -H -p $(pgrep output)
```