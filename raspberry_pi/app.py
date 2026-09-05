import os

import json
import threading
import time

import serial
from flask import Flask, jsonify, render_template, request


app = Flask(__name__)
ESP32_SERIAL_PORT = os.getenv("ESP32_SERIAL_PORT", "/dev/ttyACM0")
serial_lock = threading.Lock()
serial_connection = None

ALLOWED_EFFECTS = {"blackout", "solid", "flash", "pulse", "chase"}


def send_state(parameters):
    """Keep USB open and match each command with its acknowledgement."""
    global serial_connection
    command = (json.dumps(parameters, separators=(",", ":")) + "\n").encode("ascii")
    with serial_lock:
        try:
            if serial_connection is None:
                serial_connection = serial.Serial(
                    ESP32_SERIAL_PORT, 115200, timeout=0.2, write_timeout=1.5,
                    exclusive=True,
                )
                # USB UART boards may reset when opened.
                time.sleep(2)
            serial_connection.reset_input_buffer()
            serial_connection.write(command)
            deadline = time.monotonic() + 1.5
            reply = b""
            while time.monotonic() < deadline:
                reply += serial_connection.read_until(b"\n")
                if not reply.endswith(b"\n"):
                    continue
                line = reply.decode("ascii", errors="replace").strip()
                reply = b""
                if line == f"OK {parameters['effect']}":
                    return {"ok": True, "effect": parameters["effect"]}
                if line.startswith("ERR "):
                    raise serial.SerialException(line)
            raise serial.SerialException("ESP32 USB response timed out")
        except (serial.SerialException, OSError):
            if serial_connection is not None:
                try:
                    serial_connection.close()
                except OSError:
                    pass
                serial_connection = None
            raise


def bounded_integer(data, name, minimum, maximum, default):
    """Validate and clamp an integer. / 验证整数，并把它限制在允许范围内。"""
    try:
        return max(minimum, min(int(data.get(name, default)), maximum))
    except (TypeError, ValueError):
        raise ValueError(f"{name} must be an integer") from None


@app.get("/")
def index():
    return render_template("index.html")


@app.post("/api/led")
def set_led():
    data = request.get_json(silent=True) or {}
    if not isinstance(data, dict):
        return jsonify(error="expected a JSON object"), 400
    effect = data.get("effect", "solid")
    if not isinstance(effect, str) or effect not in ALLOWED_EFFECTS:
        return jsonify(error="unknown effect"), 400

    try:
        parameters = {
            "effect": effect,
            "r": bounded_integer(data, "r", 0, 255, 255),
            "g": bounded_integer(data, "g", 0, 255, 0),
            "b": bounded_integer(data, "b", 0, 255, 0),
            "brightness": bounded_integer(data, "brightness", 0, 255, 80),
            "speed_ms": bounded_integer(data, "speed_ms", 100, 5000, 500),
            "width": bounded_integer(data, "width", 1, 255, 4),
            "direction": "reverse" if data.get("direction") == "reverse" else "forward",
        }
    except ValueError as error:
        return jsonify(error=str(error)), 400

    try:
        response = send_state(parameters)
    except (serial.SerialException, OSError) as error:
        return jsonify(error="ESP32 USB is unavailable", detail=str(error)), 502

    return jsonify(response)


if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000, debug=False)
