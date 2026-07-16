import numpy as np
import pandas as pd
import json
import plotly.graph_objects as go
import plotly.io as pio

pio.templates.default = "plotly_white"

# Datos estimados a partir de la tendencia del artículo
z = np.array([5, 10, 15, 20, 25, 30, 35, 40], dtype=float)   # mm
F = np.array([0.30, 0.14, 0.085, 0.055, 0.038, 0.028, 0.022, 0.018], dtype=float)  # N

# Curva analítica suave
z_model = np.linspace(5, 40, 400)
F_model = 0.30 * (5 / z_model) ** 2.2

df = pd.DataFrame({
    "distancia_mm": z,
    "fuerza_N": F
})
df.to_csv("fuerza_distancia_tracker.csv", index=False)

fig = go.Figure()

fig.add_trace(go.Scatter(
    x=z,
    y=F,
    mode="markers",
    name="Tracker",
    marker=dict(size=10, symbol="circle")
))

fig.add_trace(go.Scatter(
    x=z_model,
    y=F_model,
    mode="lines",
    name="Modelo analítico",
    line=dict(width=3)
))

fig.update_layout(
    title={
        "text": "Fuerza repulsiva vs distancia<br><span style='font-size:18px;font-weight:normal;'>Datos de Tracker y modelo analítico</span>"
    },
    legend=dict(orientation="h", yanchor="bottom", y=1.05, xanchor="center", x=0.5)
)

fig.update_xaxes(title_text="Brecha (mm)")
fig.update_yaxes(title_text="Fuerza (N)")
fig.update_traces(cliponaxis=False)

fig.write_image("fuerza_distancia.png")

with open("fuerza_distancia.png.meta.json", "w", encoding="utf-8") as f:
    json.dump({
        "caption": "Fuerza repulsiva vs distancia",
        "description": "Dispersión de datos experimentales de Tracker y curva analítica decreciente."
    }, f, ensure_ascii=False)