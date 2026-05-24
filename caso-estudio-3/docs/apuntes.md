2# Cluster Computing:
- Hacer con conexion Ethernet.
- Cluster middleware se encarga de repartir los procesos.
- Se usa MPI, no threads.
- Se utilizara LAN.
### SSI Middleware
Simula una sola maquina, pero que trabajan de manera conjunta. Es una imagen del sistema como si fuera una solo maquina.

Para middleware: PVM(Parallel virtual machine).
### RRS Middleware
Usuario tiene conocimeinto que es un cluster. 
### Cluster Programming Models
Vamos a usar MPI como midleware. Hecho para C, no necesita el RRS ni el SSI. No tendra sistema de administracion, pero se garantiza la conexion por medio de esta API.
### Video explicativo
[Cluster in AWS](https://www.youtube.com/watch?v=Ls2rtHtGxCA)

# MPI (Message Passing Interface)

[Forum de MPI](https://www.mpi-forum.org/), utilizar la version 5.0 con el estandar 5.0.  Comunicación entre procesos, no estre hilos. Lo que implementamos es la generacion de procesos en multiples recursos computacionales. 

Estamos en una arquitectura de memoria distribuida. Los procesos se comunican entre si por medio de procesos de envio y recepcion que son llamadas de las librerias. MPI nos provee y garantiza la sincronizacion y movimineto de datos entre los procesos.

Trabaja UDP no TCP. NO existe acuse de recibido.

- **Comunicator**: Grupo de proceoss que se puede comunicar entre ellos.
- **Rank**: Id de los procesos, int, dentro de cada communicator.
- **Tag**: Da un orden a la los mensajes.
## Point to point
Un sender y un receiver:

[MPI tutorials](https://github.com/mpitutorial)
```
#include <mpi.h>
#include <stdio.h>

int main(int argc, char** argv) {
  // Initialize the MPI environment. The two arguments to MPI Init are not
  // currently used by MPI implementations, but are there in case future
  // implementations might need the arguments.
  MPI_Init(NULL, NULL);

  // Get the number of processes
  int world_size;
  MPI_Comm_size(MPI_COMM_WORLD, &world_size);

  // Get the rank of the process
  int world_rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

  // Get the name of the processor
  char processor_name[MPI_MAX_PROCESSOR_NAME];
  int name_len;
  MPI_Get_processor_name(processor_name, &name_len);

  // Print off a hello world message
  printf("Hello world from processor %s, rank %d out of %d processors\n",
         processor_name, world_rank, world_size);

  // Finalize the MPI environment. No more MPI calls can be made after this
  MPI_Finalize();
}
```

El Makefile seria algo asi:
```
EXECS=mpi_hello_world
MPICC?=mpicc

all: ${EXECS}

mpi_hello_world: mpi_hello_world.c
	${MPICC} -o mpi_hello_world mpi_hello_world.c

clean:
	rm -f ${EXECS}
```

NO es que exista un compilador MPICC diferente a gcc, solo que se utiliza eso como enlace simbolico que apunta a gcc apuntando las librerias necesarias.

Otro ejemplo con el uso de [ranks](https://github.com/mpitutorial/mpitutorial/tree/gh-pages/tutorials/mpi-send-and-receive/code).

**Para la ejecucion:** 
```
mpirun -np 10 myprog
```
Donde -np 10 esta declarando que son 10 procesos.

## Collective Communication

Envuelve un grupo de procesos, donde la llamada que se hace a este grupo se hace por un communicator. 

Las acciones a un comunicator se traduce a todos sus procesos. Una vez N procesos en un comunicator, todos los N seran llamados con este comunicator.

En este no hay tags. Las collective operations bloquean.

- **MPI_Barrier:** Hace una barrera bloqueante para impedir que el proceso se siga ejecutando hasta que el resto de procesos terminen.
 - **MPI_Broadcast:** Hace un broadcast xd. Sirve para mandar info a todos los procesos, PERO no es la mejor opcion.
 - **Scatter**: Permite coger un dato (no escalar), y lo distribuye en chucks. Asignándole uno de esos chucks a cada uno de los procesos del comunicador,
 ![[Pasted image 20260519174011.png]]

**Gather:** Se encarga de traer la información de los chucks de los procesos. Es recoger la info que Scatter distribuyo.

![[Pasted image 20260519174413.png]]

- **Global Reduction Operations** - **MPI_Reduce** : Ayudan a hacer reducción.  (si se necesita sumar o operaciones logicas dentro de los datos vectoriales).

![[Pasted image 20260519175238.png]]
![[Pasted image 20260519180210.png]]

- **MPI_Allreduce:** Mismo que MPI_Reduce, solo que este no tiene root, osea que esta reduccion, todos reciben el dato reducido. Este es el optimizado.

- **MPI_Reduce_scatter:** Reduce y hace scatter.

- **MPI_Scan:** Se hacen reducciones acumulativas.
-![[Pasted image 20260519180237.png]]


```
MY ANTI DEPRESSANTS JUST KICKED
                 IN ! FANTASTIC !
                   ______________________
                ╱    /\__/\       //     ╲╲
        ______⊂╱    ( ´∇`  )     // ⊃     ||╲  フ 🡖
      ,´__▔▔▔▔   ▔     ⌒▔▔▔▔╱▔▔▔▔ 🡖▔ ▔▔▔▔▔🡖 ▔▔▔▔ |
    ,╱_ _╱   /-o— /    ╱▔▔╱ ___/\  |     ▔ | /\__|
   ,========————´=============/⌒ ╲=/=======||🡖 ||
   | __  |  YEI!  |   __ "  |⌒| |/    ___/|  )╯
  (|🞕|_∈≡≡≡≡≡≡≡≡≡∋_|🞕|"  __|| ╯ ╯__ -‒‒‒‒‒┘  ╯
   ▔╲ ▔╲__╯▔ ▔▔▔▔▔▔▔三三三▔╲  ╲__╯ ▔▔  三三三三╯

```

## Consideraciones

Cuando son grandes cantidades de datos nose arian Broadcasts. NFS (Network Fily System), por medio de softawre se comparten carpetas donde los otros nodos actuan como clientes.

# Caso 3

Para el desarrollo de la presente actividad se estima una dedicación de 8 horas totales. 

Consiste en realizar la implementación de una solución utilizando MPI sobre el caso de estudio de Multiplicación de Matrices. Realizar pruebas de desempeño y mostrar los resultados en documento. Recuerden que para este experimento en cuanto a las pruebas es necesario que estas sean realizadas sobre cluster computacional (minimo 3 nodos de computo).

Para el speed up podemos utlizar tanto con caso base de 1 nodo o con el monolitico. 

La modificacion con paralelizacion con MPI se hace en el secuecial de multiplicación de matrices.

Se recomienda utlizar el mismo esquema de pruebas que se hizo con los otros casos de estudio.

**Implementacion**: OpenMPI, ya que es mas fácil de instalar en linux. 

**Infraestructura del Cluster**: Google Cloud o AWS. Se podria hacer mixto pero toca hablar con el profe si se quiere hacer un GRID.

**Pruebas**: (Creeria que hacerla 10 veces como la otra vez?)
- Num de clusters.
- Tam de matrices.
- heterogeneo vs homogeneo (MPI se supone debe garantizar que en la comunicacion se comporten igual).

**Data** que se requiere por prueba:
- Tiempo que toma en hacerse cada una.
- MPI ya tiene mecanismo para hacer la captura del timestamp.

 **TIP: No utlizar la medicion de tiempo que hemos utilizado, ya que estas estan hechas para trabajar sobre el mismo reloj.  Como hay equipos diferente, cada HW de reloj sera diferente.**