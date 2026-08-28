#include <FastLED.h>

// ================= 参数配置 =================
#define DATA_PIN     18     // 信号线连接的 GPIO 引脚
#define NUM_LEDS     30     // 测试的灯珠数量（根据你的灯带修改）
#define BRIGHTNESS   50     // 亮度限制 (0-255)，测试时建议不要调太高，防止过流
#define LED_TYPE     WS2812B
#define COLOR_ORDER  GRB    // WS2812B 常见的色彩顺序是 GRB

CRGB leds[NUM_LEDS];

void setup() {
  Serial.begin(115200);
  delay(1000); // 启动缓冲
  Serial.println("WS2812B 测试程序启动...");

  // 初始化 FastLED
  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.clear();
  FastLED.show();
}

void loop() {
  Serial.println("--- 基础三原色及白色检测 ---");
  
  // 1. 全红 (Red)
  fill_solid(leds, NUM_LEDS, CRGB::Red);
  FastLED.show();
  delay(1000);

  // 2. 全绿 (Green)
  fill_solid(leds, NUM_LEDS, CRGB::Green);
  FastLED.show();
  delay(1000);

  // 3. 全蓝 (Blue)
  fill_solid(leds, NUM_LEDS, CRGB::Blue);
  FastLED.show();
  delay(1000);

  // 4. 全白 (White) - 消耗功率最大，用来测试供电稳定性
  fill_solid(leds, NUM_LEDS, CRGB::White);
  FastLED.show();
  delay(1000);

  // 5. 关灯重置
  FastLED.clear();
  FastLED.show();
  delay(500);

  Serial.println("--- 彩虹流动动画测试 (持续 5 秒) ---");
  // 6. 彩虹跑马灯测试（检测每个灯珠的 RGB 独立调光和数据传输能力）
  uint32_t startTime = millis();
  uint8_t initialHue = 0;
  
  while (millis() - startTime < 5000) {
    // 生成彩虹色
    fill_rainbow(leds, NUM_LEDS, initialHue, 7);
    FastLED.show();
    initialHue += 2; // 改变色彩偏移量
    delay(20);
  }

  // 再次清屏
  FastLED.clear();
  FastLED.show();
  delay(1000);
}
