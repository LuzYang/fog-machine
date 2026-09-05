# entrance-light demo

This first-stage demo controls a WS2812B strip from a Raspberry Pi web page.
The ESP32 generates all animations locally, so USB serial traffic only changes
parameters and does not need to stream every LED frame.

本阶段支持：关灯、常亮、脉冲、闪烁和双向流水。代码注释采用中英双语。

## 1. ESP32-S2 Mini

1. Open `esp32/led_controller/led_controller.ino` in Arduino IDE.
2. Install the ESP32 board package, FastLED, and ArduinoJson 7.x libraries.
3. Set `DATA_PIN` and `NUM_LEDS`. For ESP32-S2 Mini native USB, enable **Tools > USB CDC On Boot > Enabled** so `Serial` uses USB.
4. Upload the sketch and open Serial Monitor at 115200 baud.
5. Close Serial Monitor and connect the ESP32 to the Raspberry Pi with a USB data cable. No ESP32 Wi-Fi setup is needed.

The existing test used GPIO 18 and 30 LEDs, so those are the defaults here.
Connect the ESP32 ground and LED power-supply ground together. Do not power a
full strip from the ESP32 board.

## 2. Raspberry Pi

```bash
cd raspberry_pi
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
ESP32_SERIAL_PORT=/dev/ttyACM0 python app.py
```

Use `ls /dev/serial/by-id/` to find the board; preferably set `ESP32_SERIAL_PORT`
to its full `/dev/serial/by-id/...` path. Native USB commonly uses `/dev/ttyACM0`;
USB-UART boards may use `/dev/ttyUSB0`. The baud rate is 115200.
If access is denied, run `sudo usermod -aG dialout "$USER"`, then log out and back in.
Only one app process should own the port; close serial monitors before starting it.
The first command waits two seconds for a possible board reset. After unplugging,
reconnect the board and send the desired effect again (errors return HTTP 502).

树莓派与 ESP32 现在通过 USB 串口通信；网页仍由树莓派提供。
ESP32-S2 Mini 使用原生 USB 时，烧录前需要启用 USB CDC On Boot。

Open
`http://<raspberry-pi-address>:5000` from a device on the same network.

## 3. First acceptance test / 第一阶段验收

- Select solid, pulse, flash, chase and blackout from the page.
- Change colour, brightness, period, chase width and direction.
- Confirm that blackout interrupts every running effect immediately.
- Confirm that the controller remains responsive while an animation runs.

Flash period is limited to at least 100 ms in firmware. Avoid prolonged
strobing and do not aim flashing lights directly at guests.

## USB protocol

Send one JSON object followed by a newline (`\n`), for example:

```json
{"effect":"chase","r":255,"g":0,"b":0,"brightness":80,"speed_ms":500,"width":4,"direction":"forward"}
```

The ESP32 replies `OK chase` followed by a newline, or `ERR ...` for invalid input.
All fields are required; supported effects are listed above. RGB and brightness
are 0–255, speed is 100–5000 ms, width is 1–255 (clamped to the LED count), and
direction is `forward` or `reverse`. Oversized lines are discarded.
