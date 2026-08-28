# `led_controller.ino` 完整代码讲解

> 适合已有基础 C、Java 和 Python/Flask 经验的读者。

## 1. 程序整体作用

这份程序运行在 ESP32-S2 Mini 中，主要完成以下工作：

1. 连接 Wi-Fi。
2. 在 ESP32 上启动一个简单的 HTTP 服务器。
3. 接收树莓派发送的灯光控制参数。
4. 保存当前灯效、颜色、亮度、速度和方向。
5. 根据时间持续计算每颗 LED 应该显示的颜色。
6. 通过 FastLED 把颜色数据发送给 WS2812B 灯带。

整体流程如下：

```text
ESP32 启动
    ↓
setup() 初始化灯带、Wi-Fi 和 Web 服务器
    ↓
loop() 不断重复
    ├─ 检查有没有 HTTP 请求
    ├─ 根据当前时间计算灯效
    └─ 把颜色发送给真实灯带
```

核心数据流是：

```text
树莓派 HTTP 请求
    ↓ 修改
state 当前灯光状态
    ↓ updateEffect() 读取
leds[] 每颗灯的颜色数组
    ↓ FastLED.show()
真实 WS2812B 灯带
```

> 安全提醒：不要把真实 Wi-Fi 密码提交到 GitHub 或发送给其他人。正式项目应将网络配置与源代码分开保存。

## 2. 引入库

```cpp
#include <FastLED.h>
#include <WiFi.h>
#include <WebServer.h>
```

这与 Python 的 `import` 或 Java 的 `import` 类似。

- `FastLED.h`：控制 WS2812B 灯带。
- `WiFi.h`：让 ESP32 连接 Wi-Fi。
- `WebServer.h`：让 ESP32 接收 HTTP 请求。

## 3. 硬件配置

```cpp
#define DATA_PIN 18
#define NUM_LEDS 30
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB
```

`#define` 会在编译前进行文本替换。例如：

```cpp
#define NUM_LEDS 30
CRGB leds[NUM_LEDS];
```

可以理解为：

```cpp
CRGB leds[30];
```

各项配置的含义：

- `DATA_PIN 18`：ESP32 使用 GPIO 18 发送灯带数据。
- `NUM_LEDS 30`：灯带共有 30 颗 LED。
- `LED_TYPE WS2812B`：灯带型号。
- `COLOR_ORDER GRB`：灯珠实际接收颜色的顺序是绿、红、蓝，FastLED 会自动转换。

## 4. Wi-Fi 配置

```cpp
const char* WIFI_SSID = "YOUR_WIFI_NAME";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
```

`const char*` 可以暂时理解成 C 风格字符串类型。

Java 中近似为：

```java
final String WIFI_SSID = "YOUR_WIFI_NAME";
```

Python 中近似为：

```python
WIFI_SSID = "YOUR_WIFI_NAME"
```

树莓派和 ESP32 必须连接到能够互相访问的网络。

## 5. 灯珠颜色数组

```cpp
CRGB leds[NUM_LEDS];
```

`CRGB` 是 FastLED 定义的颜色类型，内部包含红、绿、蓝三个分量：

```cpp
color.r
color.g
color.b
```

Java 中近似为：

```java
CRGB[] leds = new CRGB[30];
```

数组元素和灯珠的对应关系：

```text
leds[0]  → 第 1 颗灯
leds[1]  → 第 2 颗灯
...
leds[29] → 第 30 颗灯
```

例如：

```cpp
leds[0] = CRGB::Red;
leds[1] = CRGB::Blue;
FastLED.show();
```

修改 `leds[]` 只会修改 ESP32 内存中的颜色表，调用 `FastLED.show()` 后，颜色才会发送到真实灯带。

## 6. ESP32 Web 服务器

```cpp
WebServer server(80);
```

创建一个监听 80 端口的 Web 服务器，和 Flask 有些相似：

```python
app = Flask(__name__)
app.run(port=80)
```

树莓派会向 ESP32 发送类似请求：

```text
POST http://ESP32-IP/api/state
```

树莓派上的 Flask 负责给用户提供网页；ESP32 的 WebServer 只提供简单的控制 API。

## 7. 灯效枚举

```cpp
enum class Effect {
  BLACKOUT,
  SOLID,
  FLASH,
  PULSE,
  CHASE
};
```

`enum class` 定义一组固定选项：

- `BLACKOUT`：关灯。
- `SOLID`：常亮。
- `FLASH`：亮灭闪烁。
- `PULSE`：瞬间亮起后逐渐衰减。
- `CHASE`：流水追逐。

Java 中写法非常接近：

```java
enum Effect {
    BLACKOUT, SOLID, FLASH, PULSE, CHASE
}
```

使用枚举比用数字表达效果更清晰：

```cpp
Effect effect = Effect::CHASE;
```

## 8. `LedState` 状态结构

```cpp
struct LedState {
  Effect effect = Effect::BLACKOUT;
  CRGB color = CRGB::Red;
  uint8_t brightness = 80;
  uint16_t speedMs = 500;
  bool reverse = false;
  uint8_t width = 4;
};
```

`struct` 定义一种新数据类型，可以理解成一个主要用于保存数据的 Java 类：

```java
class LedState {
    Effect effect = Effect.BLACKOUT;
    CRGB color = CRGB.RED;
    int brightness = 80;
    int speedMs = 500;
    boolean reverse = false;
    int width = 4;
}
```

各字段的默认值：

- `effect`：启动时默认关灯。
- `color`：默认使用红色；在 `BLACKOUT` 状态下不会显示。
- `brightness`：默认亮度为 80，范围是 0～255。
- `speedMs`：默认时间参数为 500 毫秒。
- `reverse`：默认正向流水。
- `width`：流水光段默认包含 4 颗灯。

## 9. 创建实际状态对象

```cpp
LedState state;
```

`LedState` 是类型，`state` 是真正创建出来的对象。

Java 中近似为：

```java
LedState state = new LedState();
```

网页控制灯带的本质就是修改这个 `state` 对象。

## 10. 效果开始时间

```cpp
uint32_t effectStartedAt = 0;
```

这个变量保存当前效果开始时的时间，单位是毫秒。

ESP32 的：

```cpp
millis()
```

返回 ESP32 开机后经过的毫秒数。切换效果时执行：

```cpp
effectStartedAt = millis();
```

之后通过：

```cpp
millis() - effectStartedAt
```

计算当前效果已经运行了多久。

## 11. 限制数字范围

```cpp
long clampValue(long value, long minimum, long maximum) {
  return max(minimum, min(value, maximum));
}
```

这个函数确保数值不会超出指定范围。

```text
clampValue(300, 0, 255)  → 255
clampValue(-20, 0, 255)  → 0
clampValue(100, 0, 255)  → 100
```

Python 中可以写成：

```python
def clamp_value(value, minimum, maximum):
    return max(minimum, min(value, maximum))
```

它用于保护 RGB、亮度、速度和流水宽度等参数。

## 12. 枚举与字符串互相转换

### 枚举转换成字符串

```cpp
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
```

ESP32 内部使用 `Effect::CHASE`，但 JSON 中使用字符串 `"chase"`。这个函数负责两者之间的转换。

### 字符串转换成枚举

```cpp
bool parseEffect(const String& value, Effect& output) {
  if (value == "blackout") output = Effect::BLACKOUT;
  else if (value == "solid") output = Effect::SOLID;
  else if (value == "flash") output = Effect::FLASH;
  else if (value == "pulse") output = Effect::PULSE;
  else if (value == "chase") output = Effect::CHASE;
  else return false;
  return true;
}
```

它执行相反转换：

```text
"chase" → Effect::CHASE
```

- 返回 `true`：转换成功。
- 返回 `false`：效果名称不存在。
- `const String& value`：读取字符串但不修改它，也避免不必要的复制。
- `Effect& output`：引用参数，允许函数直接修改调用者传入的变量。

## 13. 返回当前状态

```cpp
void sendState()
```

这个函数把 `state` 拼接成 JSON，例如：

```json
{
  "effect": "chase",
  "r": 255,
  "g": 0,
  "b": 0,
  "brightness": 80,
  "speed_ms": 500,
  "direction": "forward",
  "width": 4
}
```

最后发送响应：

```cpp
server.send(200, "application/json", json);
```

含义是：

- 状态码 `200`：请求成功。
- 内容类型 `application/json`：响应是 JSON。
- `json`：实际响应内容。

Flask 中近似为：

```python
return jsonify(state), 200
```

代码中的三元表达式：

```cpp
state.reverse ? "reverse" : "forward"
```

等价于：

```cpp
if (state.reverse) {
  // 使用 "reverse"
} else {
  // 使用 "forward"
}
```

## 14. 处理控制请求

```cpp
void handleSetState()
```

树莓派发送 POST 请求时，这个函数读取参数并修改 `state`。

### 14.1 解析效果

```cpp
Effect nextEffect = state.effect;
```

先复制当前效果。如果请求没有提供新效果，就继续使用原效果。

```cpp
if (server.hasArg("effect") &&
    !parseEffect(server.arg("effect"), nextEffect)) {
  server.send(400, "application/json", "{\"error\":\"unknown effect\"}");
  return;
}
```

- `server.hasArg("effect")`：请求中是否有 `effect` 参数。
- `server.arg("effect")`：取得参数字符串。
- `!parseEffect(...)`：无法识别效果名称。
- HTTP `400`：客户端发送了错误参数。
- `return`：立即结束函数。

验证成功后保存：

```cpp
state.effect = nextEffect;
```

### 14.2 更新颜色

```cpp
if (server.hasArg("r"))
  state.color.r = clampValue(server.arg("r").toInt(), 0, 255);
```

执行顺序：

```text
检查有没有 r
→ 读取字符串
→ toInt() 转换成整数
→ 限制到 0～255
→ 保存到 state.color.r
```

绿色和蓝色的处理方式相同。

### 14.3 更新其他参数

```cpp
state.brightness = clampValue(..., 0, 255);
state.speedMs = clampValue(..., 100, 5000);
state.reverse = server.arg("direction") == "reverse";
state.width = clampValue(..., 1, NUM_LEDS);
```

- 亮度限制为 0～255。
- 时间限制为 100～5000 毫秒，避免设置过快频闪。
- 方向为 `reverse` 时，`state.reverse` 设为 `true`。
- 流水宽度至少是 1，最多等于灯珠总数。

最后重置效果时间，并返回 ESP32 实际采用的状态：

```cpp
effectStartedAt = millis();
sendState();
```

## 15. `updateEffect()`：计算动画

```cpp
void updateEffect(uint32_t now)
```

这是灯效程序的核心函数。

```cpp
const uint32_t elapsed = now - effectStartedAt;
FastLED.setBrightness(state.brightness);
```

- `elapsed`：效果已经运行了多少毫秒。
- `setBrightness()`：设置整条灯带的总亮度。

随后根据当前效果进入不同的 `switch` 分支。

### 15.1 BLACKOUT

```cpp
fill_solid(leds, NUM_LEDS, CRGB::Black);
```

把所有灯设成黑色，即全部关闭。

### 15.2 SOLID

```cpp
fill_solid(leds, NUM_LEDS, state.color);
```

把所有灯设成用户选择的颜色。

### 15.3 FLASH

```cpp
const bool lightOn = ((elapsed / state.speedMs) % 2) == 0;
```

假设 `speedMs = 500`：

```text
0～499 ms       → 亮
500～999 ms     → 灭
1000～1499 ms   → 亮
1500～1999 ms   → 灭
```

然后根据 `lightOn` 选择颜色或黑色：

```cpp
fill_solid(leds, NUM_LEDS,
           lightOn ? state.color : CRGB::Black);
```

注意：这里的 500 毫秒表示亮 500 毫秒、灭 500 毫秒，完整周期是 1000 毫秒。

### 15.4 PULSE

```cpp
const uint16_t phase = elapsed % state.speedMs;
```

`%` 取余让周期不断重复。

```cpp
const uint8_t level =
    255 - ((uint32_t)phase * 255 / state.speedMs);
```

计算得到的亮度会从 255 逐渐下降到接近 0：

```text
周期开始 → 最亮
周期中间 → 一半亮度
周期结束 → 接近熄灭
```

复制颜色并缩放亮度：

```cpp
CRGB pulseColor = state.color;
pulseColor.nscale8_video(level);
fill_solid(leds, NUM_LEDS, pulseColor);
```

这里复制 `state.color`，是为了避免永久修改原始颜色。

这个效果表现为：

```text
瞬间亮起 → 逐渐变暗 → 瞬间重新亮起
```

### 15.5 CHASE

首先关闭所有灯：

```cpp
fill_solid(leds, NUM_LEDS, CRGB::Black);
```

计算移动一颗灯所需的时间：

```cpp
const uint16_t calculatedStepMs = state.speedMs / NUM_LEDS;
const uint16_t stepMs = calculatedStepMs < 20 ? 20 : calculatedStepMs;
```

最短一步是 20 毫秒。

计算流水头部位置：

```cpp
int head = (elapsed / stepMs) % NUM_LEDS;
```

`% NUM_LEDS` 让头部到达末尾后重新回到开头。

反向时：

```cpp
if (state.reverse)
  head = NUM_LEDS - 1 - head;
```

通过循环生成带宽度的光段：

```cpp
for (uint8_t offset = 0; offset < state.width; ++offset) {
  // 计算每颗灯的位置和亮度
}
```

如果 `width = 4`，循环会处理头部及其后面的 3 颗灯。

位置超出灯带范围时，通过加法和取余回到另一端：

```cpp
while (index < 0) index += NUM_LEDS;
index %= NUM_LEDS;
```

最后，离头部越远的灯越暗：

```cpp
leds[index] = state.color;
leds[index].nscale8_video(
    255 - ((uint16_t)offset * 180 / state.width));
```

因此形成带有渐暗尾巴的流水效果。

### 15.6 更新真实灯带

```cpp
FastLED.show();
```

前面的代码只是在修改 `leds[]`，这行才会把颜色数组发送到真实灯带。

## 16. `setup()`：启动时执行一次

```cpp
void setup()
```

ESP32 每次开机或复位时只执行一次。

### 16.1 初始化串口

```cpp
Serial.begin(115200);
```

让 ESP32 可以向 Arduino IDE 串口监视器打印信息。串口监视器也要选择 115200 波特率。

### 16.2 初始化 FastLED

```cpp
FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);
FastLED.setBrightness(state.brightness);
FastLED.clear(true);
```

告诉 FastLED：

- 灯带型号。
- 使用的 GPIO。
- 颜色排列顺序。
- 颜色数组。
- 灯珠数量。

`FastLED.clear(true)` 把所有灯关闭，并立即把结果发送给灯带。

### 16.3 连接 Wi-Fi

```cpp
WiFi.mode(WIFI_STA);
WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
```

`WIFI_STA` 表示 ESP32 作为客户端连接现有路由器，而不是自己创建热点。

```cpp
while (WiFi.status() != WL_CONNECTED) {
  delay(500);
  Serial.print(".");
}
```

程序每 500 毫秒检查一次连接状态。当前写法有一个缺点：如果名称或密码错误，它会一直等待。以后可以增加连接超时和自动重连。

连接成功后打印 ESP32 的 IP：

```cpp
Serial.println(WiFi.localIP());
```

树莓派需要使用这个 IP 地址控制 ESP32。

### 16.4 注册 API

```cpp
server.on("/api/state", HTTP_GET, sendState);
server.on("/api/state", HTTP_POST, handleSetState);
```

Flask 中近似为：

```python
@app.get("/api/state")
def send_state():
    ...

@app.post("/api/state")
def handle_set_state():
    ...
```

未知地址返回 404：

```cpp
server.onNotFound([]() {
  server.send(404, "application/json",
              "{\"error\":\"not found\"}");
});
```

`[]() { ... }` 是 C++ Lambda，可以理解成一个没有名字的小函数。

最后启动服务器：

```cpp
server.begin();
```

## 17. `loop()`：不断重复

```cpp
void loop() {
  server.handleClient();
  updateEffect(millis());
  delay(5);
}
```

可以把 Arduino 的运行方式理解成：

```cpp
setup();

while (true) {
  loop();
}
```

每一轮完成三件事：

1. `server.handleClient()`：检查并处理新的 HTTP 请求。
2. `updateEffect(millis())`：根据当前时间计算下一帧灯效。
3. `delay(5)`：短暂让出运行时间给 Wi-Fi 和系统任务。

这里的 5 毫秒延迟很短，不像动画中使用几百毫秒的 `delay()` 那样长时间阻塞控制。

## 18. 一次请求的完整执行过程

假设树莓派发送：

```text
POST /api/state
effect=chase
r=255
g=0
b=0
brightness=100
speed_ms=900
direction=forward
width=4
```

执行顺序：

```text
1. loop() 调用 server.handleClient()
2. WebServer 发现 POST /api/state
3. 调用 handleSetState()
4. "chase" 转换为 Effect::CHASE
5. RGB 保存为红色
6. brightness 保存为 100
7. speedMs 保存为 900
8. reverse 保存为 false
9. width 保存为 4
10. effectStartedAt 重置为当前时间
11. sendState() 返回确认 JSON
12. loop() 调用 updateEffect()
13. updateEffect() 进入 CHASE 分支
14. 根据 elapsed 计算流水位置
15. 修改 leds[]
16. FastLED.show() 更新真实灯带
17. loop() 继续重复
```

## 19. 最重要的七个概念

不需要一次记住全部语法，先掌握以下主线：

1. `leds[]` 保存每颗灯的颜色。
2. `state` 保存当前灯效的全部参数。
3. `handleSetState()` 根据 HTTP 请求修改 `state`。
4. `updateEffect()` 根据 `state` 和时间修改 `leds[]`。
5. `FastLED.show()` 把颜色数组发送到真实灯带。
6. `setup()` 只运行一次，`loop()` 不断重复。
7. 使用 `millis()` 计算动画，可以避免长时间 `delay()` 阻塞网络控制。

掌握这七点以后，就已经理解了整份程序的主要架构。
