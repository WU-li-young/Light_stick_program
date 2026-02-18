#include <WiFi.h>
#include <WebServer.h>
#include <FastLED.h>

// ================= 請修改這裡 =================
const char* ssid     = ".1.freewifi";    // 保留雙引號
const char* password = "00000000";    // 保留雙引號
// ===========================================

#define LED_PIN     8       // 腳位
#define NUM_LEDS    96      // 燈珠數
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB

CRGB leds[NUM_LEDS];
WebServer server(80);
int brightness = 100;

// 升級版網頁介面 (包含虛擬預覽燈)
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1, user-scalable=no">
  <meta charset="UTF-8">
  <title>光棍控制台 V2</title>
  <style>
    body { 
      font-family: 'Verdana', sans-serif; 
      background-color: #121212; 
      color: white; 
      text-align: center; 
      margin: 0; padding: 20px;
      touch-action: manipulation; /* 防止雙擊縮放 */
    }
    h2 { color: #888; font-weight: normal; margin-bottom: 20px; }
    
    /* 虛擬燈泡預覽區 */
    #preview-box {
      width: 150px;
      height: 150px;
      border-radius: 50%;
      background-color: #000;
      margin: 0 auto 30px auto;
      border: 4px solid #333;
      box-shadow: 0 0 20px rgba(0,0,0,0.5);
      transition: background-color 0.2s, box-shadow 0.2s;
    }

    /* 顏色選擇器美化 */
    .control-group { margin-bottom: 30px; }
    input[type=color] { 
      -webkit-appearance: none;
      border: none;
      width: 100px; height: 50px;
      border-radius: 10px;
      background: none;
      cursor: pointer;
    }
    
    /* 亮度拉桿 */
    input[type=range] {
      width: 80%;
      height: 10px;
      border-radius: 5px;
      background: #444;
      outline: none;
      -webkit-appearance: none;
    }
    input[type=range]::-webkit-slider-thumb {
      -webkit-appearance: none;
      width: 24px; height: 24px;
      border-radius: 50%;
      background: #00e5ff;
      cursor: pointer;
    }

    /* 關燈大按鈕 */
    .btn-off {
      background-color: #2a2a2a;
      border: 2px solid #ff4444;
      color: #ff4444;
      padding: 15px 50px;
      font-size: 20px;
      border-radius: 50px;
      cursor: pointer;
      width: 80%;
      max-width: 300px;
      transition: 0.2s;
    }
    .btn-off:active {
      background-color: #ff4444;
      color: white;
      transform: scale(0.95);
    }
  </style>
</head>
<body>

  <h2>目前顏色</h2>
  
  <div id="preview-box"></div>

  <div class="control-group">
    <p>點擊下方色塊選色</p>
    <input type="color" id="colorPicker" oninput="updateColor(this.value)" value="#000000">
  </div>

  <div class="control-group">
    <p>亮度</p>
    <input type="range" min="0" max="255" value="100" oninput="sendBright(this.value)">
  </div>

  <button class="btn-off" onclick="turnOff()">關閉電源 OFF</button>

  <script>
    // 更新顏色 (同時改變網頁預覽 + 傳送給 ESP32)
    function updateColor(hex) {
      // 1. 改變網頁上的圓圈顏色 (視覺回饋)
      var preview = document.getElementById('preview-box');
      preview.style.backgroundColor = hex;
      preview.style.boxShadow = "0 0 30px " + hex; // 讓它看起來在發光

      // 2. 傳送指令給 ESP32
      var r = parseInt(hex.slice(1, 3), 16);
      var g = parseInt(hex.slice(3, 5), 16);
      var b = parseInt(hex.slice(5, 7), 16);
      fetch("/set?r=" + r + "&g=" + g + "&b=" + b);
    }

    function sendBright(val) {
      fetch("/bright?val=" + val);
    }

    function turnOff() {
      // 介面歸零
      document.getElementById('colorPicker').value = '#000000';
      var preview = document.getElementById('preview-box');
      preview.style.backgroundColor = '#000';
      preview.style.boxShadow = 'none';
      
      // 傳送關燈指令
      fetch("/off");
    }
  </script>
</body>
</html>
)rawliteral";

void setup() {
  Serial.begin(115200);
  
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(brightness);
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) delay(500);
  
  Serial.println("");
  Serial.println("Wi-Fi Connected!");
  Serial.print("IP Address: "); // 這裡很重要
  Serial.println(WiFi.localIP());

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
}

void loop() {
  server.handleClient();
}