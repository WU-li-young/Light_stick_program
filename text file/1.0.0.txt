#include <WiFi.h>
#include <WebServer.h>
#include <FastLED.h>

// ================= 這裡要改成你家的 Wi-Fi =================
const char* ssid     = ".1.freewifi";     // 請保留雙引號，填入名稱
const char* password = "00000000";     // 請保留雙引號，填入密碼
// ========================================================

// 硬體設定
#define LED_PIN     8       // SuperMini 的燈條訊號腳通常是 8
#define NUM_LEDS    96      // 你的燈珠數量
#define LED_TYPE    WS2812B // 燈條型號
#define COLOR_ORDER GRB     // 如果顏色不對(例如紅變綠)，請改成 RGB

CRGB leds[NUM_LEDS];
WebServer server(80);
int brightness = 100; // 預設亮度 (0-255)

// --- 網頁介面 (HTML/CSS/JS) ---
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1, user-scalable=no">
  <meta charset="UTF-8">
  <title>光棍控制台</title>
  <style>
    body { font-family: Arial; text-align: center; background-color: #1a1a1a; color: white; margin: 0; padding-top: 30px; }
    h1 { color: #00e5ff; margin-bottom: 30px; }
    .container { display: flex; flex-direction: column; align-items: center; gap: 20px; }
    
    /* 調色盤樣式 */
    input[type=color] { border: none; width: 150px; height: 150px; cursor: pointer; background: none; }
    
    /* 亮度拉桿樣式 */
    .slider-container { width: 80%; max-width: 400px; margin: 20px 0; }
    input[type=range] { width: 100%; height: 15px; border-radius: 5px; background: #444; outline: none; -webkit-appearance: none; }
    input[type=range]::-webkit-slider-thumb { -webkit-appearance: none; appearance: none; width: 25px; height: 25px; border-radius: 50%; background: #00e5ff; cursor: pointer; }
    
    /* 按鈕樣式 */
    .btn { background-color: #333; border: 2px solid #555; color: white; padding: 15px 40px; font-size: 18px; margin: 10px; cursor: pointer; border-radius: 30px; transition: 0.3s; }
    .btn:active { background-color: #00e5ff; color: black; }
    .btn-off { border-color: #ff4444; color: #ff4444; }
    .btn-off:active { background-color: #ff4444; color: white; }
  </style>
</head>
<body>
  <div class="container">
    <h1>My Light Stick</h1>
    
    <div>
      <p>點擊方塊選色</p>
      <input type="color" id="colorPicker" oninput="sendColor(this.value)" value="#000000">
    </div>

    <div class="slider-container">
      <p>亮度調整</p>
      <input type="range" min="0" max="255" value="100" oninput="sendBright(this.value)">
    </div>

    <button class="btn btn-off" onclick="turnOff()">關閉燈光 (OFF)</button>
  </div>

  <script>
    // 傳送顏色給 ESP32
    function sendColor(hex) {
      var r = parseInt(hex.slice(1, 3), 16);
      var g = parseInt(hex.slice(3, 5), 16);
      var b = parseInt(hex.slice(5, 7), 16);
      fetch("/set?r=" + r + "&g=" + g + "&b=" + b);
    }
    
    // 傳送亮度給 ESP32
    function sendBright(val) {
      fetch("/bright?val=" + val);
    }

    // 關燈
    function turnOff() {
      document.getElementById('colorPicker').value = '#000000';
      fetch("/off");
    }
  </script>
</body>
</html>
)rawliteral";

// --- 程式初始化 ---
void setup() {
  Serial.begin(115200);
  
  // 1. 設定燈條
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(brightness);
  fill_solid(leds, NUM_LEDS, CRGB::Black); // 開機先全暗
  FastLED.show();

  // 2. 連接 Wi-Fi
  Serial.print("正在連線到 ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  
  // 等待連線
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  // 連線成功，顯示 IP
  Serial.println("");
  Serial.println("Wi-Fi 連線成功!");
  Serial.print("請用手機瀏覽器開啟這個 IP: http://");
  Serial.println(WiFi.localIP());

  // 3. 設定網頁路由
  server.on("/", []() { server.send(200, "text/html", index_html); });
  
  server.on("/set", []() {
    String r = server.arg("r");
    String g = server.arg("g");
    String b = server.arg("b");
    fill_solid(leds, NUM_LEDS, CRGB(r.toInt(), g.toInt(), b.toInt()));
    FastLED.show();
    server.send(200, "text/plain", "OK");
  });

  server.on("/bright", []() {
    String val = server.arg("val");
    brightness = val.toInt();
    FastLED.setBrightness(brightness);
    FastLED.show();
    server.send(200, "text/plain", "OK");
  });

  server.on("/off", []() {
    fill_solid(leds, NUM_LEDS, CRGB::Black);
    FastLED.show();
    server.send(200, "text/plain", "OK");
  });

  server.begin();
  Serial.println("網頁伺服器已啟動");
}

// --- 主迴圈 ---
void loop() {
  server.handleClient(); // 監聽手機指令
}