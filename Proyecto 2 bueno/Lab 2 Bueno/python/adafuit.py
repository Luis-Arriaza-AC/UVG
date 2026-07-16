# adafruit_servo.py
#
# Puente Adafruit IO -> Serial para control de 4 servos SG90.
#
# MODOS COMPATIBLES:
#   Arduino debe estar en modo 1 (serial)
#
# FEEDS:
#   servo-1
#   servo-2
#   servo-3
#   servo-4
#
# NUEVO PROTOCOLO SERIAL:
#   Se envia TODO en un solo mensaje:
#
#       190\r
#       245\r
#       3180\r
#
#   donde:
#       primer caracter = servo
#       resto = angulo
#
# Conexion:
#   Arduino Nano via USB
#   COM3
#   9600 baud

import sys
import time
import serial

from Adafruit_IO import MQTTClient

# ─────────────────────────────────────────────────────────────
# Configuracion Adafruit
# ─────────────────────────────────────────────────────────────

ADAFRUIT_IO_USERNAME = "LF_AC"
ADAFRUIT_IO_KEY      = "aio_WPpq80CeD61IH35XGHNpVdEOBcYR"

# ─────────────────────────────────────────────────────────────
# Feeds
# ─────────────────────────────────────────────────────────────

FEEDS = {
    "servo-4": "1",
    "servo-2": "2",
    "servo-3": "3",
    "servo-1": "4",
}

# ─────────────────────────────────────────────────────────────
# Serial
# ─────────────────────────────────────────────────────────────

SERIAL_PORT = "COM6"
SERIAL_BAUD = 9600

# ─────────────────────────────────────────────────────────────
# Abrir puerto serial
# ─────────────────────────────────────────────────────────────

try:

    ser = serial.Serial(
        port=SERIAL_PORT,
        baudrate=SERIAL_BAUD,
        timeout=1
    )

    # esperar reset Arduino
    time.sleep(2)

    print(f"Puerto serial abierto: {SERIAL_PORT}")
    print(f"Baudrate: {SERIAL_BAUD}")

except serial.SerialException as e:

    print(f"Error abriendo puerto serial:")
    print(e)

    sys.exit(1)

# ─────────────────────────────────────────────────────────────
# Envio serial
# ─────────────────────────────────────────────────────────────

def send_servo(designador, valor):

    """
    NUEVO PROTOCOLO:

        190\r
        245\r
        3180\r

    """

    cmd = f"{designador}{valor}\r"

    ser.write(cmd.encode())
    ser.flush()
    time.sleep(0.03)

    print(f"Enviado -> {repr(cmd)}")

# ─────────────────────────────────────────────────────────────
# MQTT callbacks
# ─────────────────────────────────────────────────────────────

def connected(client):

    print("Conectado a Adafruit IO\n")

    for feed_id in FEEDS:

        print(f"Suscribiendo a: {feed_id}")

        client.subscribe(feed_id)

    print("\nEsperando datos...\n")

def disconnected(client):

    print("Desconectado.")

    ser.close()

    sys.exit(1)

def message(client, feed_id, payload):

    if feed_id not in FEEDS:

        print(f"Feed desconocido: {feed_id}")

        return

    designador = FEEDS[feed_id]

    # convertir payload a entero
    try:

        valor = int(float(payload))

    except ValueError:

        print(f"Payload invalido: {payload}")

        return

    # limitar rangos
    if feed_id in ("servo-4", "servo-2"):

        valor = max(0, min(90, valor))

    else:

        valor = max(0, min(180, valor))

    print(f"{feed_id} -> Servo {designador} = {valor}")

    send_servo(designador, valor)

# ─────────────────────────────────────────────────────────────
# Cliente MQTT
# ─────────────────────────────────────────────────────────────

client = MQTTClient(
    ADAFRUIT_IO_USERNAME,
    ADAFRUIT_IO_KEY
)

client.on_connect    = connected
client.on_disconnect = disconnected
client.on_message    = message

client.connect()

client.loop_background()

# ─────────────────────────────────────────────────────────────
# Loop principal
# ─────────────────────────────────────────────────────────────

print("Puente Adafruit IO <-> Arduino activo.")
print("Puerto serial: COM6")
print("Ctrl+C para salir.\n")

try:

    while True:

        time.sleep(1)

except KeyboardInterrupt:

    print("\nCerrando programa...")

    ser.close()

    sys.exit(0)