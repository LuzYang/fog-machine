# entrance-light demo

This first-stage demo controls a WS2812B strip from a Raspberry Pi web page.
The ESP32 generates all animations locally, so network traffic only changes
parameters and does not need to stream every LED frame.

本阶段支持：关灯、常亮、脉冲、闪烁和双向流水。代码注释采用中英双语。

## 1. ESP32-S2 Mini

1. Open `esp32/led_controller/led_controller.ino` in Arduino IDE.
2. Install the ESP32 board package and the FastLED library.
3. Set `WIFI_SSID`, `WIFI_PASSWORD`, `DATA_PIN` and `NUM_LEDS`.
4. Upload the sketch and open Serial Monitor at 115200 baud.
5. Record the printed ESP32 IP address.

The existing test used GPIO 18 and 30 LEDs, so those are the defaults here.
Connect the ESP32 ground and LED power-supply ground together. Do not power a
full strip from the ESP32 board.

## 2. Raspberry Pi

```bash
cd raspberry_pi
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
ESP32_BASE_URL=http://192.168.1.50 python app.py
```

Replace `192.168.1.50` with the address printed by the ESP32. Open
`http://<raspberry-pi-address>:5000` from a device on the same network.

## 3. First acceptance test / 第一阶段验收

- Select solid, pulse, flash, chase and blackout from the page.
- Change colour, brightness, period, chase width and direction.
- Confirm that blackout interrupts every running effect immediately.
- Confirm that the controller remains responsive while an animation runs.

Flash period is limited to at least 100 ms in firmware. Avoid prolonged
strobing and do not aim flashing lights directly at guests.
