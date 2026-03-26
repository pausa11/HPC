#!/usr/bin/env python3
"""
Genera las gráficas de benchmarking para el Reto-1 (Jacobi 1D) a partir de
los CSV en stats/kali/.  Guarda los PNG en docs/charts/.
"""

import os
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker

# ─── Rutas ────────────────────────────────────────────────────────────────────
SCRIPT_DIR  = os.path.dirname(os.path.abspath(__file__))
ROOT_DIR    = os.path.abspath(os.path.join(SCRIPT_DIR, '..', '..'))
STATS_DONE  = os.path.join(ROOT_DIR, 'stats', 'kali', '20260325_115342.done')
STATS_MP    = os.path.join(ROOT_DIR, 'stats', 'kali', '20260325_192127_multiprocessing_quick')
CHARTS_DIR  = os.path.join(ROOT_DIR, 'docs', 'charts')
os.makedirs(CHARTS_DIR, exist_ok=True)

# ─── Lectura de CSVs ──────────────────────────────────────────────────────────
def read_times(filepath):
    """Devuelve lista de listas con los tiempos de cada fila del CSV."""
    rows = []
    with open(filepath) as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            parts = [x for x in line.split(',') if x.strip()]
            times = [float(parts[i]) for i in range(0, len(parts), 2)]
            rows.append(times)
    return rows

def avg(rows):
    n = min(len(r) for r in rows)
    return np.array([np.mean([r[i] for r in rows]) for i in range(n)])

# ─── Cargar datos ─────────────────────────────────────────────────────────────
sec   = avg(read_times(os.path.join(STATS_DONE, 'secuential.csv')))
flag  = avg(read_times(os.path.join(STATS_DONE, 'secuential_flag.csv')))
mem   = avg(read_times(os.path.join(STATS_DONE, 'memory.csv')))
t2    = avg(read_times(os.path.join(STATS_DONE, 'threads2.csv')))
t4    = avg(read_times(os.path.join(STATS_DONE, 'threads4.csv')))
t8    = avg(read_times(os.path.join(STATS_DONE, 'threads8.csv')))
t16   = avg(read_times(os.path.join(STATS_DONE, 'threads16.csv')))
mp2   = np.array(avg(read_times(os.path.join(STATS_MP, 'multiprocessing2.csv'))))
mp4   = np.array(avg(read_times(os.path.join(STATS_MP, 'multiprocessing4.csv'))))
mp8   = np.array(avg(read_times(os.path.join(STATS_MP, 'multiprocessing8.csv'))))
mp16_rows = read_times(os.path.join(STATS_MP, 'multiprocessing16.csv'))
mp16  = np.array(avg(mp16_rows))   # solo k=1..8

k_all  = np.arange(1, 12)          # k = 1..11
k_mp16 = np.arange(1, len(mp16)+1) # k = 1..8

N_all  = 2**k_all  + 1
N_mp16 = 2**k_mp16 + 1

plt.rcParams.update({'font.size': 11, 'figure.dpi': 150})

# ─── 1. Comparación variantes secuenciales ────────────────────────────────────
fig, ax = plt.subplots(figsize=(9, 5))
ax.semilogy(k_all, sec,  'o-',  color='#e74c3c', label='Secuencial (sin flags)')
ax.semilogy(k_all, flag, 's--', color='#3498db', label='Secuencial con -O2')
ax.semilogy(k_all, mem,  '^-.', color='#2ecc71', label='Optimización de acceso')
ax.set_xlabel('Índice de malla $k$  ($N = 2^k+1$ nodos)')
ax.set_ylabel('Tiempo promedio (s) — escala logarítmica')
ax.set_title('Comparación: variantes secuenciales')
ax.set_xticks(k_all)
ax.set_xticklabels([f'k={k}\nN={n}' for k, n in zip(k_all, N_all)], fontsize=8)
ax.legend()
ax.grid(True, which='both', linestyle='--', alpha=0.5)
fig.tight_layout()
fig.savefig(os.path.join(CHARTS_DIR, 'comparacion_secuencial.png'))
plt.close(fig)
print("Guardado: comparacion_secuencial.png")

# ─── 2. Tiempos de ejecución — todas las variantes ────────────────────────────
fig, ax = plt.subplots(figsize=(10, 6))
ax.semilogy(k_all,  sec,  'o-',  color='#e74c3c',  lw=2, label='Secuencial')
ax.semilogy(k_all,  flag, 's--', color='#3498db',  lw=2, label='Secuencial -O2')
ax.semilogy(k_all,  mem,  '^-.', color='#2ecc71',  lw=2, label='Memoria opt.')
ax.semilogy(k_all,  t2,   'D-',  color='#9b59b6',  lw=1.5, label='Hilos 2')
ax.semilogy(k_all,  t4,   'v-',  color='#8e44ad',  lw=1.5, label='Hilos 4')
ax.semilogy(k_all,  t8,   'P-',  color='#6c3483',  lw=1.5, label='Hilos 8')
ax.semilogy(k_all,  t16,  'X-',  color='#4a235a',  lw=1.5, label='Hilos 16')
ax.semilogy(k_all,  mp2,  'o--', color='#e67e22',  lw=1.5, label='MP 2')
ax.semilogy(k_all,  mp4,  's--', color='#d35400',  lw=1.5, label='MP 4')
ax.semilogy(k_all,  mp8,  '^--', color='#ba4a00',  lw=1.5, label='MP 8')
ax.semilogy(k_mp16, mp16, 'D--', color='#784212',  lw=1.5, label='MP 16 (k≤8)')
ax.set_xlabel('Índice de malla $k$  ($N = 2^k+1$)')
ax.set_ylabel('Tiempo (s) — escala logarítmica')
ax.set_title('Tiempos de ejecución — todas las variantes')
ax.set_xticks(k_all)
ax.legend(ncol=3, fontsize=9)
ax.grid(True, which='both', linestyle='--', alpha=0.4)
fig.tight_layout()
fig.savefig(os.path.join(CHARTS_DIR, 'tiempo_ejecucion.png'))
plt.close(fig)
print("Guardado: tiempo_ejecucion.png")

# ─── 3. Speedup variantes secuenciales vs secuencial base ─────────────────────
fig, ax = plt.subplots(figsize=(9, 5))
ax.plot(k_all, sec/flag, 's--', color='#3498db', lw=2, label='Secuencial -O2')
ax.plot(k_all, sec/mem,  '^-.', color='#2ecc71', lw=2, label='Memoria opt.')
ax.axhline(1, color='grey', linestyle=':', lw=1)
ax.set_xlabel('Índice de malla $k$')
ax.set_ylabel('Speedup respecto al secuencial base')
ax.set_title('Speedup de variantes secuenciales optimizadas')
ax.set_xticks(k_all)
ax.legend()
ax.grid(True, linestyle='--', alpha=0.5)
fig.tight_layout()
fig.savefig(os.path.join(CHARTS_DIR, 'speedup_secuencial.png'))
plt.close(fig)
print("Guardado: speedup_secuencial.png")

# ─── 4. Ratio tiempo hilos / tiempo secuencial (>1 = más lento) ───────────────
fig, ax = plt.subplots(figsize=(9, 5))
ax.plot(k_all, t2/sec,  'D-',  color='#9b59b6',  lw=2,   label='2 hilos')
ax.plot(k_all, t4/sec,  'v-',  color='#8e44ad',  lw=2,   label='4 hilos')
ax.plot(k_all, t8/sec,  'P-',  color='#6c3483',  lw=2,   label='8 hilos')
ax.plot(k_all, t16/sec, 'X-',  color='#4a235a',  lw=2,   label='16 hilos')
ax.axhline(1, color='red', linestyle='--', lw=1.5, label='Referencia secuencial')
ax.set_xlabel('Índice de malla $k$')
ax.set_ylabel('$T_{hilos} \\ / \\ T_{sec}$  (valores > 1 indican mayor tiempo)')
ax.set_title('Degradación de rendimiento con hilos POSIX')
ax.set_xticks(k_all)
ax.set_yscale('log')
ax.legend()
ax.grid(True, which='both', linestyle='--', alpha=0.5)
fig.tight_layout()
fig.savefig(os.path.join(CHARTS_DIR, 'ratio_hilos.png'))
plt.close(fig)
print("Guardado: ratio_hilos.png")

# ─── 5. Ratio tiempo multiprocesamiento / tiempo secuencial ───────────────────
fig, ax = plt.subplots(figsize=(9, 5))
ax.plot(k_all,  mp2/sec,           'o--', color='#e67e22', lw=2, label='2 procesos')
ax.plot(k_all,  mp4/sec,           's--', color='#d35400', lw=2, label='4 procesos')
ax.plot(k_all,  mp8/sec,           '^--', color='#ba4a00', lw=2, label='8 procesos')
ax.plot(k_mp16, mp16/sec[:len(mp16)], 'D--', color='#784212', lw=2, label='16 procesos (k≤8)')
ax.axhline(1, color='red', linestyle='--', lw=1.5, label='Referencia secuencial')
ax.set_xlabel('Índice de malla $k$')
ax.set_ylabel('$T_{MP} \\ / \\ T_{sec}$  (valores > 1 indican mayor tiempo)')
ax.set_title('Degradación de rendimiento con multiprocesamiento')
ax.set_xticks(k_all)
ax.set_yscale('log')
ax.legend()
ax.grid(True, which='both', linestyle='--', alpha=0.5)
fig.tight_layout()
fig.savefig(os.path.join(CHARTS_DIR, 'ratio_procesos.png'))
plt.close(fig)
print("Guardado: ratio_procesos.png")

# ─── 6. Hilos vs Procesos (k=8..11, mejores configuraciones) ──────────────────
k_sub = np.arange(8, 12)
fig, axes = plt.subplots(1, 2, figsize=(12, 5), sharey=False)

ax = axes[0]
for arr, label, color in [
    (sec,  'Secuencial',  '#e74c3c'),
    (t2,   '2 hilos',     '#9b59b6'),
    (t4,   '4 hilos',     '#8e44ad'),
    (t8,   '8 hilos',     '#6c3483'),
    (t16,  '16 hilos',    '#4a235a'),
]:
    ax.semilogy(k_sub, arr[k_sub-1], 'o-', label=label, color=color)
ax.set_title('Hilos POSIX (k=8..11)')
ax.set_xlabel('k')
ax.set_ylabel('Tiempo (s)')
ax.set_xticks(k_sub)
ax.legend(fontsize=9)
ax.grid(True, which='both', linestyle='--', alpha=0.5)

ax = axes[1]
k_sub_mp = np.arange(8, 12)
for arr, label, color in [
    (sec,  'Secuencial', '#e74c3c'),
    (mp2,  '2 procesos', '#e67e22'),
    (mp4,  '4 procesos', '#d35400'),
    (mp8,  '8 procesos', '#ba4a00'),
]:
    ax.semilogy(k_sub_mp, arr[k_sub_mp-1], 'o--', label=label, color=color)
ax.set_title('Multiprocesamiento (k=8..11)')
ax.set_xlabel('k')
ax.set_xticks(k_sub_mp)
ax.legend(fontsize=9)
ax.grid(True, which='both', linestyle='--', alpha=0.5)

fig.suptitle('Hilos vs. Procesos — grandes instancias')
fig.tight_layout()
fig.savefig(os.path.join(CHARTS_DIR, 'hilos_vs_procesos.png'))
plt.close(fig)
print("Guardado: hilos_vs_procesos.png")

print("\nTodas las gráficas generadas en:", CHARTS_DIR)
