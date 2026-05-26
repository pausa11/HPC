import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import os

OUT_DIR = 'latex/charts'
os.makedirs(OUT_DIR, exist_ok=True)

DATA_PATH = 'stats/ip-172-31-10-216/20260525_022135.done/'
SIZES = [504, 1008, 1296, 1608, 2004, 2304, 2604, 3000, 3300, 3600, 4008]
WORKERS = [2, 4, 6]


def load(filename):
    return pd.read_csv(filename, header=None, names=SIZES,
                       usecols=range(len(SIZES)))


avg_seq = load(DATA_PATH + 'secuential.csv').mean()
avgs_mpi = {p: load(DATA_PATH + f'point_to_point_{p}.csv').mean()
            for p in WORKERS}

colors_mpi = plt.cm.Oranges(np.linspace(0.45, 0.9, len(WORKERS)))

# ── Plot 1: Tiempo de ejecución ──────────────────────────────────────────────
fig, ax = plt.subplots(figsize=(11, 6))

ax.plot(SIZES, avg_seq.values, label='Secuencial (monolítico)',
        marker='o', linewidth=2, color='steelblue')

for i, p in enumerate(WORKERS):
    ax.plot(SIZES, avgs_mpi[p].values, label=f'MPI {p} procesos',
            marker='s', color=colors_mpi[i], linewidth=2, linestyle='--')

ax.set_xlabel('Dimensión de la matriz cuadrada (N×N)', fontsize=12)
ax.set_ylabel('Tiempo de ejecución (s)', fontsize=12)
ax.set_title('Tiempo de ejecución vs. dimensión de la matriz', fontsize=13)
ax.legend(fontsize=10)
ax.grid(True, alpha=0.4)
fig.tight_layout()
fig.savefig(f'{OUT_DIR}/tiempo_ejecucion.png', dpi=150)
plt.close()

# ── Plot 2: Speedup vs tamaño de matriz (gráfica principal) ──────────────────
fig, ax = plt.subplots(figsize=(11, 6))

for i, p in enumerate(WORKERS):
    sp = avg_seq / avgs_mpi[p]
    ax.plot(SIZES, sp.values, label=f'MPI {p} procesos',
            marker='s', color=colors_mpi[i], linewidth=2)

ax.axhline(1.0, color='gray', linestyle=':', linewidth=1, label='Sin speedup (S=1)')
ax.set_xlabel('Dimensión de la matriz cuadrada (N×N)', fontsize=12)
ax.set_ylabel('Speedup ($T_{seq} / T_p$)', fontsize=12)
ax.set_title('Speedup respecto al baseline secuencial', fontsize=13)
ax.legend(fontsize=10)
ax.grid(True, alpha=0.4)
fig.tight_layout()
fig.savefig(f'{OUT_DIR}/speedup.png', dpi=150)
plt.close()

# ── Plot 3: Heatmap de speedup ───────────────────────────────────────────────
labels = [f'{p}P' for p in WORKERS]
matrix_data = np.zeros((len(WORKERS), len(SIZES)))

for i, p in enumerate(WORKERS):
    matrix_data[i] = (avg_seq / avgs_mpi[p]).values

fig, ax = plt.subplots(figsize=(12, 3.5))
vmax = float(np.ceil(matrix_data.max()))
im = ax.imshow(matrix_data, aspect='auto', cmap='YlOrRd',
               vmin=1, vmax=vmax)

ax.set_xticks(range(len(SIZES)))
ax.set_xticklabels(SIZES, rotation=45, ha='right', fontsize=9)
ax.set_yticks(range(len(labels)))
ax.set_yticklabels(labels, fontsize=10)

for i in range(len(labels)):
    for j in range(len(SIZES)):
        val = matrix_data[i, j]
        color = 'white' if val > 0.7 * vmax else 'black'
        ax.text(j, i, f'{val:.2f}', ha='center', va='center',
                fontsize=8, color=color)

cbar = fig.colorbar(im, ax=ax, fraction=0.025, pad=0.02)
cbar.set_label('Speedup', fontsize=10)

ax.set_xlabel('Dimensión de la matriz (N×N)', fontsize=11)
ax.set_ylabel('Procesos MPI', fontsize=11)
ax.set_title('Heatmap de Speedup — MPI Point-to-Point', fontsize=13)
fig.tight_layout()
fig.savefig(f'{OUT_DIR}/heatmap_speedup.png', dpi=150)
plt.close()

# ── Plot 4: Speedup vs número de procesos ────────────────────────────────────
sample_sizes = [1008, 2004, 4008]
fig, ax = plt.subplots(figsize=(10, 6))

palette = ['#1565C0', '#E65100', '#2E7D32']
for c, n in zip(palette, sample_sizes):
    speedups = [avg_seq[n] / avgs_mpi[p][n] for p in WORKERS]
    ax.plot(WORKERS, speedups, label=f'N = {n}',
            marker='o', linewidth=2, color=c)

ax.plot(WORKERS, WORKERS, label='Speedup ideal (S=p)',
        linestyle='--', color='gray', linewidth=1.5)

ax.set_xticks(WORKERS)
ax.set_xlabel('Número de procesos MPI ($p$)', fontsize=12)
ax.set_ylabel('Speedup ($T_{seq} / T_p$)', fontsize=12)
ax.set_title('Escalado del speedup al variar el número de procesos', fontsize=13)
ax.legend(fontsize=10)
ax.grid(True, alpha=0.4)
fig.tight_layout()
fig.savefig(f'{OUT_DIR}/speedup_procesos.png', dpi=150)
plt.close()

print(f"4 gráficas generadas en '{OUT_DIR}/'")
