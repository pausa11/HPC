import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
import numpy as np
import os

# Output directory (relative to project root)
OUT_DIR = 'docs/charts'
os.makedirs(OUT_DIR, exist_ok=True)

DATA_PATH = 'stats/Mac-mini-de-Daniel.local/20260318_215928.done/'
SIZES = [500, 1000, 1300, 1600, 2000, 2300, 2600, 3000, 3300, 3600, 4000]

# ── Load all data ──────────────────────────────────────────────────────────────
# CSVs have a trailing comma → pandas sees ncols+1 columns and shifts names.
# usecols=range(len(SIZES)) discards that empty last column.
def load(filename):
    return pd.read_csv(filename, header=None, names=SIZES, usecols=range(len(SIZES)))

avg_seq   = load(DATA_PATH + 'secuential.csv').mean()
avg_flags = load(DATA_PATH + 'secuential_flag.csv').mean()
avg_mem   = load(DATA_PATH + 'memory.csv').mean()

avgs_threads = {p: load(DATA_PATH + f'threads{p}.csv').mean()           for p in [2, 4, 8, 16]}
avgs_mp      = {p: load(DATA_PATH + f'mutiprocessing{p}.csv').mean()    for p in [2, 4, 8, 16]}

WORKERS = [2, 4, 8, 16]

# ── Plot 1: Execution time — all variants ──────────────────────────────────────
fig, ax = plt.subplots(figsize=(11, 6))

ax.plot(SIZES, avg_seq.values,   label='Secuencial',       marker='o', linewidth=2)
ax.plot(SIZES, avg_flags.values, label='Secuencial (-O2)', marker='s', linewidth=2, linestyle='--')
ax.plot(SIZES, avg_mem.values,   label='Memoria (transp.)', marker='^', linewidth=2, linestyle='--')

colors_t = plt.cm.Blues(np.linspace(0.45, 0.9, 4))
for i, p in enumerate(WORKERS):
    ax.plot(SIZES, avgs_threads[p].values, label=f'{p} Hilos',
            marker='o', color=colors_t[i], linewidth=1.5)

colors_mp = plt.cm.Oranges(np.linspace(0.45, 0.9, 4))
for i, p in enumerate(WORKERS):
    ax.plot(SIZES, avgs_mp[p].values, label=f'{p} Procesos',
            marker='s', color=colors_mp[i], linewidth=1.5, linestyle=':')

ax.set_xlabel('Dimensión de la matriz cuadrada (N×N)', fontsize=12)
ax.set_ylabel('Tiempo de ejecución (s)', fontsize=12)
ax.set_title('Tiempo de ejecución vs. dimensión de la matriz', fontsize=13)
ax.legend(ncol=3, fontsize=8)
ax.grid(True, alpha=0.4)
fig.tight_layout()
fig.savefig(f'{OUT_DIR}/tiempo_ejecucion.png', dpi=150)
plt.close()

# ── Plot 2: Speedup — hilos y procesos ────────────────────────────────────────
fig, ax = plt.subplots(figsize=(11, 6))

for i, p in enumerate(WORKERS):
    sp = avg_seq / avgs_threads[p]
    ax.plot(SIZES, sp.values, label=f'{p} Hilos',
            marker='o', color=colors_t[i], linewidth=1.8)

for i, p in enumerate(WORKERS):
    sp = avg_seq / avgs_mp[p]
    ax.plot(SIZES, sp.values, label=f'{p} Procesos',
            marker='s', color=colors_mp[i], linewidth=1.8, linestyle=':')

ax.set_xlabel('Dimensión de la matriz cuadrada (N×N)', fontsize=12)
ax.set_ylabel('Speedup', fontsize=12)
ax.set_title('Speedup en función de la dimensión de la matriz', fontsize=13)
ax.legend(ncol=2, fontsize=9)
ax.grid(True, alpha=0.4)
fig.tight_layout()
fig.savefig(f'{OUT_DIR}/speedup.png', dpi=150)
plt.close()

# ── Plot 3: Variantes secuenciales solamente ──────────────────────────────────
fig, ax = plt.subplots(figsize=(10, 6))

ax.plot(SIZES, avg_seq.values,   label='Secuencial (base)',  marker='o', linewidth=2, color='steelblue')
ax.plot(SIZES, avg_flags.values, label='Secuencial (-O2)',   marker='s', linewidth=2, color='darkorange', linestyle='--')
ax.plot(SIZES, avg_mem.values,   label='Memoria (transp.)',  marker='^', linewidth=2, color='seagreen',   linestyle='-.')

ax.set_xlabel('Dimensión de la matriz cuadrada (N×N)', fontsize=12)
ax.set_ylabel('Tiempo de ejecución (s)', fontsize=12)
ax.set_title('Comparación de variantes secuenciales', fontsize=13)
ax.legend(fontsize=10)
ax.grid(True, alpha=0.4)
fig.tight_layout()
fig.savefig(f'{OUT_DIR}/secuencial_comparacion.png', dpi=150)
plt.close()

# ── Plot 4: Eficiencia paralela (E = Speedup / p) ────────────────────────────
fig, ax = plt.subplots(figsize=(11, 6))

for i, p in enumerate(WORKERS):
    eff = (avg_seq / avgs_threads[p]) / p
    ax.plot(SIZES, eff.values, label=f'{p} Hilos',
            marker='o', color=colors_t[i], linewidth=1.8)

for i, p in enumerate(WORKERS):
    eff = (avg_seq / avgs_mp[p]) / p
    ax.plot(SIZES, eff.values, label=f'{p} Procesos',
            marker='s', color=colors_mp[i], linewidth=1.8, linestyle=':')

ax.axhline(1.0, color='black', linestyle='--', linewidth=1, label='Ideal (E=1)')
ax.set_ylim(0, 1.15)
ax.yaxis.set_major_formatter(ticker.PercentFormatter(xmax=1))
ax.set_xlabel('Dimensión de la matriz cuadrada (N×N)', fontsize=12)
ax.set_ylabel('Eficiencia paralela (Speedup / p)', fontsize=12)
ax.set_title('Eficiencia paralela en función de la dimensión de la matriz', fontsize=13)
ax.legend(ncol=2, fontsize=9)
ax.grid(True, alpha=0.4)
fig.tight_layout()
fig.savefig(f'{OUT_DIR}/eficiencia_paralela.png', dpi=150)
plt.close()

# ── Plot 5: Hilos vs Procesos (mismo nº de workers) ───────────────────────────
fig, axes = plt.subplots(2, 2, figsize=(12, 8), sharey=False)
axes = axes.flatten()

for idx, p in enumerate(WORKERS):
    ax = axes[idx]
    sp_t  = avg_seq / avgs_threads[p]
    sp_mp = avg_seq / avgs_mp[p]
    ax.plot(SIZES, sp_t.values,  label=f'Hilos',    marker='o', linewidth=2, color='steelblue')
    ax.plot(SIZES, sp_mp.values, label=f'Procesos', marker='s', linewidth=2, color='darkorange', linestyle='--')
    ax.set_title(f'{p} workers', fontsize=11)
    ax.set_xlabel('Dimensión (N×N)', fontsize=9)
    ax.set_ylabel('Speedup', fontsize=9)
    ax.legend(fontsize=9)
    ax.grid(True, alpha=0.4)

fig.suptitle('Speedup: Hilos vs. Procesos por número de workers', fontsize=13)
fig.tight_layout()
fig.savefig(f'{OUT_DIR}/hilos_vs_procesos.png', dpi=150)
plt.close()

# ── Plot 6: Heatmap de speedup ────────────────────────────────────────────────
all_labels = [f'{p}H' for p in WORKERS] + [f'{p}P' for p in WORKERS]
matrix_data = np.zeros((len(all_labels), len(SIZES)))

for i, p in enumerate(WORKERS):
    matrix_data[i] = (avg_seq / avgs_threads[p]).values
for i, p in enumerate(WORKERS):
    matrix_data[len(WORKERS) + i] = (avg_seq / avgs_mp[p]).values

fig, ax = plt.subplots(figsize=(12, 5))
im = ax.imshow(matrix_data, aspect='auto', cmap='YlOrRd', vmin=1, vmax=5.5)

ax.set_xticks(range(len(SIZES)))
ax.set_xticklabels(SIZES, rotation=45, ha='right', fontsize=9)
ax.set_yticks(range(len(all_labels)))
ax.set_yticklabels(all_labels, fontsize=9)

for i in range(len(all_labels)):
    for j in range(len(SIZES)):
        val = matrix_data[i, j]
        text_color = 'white' if val > 4.0 else 'black'
        ax.text(j, i, f'{val:.2f}', ha='center', va='center',
                fontsize=7, color=text_color)

# Separator line between hilos and procesos
ax.axhline(len(WORKERS) - 0.5, color='white', linewidth=2)

cbar = fig.colorbar(im, ax=ax, fraction=0.02, pad=0.04)
cbar.set_label('Speedup', fontsize=10)

ax.set_xlabel('Dimensión de la matriz (N×N)', fontsize=11)
ax.set_ylabel('Configuración (H=Hilos, P=Procesos)', fontsize=11)
ax.set_title('Heatmap de Speedup — Hilos y Procesos', fontsize=13)
fig.tight_layout()
fig.savefig(f'{OUT_DIR}/heatmap_speedup.png', dpi=150)
plt.close()

# ── Plot 7: Speedup — solo hilos (4 curvas) ──────────────────────────────────
fig, ax = plt.subplots(figsize=(11, 6))

colors_t = plt.cm.Blues(np.linspace(0.45, 0.9, 4))
for i, p in enumerate(WORKERS):
    sp = avg_seq / avgs_threads[p]
    ax.plot(SIZES, sp.values, label=f'{p} Hilos',
            marker='o', color=colors_t[i], linewidth=2)

ax.set_xlabel('Dimensión de la matriz cuadrada (N×N)', fontsize=12)
ax.set_ylabel('Speedup', fontsize=12)
ax.set_title('Speedup — Hilos POSIX (2, 4, 8, 16 hilos)', fontsize=13)
ax.legend(fontsize=10)
ax.grid(True, alpha=0.4)
fig.tight_layout()
fig.savefig(f'{OUT_DIR}/speedup_hilos.png', dpi=150)
plt.close()

# ── Plot 8: Speedup — solo procesos (4 curvas) ────────────────────────────────
fig, ax = plt.subplots(figsize=(11, 6))

colors_mp = plt.cm.Oranges(np.linspace(0.45, 0.9, 4))
for i, p in enumerate(WORKERS):
    sp = avg_seq / avgs_mp[p]
    ax.plot(SIZES, sp.values, label=f'{p} Procesos',
            marker='s', color=colors_mp[i], linewidth=2, linestyle='--')

ax.set_xlabel('Dimensión de la matriz cuadrada (N×N)', fontsize=12)
ax.set_ylabel('Speedup', fontsize=12)
ax.set_title('Speedup — Multiprocesamiento (2, 4, 8, 16 procesos)', fontsize=13)
ax.legend(fontsize=10)
ax.grid(True, alpha=0.4)
fig.tight_layout()
fig.savefig(f'{OUT_DIR}/speedup_procesos.png', dpi=150)
plt.close()

# ── Plot 9: 6 curvas — mejores variantes ──────────────────────────────────────
# Determine 2 best threads and 2 best processes by average speedup
sp_threads = {p: (avg_seq / avgs_threads[p]).mean() for p in WORKERS}
sp_mp      = {p: (avg_seq / avgs_mp[p]).mean()      for p in WORKERS}
best_threads = sorted(sp_threads, key=sp_threads.get, reverse=True)[:2]
best_mp      = sorted(sp_mp,      key=sp_mp.get,      reverse=True)[:2]

fig, ax = plt.subplots(figsize=(11, 6))

ax.plot(SIZES, (avg_seq / avg_flags).values, label='Secuencial (-O2)',
        marker='D', linewidth=2, color='darkorange', linestyle='--')
ax.plot(SIZES, (avg_seq / avg_mem).values,   label='Memoria (transp.)',
        marker='^', linewidth=2, color='seagreen',   linestyle='-.')

colors_t2  = ['#1565C0', '#42A5F5']
colors_mp2 = ['#E65100', '#FF8A65']

for i, p in enumerate(sorted(best_threads)):
    ax.plot(SIZES, (avg_seq / avgs_threads[p]).values,
            label=f'Hilos {p}', marker='o', color=colors_t2[i], linewidth=2)

for i, p in enumerate(sorted(best_mp)):
    ax.plot(SIZES, (avg_seq / avgs_mp[p]).values,
            label=f'Procesos {p}', marker='s', color=colors_mp2[i],
            linewidth=2, linestyle=':')

ax.set_xlabel('Dimensión de la matriz cuadrada (N×N)', fontsize=12)
ax.set_ylabel('Speedup', fontsize=12)
ax.set_title('Speedup — 6 mejores variantes', fontsize=13)
ax.legend(fontsize=10)
ax.grid(True, alpha=0.4)
fig.tight_layout()
fig.savefig(f'{OUT_DIR}/speedup_6curvas.png', dpi=150)
plt.close()

print(f"9 gráficas generadas en '{OUT_DIR}/'")
