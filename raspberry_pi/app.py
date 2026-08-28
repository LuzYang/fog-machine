import os

import requests
from flask import Flask, jsonify, render_template, request


app = Flask(__name__)
ESP32_BASE_URL = os.getenv("ESP32_BASE_URL", "http://192.168.1.50")

ALLOWED_EFFECTS = {"blackout", "solid", "flash", "pulse", "chase"}


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
    effect = data.get("effect", "solid")
    if effect not in ALLOWED_EFFECTS:
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
        response = requests.post(
            f"{ESP32_BASE_URL}/api/state",
            params=parameters,
            timeout=1.5,
        )
        response.raise_for_status()
    except requests.RequestException as error:
        return jsonify(error="ESP32 is unavailable", detail=str(error)), 502

    return jsonify(response.json())


if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000, debug=False)
