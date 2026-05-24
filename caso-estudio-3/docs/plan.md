# Plan: Despliegue de caso-estudio-3 en AWS con ParallelCluster

## Contexto

El proyecto requiere ejecutar el benchmark de multiplicación de matrices con MPI en un clúster real de mínimo 3 nodos (según el enunciado). Actualmente el código y el script `RunAll.sh` solo ejecutan localmente. Se usará AWS ParallelCluster para crear un clúster HPC administrado con Slurm, 4 nodos compute (`c5.xlarge`, 4 vCPUs cada uno), y sistema de archivos compartido vía NFS.

**Objetivo:** Ejecutar las 440 corridas del benchmark (4 procesos × 10 reps × 11 tamaños) distribuidas entre los 4 nodos y recolectar los CSVs con tiempos de ejecución.

---

## Fase 1 — Prerrequisitos locales

### 1.1 Instalar AWS CLI v2
```bash
curl "https://awscli.amazonaws.com/AWSCLIV2.pkg" -o "AWSCLIV2.pkg"
sudo installer -pkg AWSCLIV2.pkg -target /
aws --version
```

### 1.2 Configurar credenciales AWS
```bash
aws configure
# Pedir: Access Key ID, Secret Access Key, Region (us-east-1), Output format (json)
```
> Las credenciales se obtienen en IAM → Users → Security credentials → Create access key.

### 1.3 Instalar AWS ParallelCluster CLI
```bash
pip3 install "aws-parallelcluster"
pcluster version
```

---

## Fase 2 — Crear par de claves SSH

```bash
aws ec2 create-key-pair \
  --key-name hpc-mpi-key \
  --query 'KeyMaterial' \
  --output text > ~/.ssh/hpc-mpi-key.pem
chmod 400 ~/.ssh/hpc-mpi-key.pem
```

---

## Fase 3 — Archivo de configuración del clúster

Crear `~/hpc-cluster-config.yaml` con el siguiente contenido:

```yaml
Region: us-east-1
Image:
  Os: alinux2

HeadNode:
  InstanceType: t3.medium
  Networking:
    SubnetId: <SUBNET_ID>        # completar en Fase 3.1
  Ssh:
    KeyName: hpc-mpi-key

Scheduling:
  Scheduler: slurm
  SlurmQueues:
    - Name: compute
      ComputeResources:
        - Name: c5xlarge
          InstanceType: c5.xlarge
          MinCount: 3             # Nodos siempre activos (requisito enunciado)
          MaxCount: 4
      Networking:
        SubnetIds:
          - <SUBNET_ID>          # mismo subnet del HeadNode

SharedStorage:
  - MountDir: /shared
    Name: nfs-shared
    StorageType: Efs
    EfsSettings:
      Encrypted: false
```

### 3.1 Obtener un Subnet ID válido
```bash
aws ec2 describe-subnets \
  --query 'Subnets[?State==`available`].[SubnetId,AvailabilityZone,CidrBlock]' \
  --output table
```
Reemplazar `<SUBNET_ID>` en el YAML con el valor obtenido.

---

## Fase 4 — Desplegar el clúster

```bash
pcluster create-cluster \
  --cluster-name hpc-mpi-cluster \
  --cluster-configuration ~/hpc-cluster-config.yaml

# Verificar estado (esperar ~10-15 min hasta CREATE_COMPLETE)
pcluster describe-cluster --cluster-name hpc-mpi-cluster
```

---

## Fase 5 — Subir el código al HeadNode

```bash
# Obtener IP del HeadNode
HEAD_IP=$(pcluster describe-cluster \
  --cluster-name hpc-mpi-cluster \
  --query 'headNode.publicIpAddress' \
  --output text)

# Copiar el proyecto
scp -i ~/.ssh/hpc-mpi-key.pem -r \
  /Users/danieltorosoto/universidad/HPC/caso-estudio-3 \
  ec2-user@$HEAD_IP:/shared/caso-estudio-3

# Conectarse
ssh -i ~/.ssh/hpc-mpi-key.pem ec2-user@$HEAD_IP
```

---

## Fase 6 — Compilar en el HeadNode

```bash
# Dentro del HeadNode
cd /shared/caso-estudio-3
sudo yum install -y openmpi openmpi-devel  # si no viene preinstalado (normalmente ya está en alinux2)
make
# Verifica: output/point_to_point debe existir
ls output/
```

> El directorio `/shared` es NFS compartido, por lo que todos los nodos compute ven el mismo binario compilado automáticamente.

---

## Fase 7 — Adaptar RunAll.sh para el clúster

El único cambio necesario en `scripts/RunAll.sh` es reemplazar `mpirun -n` por `mpirun --hostfile $MPI_HOSTFILE -n` para forzar distribución real entre nodos.

**Cambio en `scripts/RunAll.sh` (línea 77):**
```bash
# ANTES:
mpirun -n "$n" "$ROOT_DIR/output/point_to_point" "$i" "$n"

# DESPUÉS:
mpirun --hostfile /shared/hostfile -n "$n" \
  "$ROOT_DIR/output/point_to_point" "$i" "$n"
```

**Crear el hostfile con los nodos compute:**
```bash
# Obtener nombres de los nodos activos desde Slurm
sinfo -N -h -o "%N" | sort | head -4 > /shared/hostfile
# Verificar que tiene 3-4 líneas con los hostnames
cat /shared/hostfile
```
Formato esperado del hostfile:
```
compute-dy-c5xlarge-1
compute-dy-c5xlarge-2
compute-dy-c5xlarge-3
```

---

## Fase 8 — Ejecutar el benchmark

```bash
# Verificar que los nodos están UP
sinfo

# Solicitar asignación de recursos para todos los nodos (interactivo)
salloc --nodes=4 --ntasks=16 --ntasks-per-node=4 --partition=compute

# Dentro de la asignación, correr el benchmark
cd /shared/caso-estudio-3
bash scripts/RunAll.sh
```

> `RunAll.sh` generará resultados en `/shared/caso-estudio-3/stats/<hostname>/`.

---

## Fase 9 — Recolectar resultados localmente

```bash
# Desde la máquina local
scp -i ~/.ssh/hpc-mpi-key.pem -r \
  ec2-user@$HEAD_IP:/shared/caso-estudio-3/stats \
  /Users/danieltorosoto/universidad/HPC/caso-estudio-3/stats/
```

---

## Fase 10 — Destruir el clúster (evitar costos)

```bash
pcluster delete-cluster --cluster-name hpc-mpi-cluster
```

---

## Archivos a modificar

| Archivo | Cambio |
|---|---|
| `scripts/RunAll.sh` | Línea 77: añadir `--hostfile /shared/hostfile` al comando `mpirun` |
| `~/hpc-cluster-config.yaml` | Archivo nuevo — configuración del clúster (fuera del repo) |

---

## Estimación de costos

| Recurso | Tipo | Precio/hora | Horas | Total |
|---|---|---|---|---|
| HeadNode | t3.medium | $0.042 | ~4h | ~$0.17 |
| 3 Compute nodes | c5.xlarge | $0.17 × 3 | ~4h | ~$2.04 |
| EFS storage | ~1 GB | $0.30/GB-mes | ~4h | ~$0.001 |
| **Total estimado** | | | | **~$2.21** |

---

## Verificación

1. `pcluster describe-cluster` muestra `CREATE_COMPLETE`
2. `sinfo` muestra 3 nodos en estado `idle`
3. Prueba rápida antes del RunAll completo:
   ```bash
   mpirun --hostfile /shared/hostfile -n 4 \
     /shared/caso-estudio-3/output/point_to_point 500 4
   # Debe imprimir un número flotante (tiempo en segundos)
   ```
4. Después de RunAll.sh: existen 4 archivos CSV en `stats/*/` con ~110 valores cada uno
