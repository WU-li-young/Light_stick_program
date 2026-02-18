#include <WiFi.h>
#include <WebServer.h>
#include <FastLED.h>

// ================= 設定區 =================
const char* ap_ssid     = "My_Light_Stick"; 
const char* ap_password = "88888888";
// =========================================

#define LED_PIN     8
#define NUM_LEDS    96
#define HALF_LEDS   48
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB

CRGB leds[NUM_LEDS];
WebServer server(80);

// --- 系統變數 ---
uint8_t brightness = 100;
uint8_t effectSpeed = 30; // 速度 (1-255, 越小越快)

// --- 狀態變數 (0=靜態, 1=彩虹, 2=呼吸, 3=跑馬燈) ---
int modeA = 0; 
int modeB = 0;
CRGB colorA = CRGB::Red;   // A 預設顏色
CRGB colorB = CRGB::Blue;  // B 預設顏色

// --- 網頁介面 ---
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1, user-scalable=no">
  <meta charset="UTF-8">
  <title>光棍特效版 V6</title>
  <style>
    body { font-family: 'Verdana', sans-serif; background-color: #000; color: white; text-align: center; margin: 0; padding: 10px; user-select: none; }
    
    /* 雙截預覽 */
    .stick-container { display: flex; flex-direction: column; align-items: center; margin: 15px 0; gap: 4px; }
    .stick-part { width: 70px; height: 80px; background: #111; border: 2px solid #333; display: flex; align-items: center; justify-content: center; font-size: 14px; color: #555; font-weight: bold; transition: 0.2s; }
    #preview-top { border-radius: 12px 12px 0 0; border-bottom: none; }
    #preview-btm { border-radius: 0 0 12px 12px; border-top: none; }

    /* 控制區塊 */
    .panel { background: #1a1a1a; padding: 15px; border-radius: 12px; margin-bottom: 15px; border: 1px solid #333; }
    
    /* 模式按鈕 (SYNC/TOP/BTM) */
    .mode-group { display: flex; gap: 5px; margin-bottom: 15px; }
    .mode-btn { flex: 1; padding: 12px; background: #333; border: 2px solid #444; color: #aaa; border-radius: 8px; cursor: pointer; font-weight: bold; }
    .mode-btn.active { border-color: #00e5ff; color: #000; background: #00e5ff; }

    /* 特效按鈕矩陣 */
    .fx-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 8px; margin-bottom: 15px; }
    .fx-btn { padding: 15px; background: #222; border: 1px solid #444; color: #fff; border-radius: 8px; cursor: pointer; font-size: 14px; }
    .fx-btn.active { background: #ff9800; color: #000; border-color: #ff9800; font-weight: bold; }

    /* 拉桿與色盤 */
    .row { display: flex; align-items: center; gap: 10px; margin: 10px 0; justify-content: space-between; }
    input[type=range] { flex-grow: 1; height: 8px; border-radius: 4px; background: #444; outline: none; -webkit-appearance: none; }
    input[type=range]::-webkit-slider-thumb { -webkit-appearance: none; width: 24px; height: 24px; border-radius: 50%; background: #fff; }
    input[type=color] { border: none; width: 60px; height: 40px; border-radius: 5px; background: none; }

    .btn-off { width: 100%; padding: 15px; background: #222; border: 2px solid #ff4444; color: #ff4444; border-radius: 10px; font-size: 18px; margin-top: 10px; }
  </style>
</head>
<body>
  <div class="stick-container">
    <div id="preview-top" class="stick-part">TOP</div>
    <div id="preview-btm" class="stick-part">BTM</div>
  </div>

  <div class="mode-group">
    <button id="btn-sync" class="mode-btn active" onclick="setTarget(0)">全合 SYNC</button>
    <button id="btn-top" class="mode-btn" onclick="setTarget(1)">上端 TOP</button>
    <button id="btn-btm" class="mode-btn" onclick="setTarget(2)">下端 BTM</button>
  </div>

  <div class="panel">
    <div class="fx-grid">
      <button class="fx-btn" onclick="setEffect(0)">🔵 靜態色</button>
      <button class="fx-btn" onclick="setEffect(1)">🌈 彩虹流動</button>
      <button class="fx-btn" onclick="setEffect(2)">🫁 呼吸燈</button>
      <button class="fx-btn" onclick="setEffect(3)">🏃 跑馬燈</button>
    </div>

    <div class="row">
      <span>顏色選擇</span>
      <input type="color" id="picker" value="#ff0000" oninput="setColor(this.value)">
    </div>

    <div class="row">
      <span>速度</span>
      <input type="range" min="1" max="100" value="30" oninput="setSpeed(this.value)">
    </div>
    
    <div class="row">
      <span>亮度</span>
      <input type="range" min="0" max="255" value="100" oninput="setBri(this.value)">
    </div>
  </div>

  <button class="btn-off" onclick="turnOff()">關閉電源 OFF</button>

  <script>
    var target = 0; // 0=Sync, 1=Top, 2=Btm
    
    function setTarget(t) {
      target = t;
      document.querySelectorAll('.mode-btn').forEach(b => b.classList.remove('active'));
      if(t===0) document.getElementById('btn-sync').classList.add('active');
      if(t===1) document.getElementById('btn-top').classList.add('active');
      if(t===2) document.getElementById('btn-btm').classList.add('active');
    }

    function setEffect(mode) {
      // UI 反饋 (簡單變色示意)
      document.querySelectorAll('.fx-btn').forEach(b => b.classList.remove('active'));
      event.target.classList.add('active'); // 點擊的按鈕亮起
      
      updatePreview(mode, document.getElementById('picker').value);
      fetch("/mode?t=" + target + "&m=" + mode);
    }

    function setColor(hex) {
      var r = parseInt(hex.slice(1, 3), 16);
      var g = parseInt(hex.slice(3, 5), 16);
      var b = parseInt(hex.slice(5, 7), 16);
      
      // 如果選顏色，自動切回靜態模式(0)
      updatePreview(0, hex);
      fetch("/color?t=" + target + "&r=" + r + "&g=" + g + "&b=" + b);
    }

    function setSpeed(val) { fetch("/speed?val=" + val); }
    function setBri(val) { fetch("/bri?val=" + val); }
    function turnOff() { fetch("/off"); updatePreview(0, '#000000'); }

    function updatePreview(mode, color) {
      var top = document.getElementById('preview-top');
      var btm = document.getElementById('preview-btm');
      var displayColor = (mode === 0) ? color : (mode === 1 ? 'linear-gradient(45deg, red, yellow, green, blue)' : '#555');
      
      if(target === 0) { top.style.background = displayColor; btm.style.background = displayColor; }
      else if(target === 1) { top.style.background = displayColor; }
      else if(target === 2) { btm.style.background = displayColor; }
    }
  </script>
</body>
</html>
)rawliteral";

// --- 動畫核心邏輯 ---
void drawSegment(int start, int len, int mode, CRGB staticColor, uint8_t hue) {
  if (mode == 0) { // 靜態
    fill_solid(leds + start, len, staticColor);
  } 
  else if (mode == 1) { // 彩虹
    fill_rainbow(leds + start, len, hue, 7);
  }
  else if (mode == 2) { // 呼吸
    float breath = (exp(sin(millis()/1000.0*PI)) - 0.36787944)*108.0;
    fill_solid(leds + start, len, staticColor);
    fadeToBlackBy(leds + start, len, 255 - breath);
  }
  else if (mode == 3) { // 跑馬燈
    fill_solid(leds + start, len, CRGB::Black);
    int pos = (millis() / (effectSpeed * 2)) % len;
    leds[start + pos] = staticColor;
    // 拖尾效果
    if(pos > 0) leds[start + pos - 1] = staticColor;
    leds[start + pos - 1].fadeToBlackBy(180);
  }
}

void setup() {
  Serial.begin(115200);
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(brightness);
  
  WiFi.softAP(ap_ssid, ap_password);
  Serial.print("IP: "); Serial.println(WiFi.softAPIP());

  server.on("/", []() { server.send(200, "text/html", index_html); });

  // 設定模式 (0=靜態, 1=彩虹...)
  server.on("/mode", []() {
    int t = server.arg("t").toInt();
    int m = server.arg("m").toInt();
    if (t == 0) { modeA = m; modeB = m; }
    else if (t == 1) { modeA = m; }
    else if (t == 2) { modeB = m; }
    server.send(200, "text/plain", "OK");
  });

  // 設定顏色
  server.on("/color", []() {
    int t = server.arg("t").toInt();
    int r = server.arg("r").toInt();
    int g = server.arg("g").toInt();
    int b = server.arg("b").toInt();
    CRGB c = CRGB(r, g, b);
    if (t == 0) { colorA = c; colorB = c; modeA = 0; modeB = 0; }
    else if (t == 1) { colorA = c; modeA = 0; }
    else if (t == 2) { colorB = c; modeB = 0; }
    server.send(200, "text/plain", "OK");
  });

  server.on("/speed", []() { effectSpeed = 101 - server.arg("val").toInt(); server.send(200, "text/plain", "OK"); });
  server.on("/bri", []() { FastLED.setBrightness(server.arg("val").toInt()); server.send(200, "text/plain", "OK"); });
  server.on("/off", []() { modeA=0; modeB=0; colorA=CRGB::Black; colorB=CRGB::Black; server.send(200, "text/plain", "OK"); });

  server.begin();
}

// --- 主迴圈 (這裡是最重要的動畫引擎) ---
void loop() {
  server.handleClient(); // 處理手機指令

  // 使用 FastLED 的計時器來產生動畫 (Hue 會一直變)
  uint8_t currentHue = beat8(effectSpeed); 

  // 繪製上端 (0 ~ 47)
  drawSegment(0, HALF_LEDS, modeA, colorA, currentHue);

  // 繪製下端 (48 ~ 95)
  // 注意：這裡我們把 Hue 加上 128，讓上下兩段的彩虹顏色剛好錯開 (比較好看)
  drawSegment(HALF_LEDS, HALF_LEDS, modeB, colorB, currentHue + 128);

  FastLED.show();
  delay(10); // 給 Wi-Fi 一點喘息時間，避免過熱
}