"""
Simulación de Superficies Equipotenciales
Ecuación de potencial en rectángulo con Solución en serie con doble fuente
"""

import numpy as np
import matplotlib
import matplotlib.pyplot as plt
from matplotlib.widgets import Slider, Button
from matplotlib.gridspec import GridSpec
import warnings

warnings.filterwarnings('ignore')

matplotlib.rcParams['toolbar'] = 'toolbar2'
matplotlib.rcParams['figure.facecolor'] = '#000000'
matplotlib.rcParams['figure.edgecolor'] = '#000000'
matplotlib.rcParams['axes.facecolor'] = '#000000'
matplotlib.rcParams['savefig.facecolor'] = '#000000'
matplotlib.rcParams['savefig.edgecolor'] = '#000000'
matplotlib.rcParams['text.color'] = 'white'
matplotlib.rcParams['axes.labelcolor'] = 'white'
matplotlib.rcParams['xtick.color'] = 'white'
matplotlib.rcParams['ytick.color'] = 'white'

def calcular_V(X, Y, a, b, V1, V2, N_terminos=8):
    V = np.zeros_like(X, dtype=float)
    for k in range(1, N_terminos + 1):
        n = 2 * k - 1
        s = np.sin(n * np.pi * X / b)
        den = n * np.sinh(n * np.pi * a / b)
        term = (V1 * np.sinh(n * np.pi * (a - Y) / b) + V2 * np.sinh(n * np.pi * Y / b))
        V += s * term / den
    return (4.0 / np.pi) * V

def tabla_terminos(x0, y0, a, b, V1, V2, N_terminos=8):
    filas = []
    suma = 0.0
    for k in range(1, N_terminos + 1):
        n = 2 * k - 1
        sv = np.sin(n * np.pi * x0 / b)
        sha = np.sinh(n * np.pi * a / b)
        sh1 = np.sinh(n * np.pi * (a - y0) / b)
        sh2 = np.sinh(n * np.pi * y0 / b)
        t = sv * (V1 * sh1 + V2 * sh2) / (n * sha)
        suma += t
        filas.append((n, sv, sh1, sh2, sha, t, suma))
    return filas, (4.0 / np.pi) * suma

def simulacion_interactiva():
    DELTA = 0.10
    PX, PY = 0.8, 0.5

    fig = plt.figure(figsize=(15, 8.5))
    fig.patch.set_facecolor('#000000')
    fig.suptitle(
        'Simulación de Potencial Eléctrico — Ecuación del rectángulo con V1 y V2',
        fontsize=13, fontweight='bold', color='white', y=0.96
    )

    gs = GridSpec(
        1, 2,
        figure=fig,
        left=0.05, right=0.97,
        top=0.89, bottom=0.30,
        wspace=0.38
    )
    ax_contour = fig.add_subplot(gs[0, 0])
    ax_tabla = fig.add_subplot(gs[0, 1])

    ax_sl = {
        'a': fig.add_axes([0.06, 0.20, 0.38, 0.024]),
        'b': fig.add_axes([0.06, 0.15, 0.38, 0.024]),
        'V1': fig.add_axes([0.55, 0.20, 0.38, 0.024]),
        'V2': fig.add_axes([0.55, 0.15, 0.38, 0.024]),
    }
    sliders = {
        'a': Slider(ax_sl['a'], 'a  (alto)', 0.5, 5.0, valinit=1.0, valstep=0.5, color='#e94560'),
        'b': Slider(ax_sl['b'], 'b  (ancho)', 0.5, 5.0, valinit=1.0, valstep=0.5, color='#e94560'),
        'V1': Slider(ax_sl['V1'], 'V₁ (V)', 0, 200, valinit=100, valstep=10, color='#4ade80'),
        'V2': Slider(ax_sl['V2'], 'V₂ (V)', 0, 200, valinit=50, valstep=10, color='#60a5fa'),
    }
    for sl in sliders.values():
        sl.label.set_color('white')
        sl.valtext.set_color('white')
        sl.ax.set_facecolor('#000000')

    escenarios = [
        {'a': 1, 'b': 1, 'V1': 100, 'V2': 50, 'label': 'a=1, b=1'},
        {'a': 1, 'b': 2, 'V1': 100, 'V2': 50, 'label': 'a=1, b=2'},
        {'a': 1, 'b': 3, 'V1': 100, 'V2': 50, 'label': 'a=1, b=3'},
        {'a': 4, 'b': 1, 'V1': 100, 'V2': 50, 'label': 'a=4, b=1'},
    ]

    btn_axes = [fig.add_axes([0.06 + i * 0.105, 0.07, 0.09, 0.045]) for i in range(4)]
    botones = []
    for ax_b, esc in zip(btn_axes, escenarios):
        btn = Button(ax_b, esc['label'], color='#000000', hovercolor='#333333')
        btn.label.set_color('white')
        btn.label.set_fontsize(8.5)
        botones.append(btn)

    ax_btn_upd = fig.add_axes([0.87, 0.065, 0.10, 0.055])
    btn_upd = Button(ax_btn_upd, 'Actualizar\nGráfica', color='#e94560', hovercolor='#c73652')
    btn_upd.label.set_color('white')
    btn_upd.label.set_fontweight('bold')
    btn_upd.label.set_fontsize(9)

    _cbar = [None]

    def dibujar(_evt=None):
        a = sliders['a'].val
        b = sliders['b'].val
        V1 = sliders['V1'].val
        V2 = sliders['V2'].val
        N = 8

        x = np.arange(0, b + DELTA, DELTA)
        y = np.arange(0, a + DELTA, DELTA)
        X, Y = np.meshgrid(x, y)
        V = calcular_V(X, Y, a, b, V1, V2, N)

        if _cbar[0] is not None:
            try:
                _cbar[0].remove()
            except Exception:
                pass
            _cbar[0] = None

        ax_contour.clear()
        ax_contour.set_facecolor('#000000')

        vmin = min(0, V1, V2)
        vmax = max(V1, V2)
        niveles = np.linspace(vmin, vmax, 21)

        cf = ax_contour.contourf(X, Y, V, levels=niveles, cmap='plasma', alpha=0.92)
        cs = ax_contour.contour(X, Y, V, levels=niveles, colors='white', linewidths=0.55, alpha=0.45)
        ax_contour.clabel(cs, inline=True, fontsize=6.5, fmt='%.0f V', colors='white')

        if 0 <= PX <= b and 0 <= PY <= a:
            _, Vp = tabla_terminos(PX, PY, a, b, V1, V2, N)
            ax_contour.plot(PX, PY, 'o', color='#00ffcc', ms=7, zorder=5)
            ax_contour.annotate(
                f'  ({PX}, {PY})\n  V≈{Vp:.2f} V',
                xy=(PX, PY), color='#00ffcc', fontsize=8, fontweight='bold'
            )

        ax_contour.set_xlabel('x', color='white', fontsize=10, fontweight='bold')
        ax_contour.set_ylabel('y', color='white', fontsize=10, fontweight='bold')
        ax_contour.set_title(
            f'Equipotenciales | a={a}, b={b}, V₁={V1} V, V₂={V2} V',
            color='white', fontsize=11, fontweight='bold', pad=8
        )
        ax_contour.tick_params(colors='white')
        for sp in ax_contour.spines.values():
            sp.set_edgecolor('#666666')
        ax_contour.set_xlim(0, b)
        ax_contour.set_ylim(0, a)
        ax_contour.set_aspect('equal', adjustable='box')

        cb = fig.colorbar(cf, ax=ax_contour, label='V (Voltios)', pad=0.03, fraction=0.046)
        cb.ax.yaxis.label.set_color('white')
        cb.ax.tick_params(colors='white')
        cb.ax.set_facecolor('#000000')
        _cbar[0] = cb

        filas, Vp = tabla_terminos(PX, PY, a, b, V1, V2, N)

        ax_tabla.clear()
        ax_tabla.set_facecolor('#000000')
        ax_tabla.axis('off')
        ax_tabla.set_title(
            f'Términos de la sumatoria — V({PX}, {PY})',
            color='white', fontsize=10, fontweight='bold', pad=8
        )

        headers = ['n', 'sin(nπx/b)', 'sinh(nπ(a-y)/b)', 'sinh(nπy/b)', 'sinh(nπa/b)', 'Término', 'Σ acum.']
        col_data = [[
            str(f[0]),
            f'{f[1]:+.6f}',
            f'{f[2]:.4e}',
            f'{f[3]:.4e}',
            f'{f[4]:.4e}',
            f'{f[5]:+.5e}',
            f'{f[6]:+.8f}',
        ] for f in filas]

        tbl = ax_tabla.table(
            cellText=col_data,
            colLabels=headers,
            loc='center',
            cellLoc='center',
            bbox=[0.0, 0.10, 1.0, 0.88]
        )
        tbl.auto_set_font_size(False)
        tbl.set_fontsize(7.6)
        tbl.scale(1, 1.5)

        for (row, col), cell in tbl.get_celld().items():
            if row == 0:
                cell.set_facecolor('#333333')
                cell.set_text_props(color='white', fontweight='bold')
            else:
                cell.set_facecolor('#111111' if row % 2 else '#222222')
                cell.set_text_props(color='white')
            cell.set_edgecolor('#666666')

        ax_tabla.text(
            0.5, 0.03,
            f'V({PX}, {PY}) ≈ {Vp:.6f} V',
            transform=ax_tabla.transAxes, ha='center', va='bottom',
            color='#00ffcc', fontsize=10, fontweight='bold'
        )

        fig.canvas.draw_idle()

    btn_upd.on_clicked(dibujar)

    def make_cb(esc):
        def cb(_e):
            sliders['a'].set_val(esc['a'])
            sliders['b'].set_val(esc['b'])
            sliders['V1'].set_val(esc['V1'])
            sliders['V2'].set_val(esc['V2'])
            dibujar()
        return cb

    for btn, esc in zip(botones, escenarios):
        btn.on_clicked(make_cb(esc))

    dibujar()
    plt.show()

def graficar_4_escenarios():
    escenarios = [
        {'a': 1, 'b': 1, 'V1': 100, 'V2': 50},
        {'a': 1, 'b': 2, 'V1': 100, 'V2': 50},
        {'a': 1, 'b': 3, 'V1': 100, 'V2': 50},
        {'a': 4, 'b': 1, 'V1': 100, 'V2': 50},
    ]
    DELTA = 0.10
    N = 8
    PX, PY = 0.8, 0.5

    fig = plt.figure(figsize=(17, 13))
    fig.patch.set_facecolor('#000000')
    fig.suptitle(
        'Superficies Equipotenciales — Rectángulo con V₁ y V₂ en lados opuestos\n'
        f'8 términos impares | Δ = {DELTA}',
        fontsize=14, fontweight='bold', color='white', y=0.98
    )

    gs = GridSpec(
        2, 4,
        figure=fig,
        top=0.92, bottom=0.05,
        left=0.04, right=0.97,
        hspace=0.50, wspace=0.35,
        height_ratios=[3, 1.1]
    )

    for idx, esc in enumerate(escenarios):
        a, b, V1, V2 = esc['a'], esc['b'], esc['V1'], esc['V2']
        ax = fig.add_subplot(gs[0, idx])

        x = np.arange(0, b + DELTA, DELTA)
        y = np.arange(0, a + DELTA, DELTA)
        X, Y = np.meshgrid(x, y)
        V = calcular_V(X, Y, a, b, V1, V2, N)

        niveles = np.linspace(min(0, V1, V2), max(V1, V2), 21)
        ax.set_facecolor('#000000')
        cf = ax.contourf(X, Y, V, levels=niveles, cmap='plasma', alpha=0.90)
        cs = ax.contour(X, Y, V, levels=niveles, colors='white', linewidths=0.5, alpha=0.4)
        ax.clabel(cs, inline=True, fontsize=6, fmt='%.0f V', colors='white')

        if idx == 0 and 0 <= PX <= b and 0 <= PY <= a:
            _, Vp = tabla_terminos(PX, PY, a, b, V1, V2, N)
            ax.plot(PX, PY, 'o', color='#00ffcc', ms=8, zorder=5)
            ax.annotate(
                f' ({PX},{PY})\n V≈{Vp:.2f}V',
                xy=(PX, PY), color='#00ffcc', fontsize=8, fontweight='bold'
            )

        cb = fig.colorbar(cf, ax=ax, label='V (V)', pad=0.02, fraction=0.046)
        cb.ax.yaxis.label.set_color('white')
        cb.ax.tick_params(colors='white')
        cb.ax.set_facecolor('#000000')

        ax.set_xlabel('x', color='white', fontsize=9, fontweight='bold')
        ax.set_ylabel('y', color='white', fontsize=9, fontweight='bold')
        ax.set_title(f'a={a}, b={b}, V₁={V1} V, V₂={V2} V',
                     color='white', fontsize=11, fontweight='bold')
        ax.tick_params(colors='white')
        for sp in ax.spines.values():
            sp.set_edgecolor('#666666')
        ax.set_aspect('equal', adjustable='box')

    ax_tbl = fig.add_subplot(gs[1, :])
    ax_tbl.set_facecolor('#000000')
    ax_tbl.axis('off')
    ax_tbl.set_title(
        f'Tabla de los 8 términos impares — V({PX}, {PY}), caso a=1, b=1, V₁=100 V, V₂=50 V',
        color='white', fontsize=10, fontweight='bold', pad=6
    )

    filas, Vp = tabla_terminos(PX, PY, 1.0, 1.0, 100.0, 50.0, 8)
    headers = ['n', 'sin(nπ·0.8)', 'sinh(nπ(a-y))', 'sinh(nπy)', 'sinh(nπa)',
               'Término', 'Σ acumulada']
    col_data = [[
        str(f[0]),
        f'{f[1]:+.8f}',
        f'{f[2]:.6e}',
        f'{f[3]:.6e}',
        f'{f[4]:.6e}',
        f'{f[5]:+.8e}',
        f'{f[6]:+.10f}',
    ] for f in filas]

    tbl = ax_tbl.table(
        cellText=col_data,
        colLabels=headers,
        loc='center',
        cellLoc='center',
        bbox=[0.0, 0.15, 1.0, 0.80]
    )
    tbl.auto_set_font_size(False)
    tbl.set_fontsize(7.6)
    tbl.scale(1, 1.6)

    for (row, col), cell in tbl.get_celld().items():
        if row == 0:
            cell.set_facecolor('#333333')
            cell.set_text_props(color='white', fontweight='bold')
        else:
            cell.set_facecolor('#111111' if row % 2 else '#222222')
            cell.set_text_props(color='white')
        cell.set_edgecolor('#666666')

    ax_tbl.text(
        0.5, 0.03,
        f'Resultado: V(0.8, 0.5) ≈ {Vp:.6f} V',
        transform=ax_tbl.transAxes, ha='center', va='bottom',
        color='#00ffcc', fontsize=10, fontweight='bold'
    )

    plt.savefig('equipotenciales_4_escenarios.png', dpi=150,
                bbox_inches='tight', facecolor=fig.get_facecolor())
    print('✅ Figura guardada → equipotenciales_4_escenarios.png')
    plt.show()

if __name__ == '__main__':
    print('=' * 62)
    print('  Potencial Eléctrico en Región Rectangular')
    print('=' * 62)
    print()
    print('  1. Interfaz interactiva')
    print('  2. 4 escenarios del problema + tabla primer caso')
    print()

    opcion = input('  Seleccione [1/2, default=1]: ').strip()

    if opcion == '2':
        graficar_4_escenarios()
    else:
        print('\n  Abriendo interfaz interactiva …')
        print('  • Use los sliders y presione «Actualizar Gráfica»')
        print('  • Los botones cargan los 4 escenarios del enunciado\n')
        simulacion_interactiva()