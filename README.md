
# HPC - Multiplicación de Matrices

Programa en C que multiplica dos matrices cuadradas de enteros aleatorios (0–99) y mide el tiempo de CPU en modo usuario mediante `getrusage()`.

## Compilar y ejecutar manualmente

```bash
gcc BasicMatrixSolver.c -o output && ./output <filas> <columnas>
```

Ejemplo con una matriz 4×4:

```bash
gcc BasicMatrixSolver.c -o output && ./output 4 4
```

La salida es el tiempo de CPU del usuario en segundos (6 decimales).

## Script de testing

`testing.sh` ejecuta el programa 10 veces para cada tamaño de matriz (10, 100, 200, 300, 400, 500) y guarda los tiempos en el archivo `OUTPUT_FILE`.

### Uso

1. Compilar el programa:

   ```bash
   gcc BasicMatrixSolver.c -o output
   ```

2. Dar permisos de ejecución al script (solo la primera vez):

   ```bash
   chmod +x testing.sh
   ```

3. Ejecutar el script:

   ```bash
   ./testing.sh
   ```

Los resultados se escriben en `OUTPUT_FILE`.
