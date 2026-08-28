#include <FastLED.h>
#include <WiFi.h>
#include <WebServer.h>

// ================= Hardware configuration / 硬件配置 =================
#define DATA_PIN 18
#define NUM_LEDS 30
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB

// Replace these values before uploading. / 烧录前请填写树莓派热点或路由器信息。
const char* WIFI_SSID = "YOUR_WIFI_NAME";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

CRGB leds[NUM_LEDS];
WebServer server(80);

enum class Effect { BLACKOUT, SOLID, FLASH, PULSE, CHASE };

struct LedState {
  Effect effect = Effect::BLACKOUT;
  CRGB color = CRGB::Red;
  uint8_t brightness = 80;
  uint16_t speedMs = 500;
  bool reverse = false;
  uint8_t width = 4;
};

LedState state;
uint32_t effectStartedAt = 0;

// Keep a number inside a safe range. / 将数值限制在安全范围内。
long clampValue(long value, long minimum, long maximum) {
  return max(minimum, min(value, maximum));
}

const char* effectName(Effect effect) {
  switch (effect) {
    case Effect::BLACKOUT: return "blackout";
    case Effect::SOLID: return "solid";
    case Effect::FLASH: return "flash";
    case Effect::PULSE: return "pulse";
    case Effect::CHASE: return "chase";
  }
  return "blackout";
}

bool parseEffect(const String& value, Effect& output) {
  if (value == "blackout") output = Effect::BLACKOUT;
  else if (value == "solid") output = Effect::SOLID;
  else if (value == "flash") output = Effect::FLASH;
  else if (value == "pulse") output = Effect::PULSE;
  else if (value == "chase") output = Effect::CHASE;
  else return false;
  return true;
}

void sendState() {
  String json = "{\"effect\":\"" + String(effectName(state.effect)) + "\",";
  json += "\"r\":" + String(state.color.r) + ",";
  json += "\"g\":" + String(state.color.g) + ",";
  json += "\"b\":" + String(state.color.b) + ",";
  json += "\"brightness\":" + String(state.brightness) + ",";
  json += "\"speed_ms\":" + String(state.speedMs) + ",";
  json += "\"direction\":\"" + String(state.reverse ? "reverse" : "forward") + "\",";
  json += "\"width\":" + String(state.width) + "}";
  server.send(200, "application/json", json);
}

void handleSetState() {
  Effect nextEffect = state.effect;
  if (server.hasArg("effect") && !parseEffect(server.arg("effect"), nextEffect)) {
    server.send(400, "application/json", "{\"error\":\"unknown effect\"}");
    return;
  }

  state.effect = nextEffect;
  if (server.hasArg("r")) state.color.r = clampValue(server.arg("r").toInt(), 0, 255);
  if (server.hasArg("g")) state.color.g = clampValue(server.arg("g").toInt(), 0, 255);
  if (server.hasArg("b")) state.color.b = clampValue(server.arg("b").toInt(), 0, 255);
  if (server.hasArg("brightness")) {
    state.brightness = clampValue(server.arg("brightness").toInt(), 0, 255);
  }
  if (server.hasArg("speed_ms")) {
    // 100 ms minimum prevents unsafe high-frequency flashing. / 最短100毫秒，限制高速频闪。
    state.speedMs = clampValue(server.arg("speed_ms").toInt(), 100, 5000);
  }
  if (server.hasArg("direction")) state.reverse = server.arg("direction") == "reverse";
  if (server.hasArg("width")) state.width = clampValue(server.arg("width").toInt(), 1, NUM_LEDS);

  effectStartedAt = millis();
  sendState();
}

void updateEffect(uint32_t now) {
  const uint32_t elapsed = now - effectStartedAt;
  FastLED.setBrightness(state.brightness);

  switch (state.effect) {
    case Effect::BLACKOUT:
      fill_solid(leds, NUM_LEDS, CRGB::Black);
      break;

    case Effect::SOLID:
      fill_solid(leds, NUM_LEDS, state.color);
      break;

    case Effect::FLASH: {
      const bool lightOn = ((elapsed / state.speedMs) % 2) == 0;
      fill_solid(leds, NUM_LEDS, lightOn ? state.color : CRGB::Black);
      break;
    }

    case Effect::PULSE: {
      // One beat rises immediately and fades out. / 每拍瞬间亮起，然后逐渐衰减。
      const uint16_t phase = elapsed % state.speedMs;
      const uint8_t level = 255 - ((uint32_t)phase * 255 / state.speedMs);
      CRGB pulseColor = state.color;
      pulseColor.nscale8_video(level);
      fill_solid(leds, NUM_LEDS, pulseColor);
      break;
    }

    case Effect::CHASE: {
      fill_solid(leds, NUM_LEDS, CRGB::Black);
      const uint16_t calculatedStepMs = state.speedMs / NUM_LEDS;
      const uint16_t stepMs = calculatedStepMs < 20 ? 20 : calculatedStepMs;
      int head = (elapsed / stepMs) % NUM_LEDS;
      if (state.reverse) head = NUM_LEDS - 1 - head;

      for (uint8_t offset = 0; offset < state.width; ++offset) {
        int index = state.reverse ? head + offset : head - offset;
        while (index < 0) index += NUM_LEDS;
        index %= NUM_LEDS;
        leds[index] = state.color;
        leds[index].nscale8_video(255 - ((uint16_t)offset * 180 / state.width));
      }
      break;
    }
  }

  FastLED.show();
}

void setup() {
  Serial.begin(115200);
  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(state.brightness);
  FastLED.clear(true);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi / 正在连接 Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("ESP32 address / ESP32 地址: http://");
  Serial.println(WiFi.localIP());

  server.on("/api/state", HTTP_GET, sendState);
  server.on("/api/state", HTTP_POST, handleSetState);
  server.onNotFound([]() {
    server.send(404, "application/json", "{\"error\":\"not found\"}");
  });
  server.begin();
}

void loop() {
  server.handleClient();
  updateEffect(millis());
  delay(5);  // Yield to Wi-Fi; this is not an animation delay. / 给Wi-Fi任务让出运行时间。
}
