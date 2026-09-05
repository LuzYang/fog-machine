#include <FastLED.h>

#define DATA_PIN 18
#define NUM_LEDS 30
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB
#include <ArduinoJson.h>

CRGB leds[NUM_LEDS];

// 程序支持的效果
enum class Effect {
  BLACKOUT,
  SOLID,
  FLASH,
  PULSE,
  CHASE
};

// 当前灯光状态
struct LedState {
  Effect effect = Effect::FLASH;
  CRGB color = CRGB::Red;
  uint8_t brightness = 80;
  uint16_t speedMs = 500;
  bool reverse = false;
	uint8_t width = 4;
};

LedState state;

// 保存当前效果开始时的时间
uint32_t effectStartedAt = 0;

// Receive one JSON object per line over USB serial. / 每行一个 JSON 命令。
void handleSetState(const char* command) {
  JsonDocument doc;
  if (deserializeJson(doc, command) || !doc.is<JsonObject>()) {
    Serial.println("ERR invalid JSON");
    return;
  }
  const char* effect = doc["effect"] | "";
  Effect nextEffect;
  if (strcmp(effect, "blackout") == 0) nextEffect = Effect::BLACKOUT;
  else if (strcmp(effect, "solid") == 0) nextEffect = Effect::SOLID;
  else if (strcmp(effect, "flash") == 0) nextEffect = Effect::FLASH;
  else if (strcmp(effect, "pulse") == 0) nextEffect = Effect::PULSE;
  else if (strcmp(effect, "chase") == 0) nextEffect = Effect::CHASE;
  else { Serial.println("ERR unknown effect"); return; }
  const char* keys[] = {"r", "g", "b", "brightness", "speed_ms", "width"};
  const int minimum[] = {0, 0, 0, 0, 100, 1};
  const int maximum[] = {255, 255, 255, 255, 5000, 255};
  for (int i = 0; i < 6; ++i) {
    if (!doc[keys[i]].is<int>() || doc[keys[i]].as<int>() < minimum[i] ||
        doc[keys[i]].as<int>() > maximum[i]) {
      Serial.println("ERR invalid parameters");
      return;
    }
  }
  const char* direction = doc["direction"] | "";
  if (strcmp(direction, "forward") != 0 && strcmp(direction, "reverse") != 0) {
    Serial.println("ERR invalid direction");
    return;
  }
  state.effect = nextEffect;
  state.color = CRGB(doc["r"].as<int>(), doc["g"].as<int>(), doc["b"].as<int>());
  state.brightness = doc["brightness"].as<int>();
  state.speedMs = doc["speed_ms"].as<int>();
  state.width = min(doc["width"].as<int>(), NUM_LEDS);
  state.reverse = strcmp(direction, "reverse") == 0;
  effectStartedAt = millis();
  Serial.print("OK ");
  Serial.println(effect);
}

void readSerialCommands() {
  static char buffer[384];
  static size_t length = 0;
  static bool overflow = false;
  // Bounded, nonblocking reads keep animations running. / 非阻塞读取。
  for (uint16_t count = 0; count < 384 && Serial.available(); ++count) {
    const char character = Serial.read();
    if (character == '\n') {
      buffer[length] = '\0';
      if (overflow) Serial.println("ERR command too long");
      else if (length != 0) handleSetState(buffer);
      length = 0;
      overflow = false;
    } else if (character != '\r' && !overflow) {
      if (length < sizeof(buffer) - 1) buffer[length++] = character;
      else overflow = true;
    }
  }
}

// 根据当前时间更新灯效
void updateEffect(uint32_t now){
   // 效果已经运行了多久
  uint32_t elapsed = now - effectStartedAt;
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
  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(
    leds,
    NUM_LEDS
  );
}

void loop() {
  readSerialCommands();
  updateEffect(millis());
  
}