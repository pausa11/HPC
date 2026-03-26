# Análisis de rendimiento: versión con hilos vs versión secuencial

Al ejecutar ambas versiones para valores de k del 1 al 9, la versión con hilos resulta
sustancialmente más lenta que la versión secuencial. Este documento explica las causas
técnicas de ese comportamiento y por qué el problema es inherente al algoritmo y no al
código implementado.

---

## Parte 1 — Por qué la versión con hilos es más lenta

### 1. El trabajo por hilo es demasiado pequeño

Para valores pequeños de k, el número de nodos interiores es muy reducido:

| k | N | Nodos interiores | Nodos por hilo (4 hilos) |
|---|---|---|---|
| 2 | 5 | 3 | ~1 |
| 5 | 33 | 31 | ~8 |
| 7 | 129 | 127 | ~32 |
| 9 | 513 | 511 | ~128 |

La aritmética real que realiza cada hilo por barrido es un puñado de sumas y una
división — trabajo que toma nanosegundos. Crear hilos, esperar en barreras y destruirlos
toma microsegundos — órdenes de magnitud más que el trabajo que realizan. Se paga un
costo fijo muy alto para paralelizar una tarea trivialmente pequeña.

---

### 2. El overhead de las barreras se multiplica con el número de iteraciones

Este es el problema más dañino para este caso específico. Cada iteración de Jacobi
requiere **cuatro sincronizaciones de barrera**. Para k=9 eso significa:

```
792,553 iteraciones × 4 barreras = 3,170,212 cruces de barrera
```

Cada llamada a `pthread_barrier_wait()` involucra una syscall al kernel, tráfico de
coherencia de caché entre núcleos y una variable de condición internamente. Incluso si
cada barrera cuesta solo 1 microsegundo, eso representa más de 3 segundos de overhead
de sincronización puro, sumado al costo de la computación real. La versión secuencial
tiene costo de sincronización cero.

---

### 3. El problema es inherentemente secuencial en el tiempo

Cada barrido de Jacobi depende completamente del resultado del barrido anterior. No es
posible comenzar el barrido `n+1` hasta que todos los nodos del barrido `n` estén
terminados. Eso es exactamente lo que las barreras garantizan. Por lo tanto, los hilos
solo ayudan *dentro* de un único barrido — no pueden solapar barridos entre sí. Para un
problema 1D donde cada barrido ya es rápido, este paralelismo intra-barrido no es
suficiente para compensar el costo de sincronización entre barridos.

---

### 4. False sharing en líneas de caché

Los arreglos `u`, `u_old` y `f` se almacenan de forma contigua en memoria. Una línea de
caché contiene 8 valores `double` consecutivos (64 bytes). Cuando el hilo 0 termina de
escribir `u[8]` y el hilo 1 lee `u_old[8]`, ambos valores están en la misma o en líneas
de caché adyacentes. El protocolo de coherencia de caché del procesador obliga a los
núcleos a coordinar la propiedad de esas líneas aunque estén accediendo a índices
distintos. Esto se denomina **false sharing** y añade latencia invisible en cada frontera
entre rangos de hilos.

---

### 5. El problema es 1D — el cuello de botella es el ancho de banda de memoria

El barrido de Jacobi realiza casi ninguna aritmética — tres sumas y una división por
nodo. El cuello de botella es cargar `u_old[j-1]`, `u_old[j]` y `u_old[j+1]` desde
memoria. Múltiples hilos no aumentan el ancho de banda de memoria — comparten el mismo
bus de memoria. Por lo tanto, cuatro hilos leyendo distintas partes del mismo arreglo
simultáneamente compiten por el mismo recurso en lugar de ejecutarse de forma
independiente.

---

### 6. La creación y unión de hilos se paga en cada llamada

En esta implementación, todos los hilos se crean dentro de `jacobi()` y se unen antes
de que la función retorne. Eso significa que `pthread_create` y `pthread_join` se llaman
en cada invocación. La creación de hilos involucra asignación de recursos a nivel del
kernel — asignación de pila, registro en el planificador — lo cual es costoso en relación
al trabajo que se realiza.

---

## Parte 2 — El problema es del algoritmo, no del código

El código es correcto. Los conteos de iteraciones coinciden perfectamente entre la versión
secuencial y la versión con hilos, las barreras están ubicadas correctamente y no existen
condiciones de carrera. La pérdida de rendimiento no es un error — es una propiedad
fundamental del algoritmo aplicado a esta forma de problema específica.

---

### La estructura de costo propia del algoritmo

El propio documento de Burkardt identifica esto en la Sección 11. Jacobi sobre una
malla 1D tiene un costo total de **O(N³)** — cada vez que N se duplica, las iteraciones
se cuadruplican. Esto no está causado por una implementación deficiente. Es la tasa de
convergencia matemática del método de Jacobi para esta clase de problema. Ninguna
cantidad de hilos corrige O(N³) — solo se puede dividir el factor constante, no cambiar
el exponente.

La razón por la que la convergencia es tan lenta es física. En el primer barrido, el
nodo `j` solo "conoce" sus dos vecinos inmediatos. La información sobre las condiciones
de frontera se propaga exactamente **un nodo por barrido**. Para una malla de N nodos,
una perturbación en un extremo necesita N barridos solo para llegar al otro extremo.
Esto se denomina la **velocidad de propagación** del método, y está intrínsecamente
limitada a un espaciado de malla por iteración en el caso de Jacobi.

---

### Por qué los hilos no pueden solucionarlo

Los hilos dividen el trabajo de cada barrido entre los núcleos disponibles. Pero no
cambian el número de barridos requeridos. Para k=9 aún se necesitan ~792,000 barridos
independientemente de si se usa 1 hilo o 16. Los hilos pueden dar como máximo un factor
de aceleración igual al número de núcleos — no pueden reducir el conteo de iteraciones.

Y como se observó en las pruebas, para este problema el overhead de sincronización entre
barridos cuesta más de lo que el paralelismo ahorra dentro de cada barrido. Por lo tanto
los hilos no solo no dan la aceleración esperada — empeoran el rendimiento.


---

## Resumen

| Causa | Tipo | Solución real |
|---|---|---|
| Trabajo por hilo demasiado pequeño | Escala del problema | Problema 2D/3D o N mucho mayor |
| Overhead de barreras × iteraciones | Sincronización | Reducir frecuencia de sync |
| Dependencia temporal entre barridos | Algoritmo | Cambiar a Multigrid |
| False sharing en fronteras de hilos | Hardware | Cache blocking |
| Cuello de botella en ancho de banda | Hardware | Problema no es compute-bound |
| Creación de hilos por invocación | Implementación | Thread pool persistente |

La conclusión es que la versión con hilos implementada es correcta. La pérdida de
rendimiento observada es la consecuencia esperada de aplicar paralelismo a un algoritmo
cuyo costo es O(N³) y cuya unidad de trabajo paralelizable — un solo barrido sobre N
nodos 1D — es demasiado pequeña para amortizar el overhead de coordinación entre hilos.