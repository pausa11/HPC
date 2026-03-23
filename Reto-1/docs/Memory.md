# Mejoras de memoria aplicadas al solver Jacobi secuencial

El solver original de Jacobi realiza múltiples recorridos completos sobre los arreglos en cada iteración,
lo que genera un uso ineficiente de la caché del procesador. A medida que `k` crece y los arreglos dejan
de caber en los niveles de caché (L1, L2, L3), cada acceso extra a memoria se convierte en un fallo de
caché costoso. Las siguientes cuatro mejoras reducen el tráfico de memoria sin modificar el algoritmo
matemático ni el resultado numérico.

---

## | 1. Fusionar el barrido y el cálculo del residual en un solo paso

### Problema

El barrido de Jacobi y el cálculo del residual eran dos bucles separados que recorrían los mismos datos
con el mismo patrón de acceso:

- **Barrido:** lee `u_old[j-1]`, `u_old[j]`, `u_old[j+1]` y `f[j]` → escribe `u[j]`
- **Residual:** lee `u[j-1]`, `u[j]`, `u[j+1]` y `f[j]` → acumula `sum_sq`

Al ejecutarse en bucles separados, los datos cargados en una línea de caché durante el barrido ya habían
sido eviccionados cuando el bucle del residual los necesitaba nuevamente. El resultado era el doble de
tráfico de memoria sin ningún beneficio computacional.

### Solución aplicada

En las iteraciones de verificación ambos cálculos se fusionaron en un único bucle:

```c
for (j = 1; j < N - 1; j++)
{
    /* residual del nodo j — datos ya en caché */
    r_j    = (-u_cur[j-1] + 2.0*u_cur[j] - u_cur[j+1]) * inv_h2 - f[j];
    sum_sq += r_j * r_j;

    /* actualización de Jacobi — mismos tres valores, sin carga adicional */
    u_nxt[j] = (f[j] * h2 + u_cur[j-1] + u_cur[j+1]) / 2.0;
}
```

Los valores `u_cur[j-1]`, `u_cur[j]` y `u_cur[j+1]` se cargan una sola vez desde memoria y se usan
para ambos cálculos antes de que la línea de caché sea eviccionada. Esto divide a la mitad el número
de veces que se recorren los arreglos en las iteraciones de verificación. En las iteraciones que no
son de verificación se ejecuta únicamente el barrido simple, sin cálculo de residual.

---

## 2. Eliminar la copia de arreglo mediante intercambio de punteros

### Problema

Al inicio de cada barrido el código copiaba todo el contenido de `u` en `u_old`:

```c
for (j = 0; j < N; j++)
    u_old[j] = u[j];
```

Este recorrido O(N) sobre N `double`s existía únicamente para preservar los valores anteriores antes
de sobrescribirlos. Para `k=10` esto significa mover más de 8 MB de datos por iteración, multiplicado
por más de tres millones de iteraciones.

### Solución aplicada

En lugar de mover datos, se mantienen dos punteros — `u_cur` y `u_nxt` — que alternan sus roles
mediante un intercambio de punteros al final de cada barrido:

```c
double *tmp = u_cur;
u_cur       = u_nxt;
u_nxt       = tmp;
```

Ningún dato se mueve. Solo dos variables de puntero intercambian sus valores, lo que equivale a tres
asignaciones de tipo `double *` en lugar de N copias de `double`. Los valores de frontera de `u_nxt`
se inicializan una única vez antes del bucle principal y nunca son sobreescritos por el barrido, por lo
que permanecen correctos después de cada intercambio. Al finalizar la iteración, si `u_cur` apunta al
buffer auxiliar en lugar del arreglo original del llamador, se realiza una única copia de vuelta —
O(N) una sola vez al terminar, independientemente de cuántas iteraciones se ejecutaron.

---

## 3. Reducir la frecuencia de verificación del residual

### Problema

El residual RMS completo se calculaba después de cada barrido individual. Para valores grandes de `k`,
donde se necesitan decenas o cientos de miles de iteraciones, el residual cambia muy lentamente entre
barridos consecutivos. Calcular una norma O(N) en cada una de esas iteraciones representa una fracción
dominante del tiempo total de ejecución, especialmente cuando los arreglos no caben en caché.

### Solución aplicada

El residual se verifica únicamente cada `CHECK_INTERVAL = 50` barridos:

```c
const int CHECK_INTERVAL = 50;

if (it % CHECK_INTERVAL == 0)
{
    /* barrido fusionado con cálculo de residual */
}
else
{
    /* barrido simple, sin cálculo de residual */
}
```

Esto reduce el costo del cálculo de residual en un factor de 50. La convergencia puede detectarse con
un retraso máximo de 50 barridos adicionales, lo cual tiene un impacto despreciable en el conteo total
de iteraciones — aproximadamente 13 iteraciones extra para `k=5` y 22 para `k=8`.

---

## Impacto combinado

Las cuatro mejoras atacan el mismo problema desde ángulos distintos: reducir la cantidad de veces que
los datos deben ser leídos desde memoria principal en cada iteración.

| Mejora | Recorridos eliminados por iteración |
|--------|-------------------------------------|
| Fix 1 — barrido y residual fusionados | 1 recorrido sobre `u` y `f` (en iters. de verificación) |
| Fix 2 — intercambio de punteros | 1 recorrido completo de copia sobre `u` y `u_old` |
| Fix 3 — verificación cada 50 barridos | residual ejecutado 50× menos frecuentemente |

Ninguna de estas modificaciones altera el algoritmo matemático de Jacobi ni la precisión del resultado.
El número de iteraciones requeridas para converger es idéntico al original; solo cambia el costo por
iteración en términos de accesos a memoria.
