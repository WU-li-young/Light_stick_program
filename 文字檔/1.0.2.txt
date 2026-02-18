#include <WiFi.h>
#include <WebServer.h>
#include <FastLED.h>

// ================= 請修改這裡 =================
const char* ssid     = ".1.freewifi";    // 保留雙引號
const char* password = "00000000";    // 保留雙引號
// ===========================================

#define LED_PIN     8       // 腳位
#define NUM_LEDS    96      // 總燈珠數
#define HALF_LEDS   48      // 一半的數量
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB

CRGB leds[NUM_LEDS];
WebServer server(80);

int brightness = 100;

// 網頁介面 (新增雙截預覽 + 防呆邏輯)
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1, user-scalable=no">
  <meta charset="UTF-8">
  <title>光棍終極控制台</title>
  <style>
    body { font-family: 'Verdana', sans-serif; background-color: #121212; color: white; text-align: center; margin: 0; padding: 15px; user-select: none; }
    
    /* 雙截燈管預覽區 */
    .stick-container { display: flex; flex-direction: column; align-items: center; margin: 20px 0; gap: 5px; }
    .stick-part { 
      width: 60px; height: 80px; 
      background: #000; 
      border: 3px solid #333; 
      display: flex; align-items: center; justify-content: center;
      font-size: 12px; color: #555; font-weight: bold;
      transition: 0.2s;
    }
    #preview-top { border-radius: 15px 15px 0 0; border-bottom: none; }
    #preview-btm { border-radius: 0 0 15px 15px; border-top: none; }
    
    /* 模式按鈕 */
    .mode-group { display: flex; justify-content: center; gap: 8px; margin-bottom: 20px; }
    .mode-btn { flex: 1; padding: 12px 0; background: #333; border: 2px solid #444; color: #aaa; border-radius: 8px; font-size: 14px; cursor: pointer; }
    .mode-btn.active { border-color: #00e5ff; color: #00e5ff; background: #222; box-shadow: 0 0 10px rgba(0,229,255,0.2); }

    /* 控制面板 */
    .panel { background: #1e1e1e; padding: 15px; border-radius: 15px; margin-bottom: 15px; }
    
    /* 色盤與Hex */
    .color-row { display: flex; align-items: center; justify-content: space-between; gap: 10px; margin-bottom: 15px; }
    input[type=color] { border: none; width: 60px; height: 40px; border-radius: 5px; background: none; }
    input[type=text] { background: #333; border: 1px solid #555; color: white; padding: 8px; border-radius: 5px; width: 80px; text-align: center; font-family: monospace; }
    
    /* RGB拉桿 */
    .slider-row { display: flex; align-items: center; gap: 10px; margin: 8px 0; }
    .lbl { width: 15px; font-weight: bold; }
    input[type=range] { flex-grow: 1; height: 6px; border-radius: 3px; outline: none; -webkit-appearance: none; background: #444; }
    input[type=range]::-webkit-slider-thumb { -webkit-appearance: none; width: 20px; height: 20px; border-radius: 50%; background: #fff; box-shadow: 0 2px 5px rgba(0,0,0,0.5); }

    /* 特別色拉桿背景 */
    #r-s { background: linear-gradient(90deg, #333, #ff4444); }
    #g-s { background: linear-gradient(90deg, #333, #44ff44); }
    #b-s { background: linear-gradient(90deg, #333, #4444ff); }

    .btn-off { width: 100%; padding: 15px; background: #222; border: 2px solid #ff4444; color: #ff4444; border-radius: 10px; font-size: 18px; margin-top: 10px; }
  </style>
</head>
<body>

  <div class="stick-container">
    <div id="preview-top" class="stick-part">TOP</div>
    <div id="preview-btm" class="stick-part">BTM</div>
  </div>

  <div class="mode-group">
    <button id="btn-sync" class="mode-btn active" onclick="setMode(0)">同步 SYNC</button>
    <button id="btn-top" class="mode-btn" onclick="setMode(1)">上端 TOP</button>
    <button id="btn-btm" class="mode-btn" onclick="setMode(2)">下端 BTM</button>
  </div>

  <div class="panel">
    <div class="color-row">
      <span>Color Picker</span>
      <input type="text" id="hexInput" value="#000000" onchange="inputHex(this.value)">
      <input type="color" id="picker" value="#000000" oninput="inputPicker(this.value)">
    </div>

    <div class="slider-row"><span class="lbl" style="color:#f44">R</span><input type="range" id="r-s" max="255" value="0" oninput="inputRGB()"></div>
    <div class="slider-row"><span class="lbl" style="color:#4f4">G</span><input type="range" id="g-s" max="255" value="0" oninput="inputRGB()"></div>
    <div class="slider-row"><span class="lbl" style="color:#44f">B</span><input type="range" id="b-s" max="255" value="0" oninput="inputRGB()"></div>
  </div>

  <div class="panel">
    <div class="slider-row">
      <span class="lbl">☀</span>
      <input type="range" id="bri-s" max="255" value="100" oninput="sendBright(this.value)">
    </div>
  </div>

  <button class="btn-off" onclick="turnOff()">關閉 OFF</button>

  <script>
    var targetMode = 0; // 0=Sync, 1=Top, 2=Btm
    var colorA = "#000000";
    var colorB = "#000000";

    function setMode(m) {
      targetMode = m;
      // UI 更新
      document.querySelectorAll('.mode-btn').forEach(b => b.classList.remove('active'));
      if(m===0) document.getElementById('btn-sync').classList.add('active');
      if(m===1) document.getElementById('btn-top').classList.add('active');
      if(m===2) document.getElementById('btn-btm').classList.add('active');
      
      // 當切換模式時，將控制面板的顏色設為目前該區段的顏色 (UX優化)
      if(m===1) updateUI(colorA); 
      else if(m===2) updateUI(colorB);
      // 同步模式就不動，保留最後操作的顏色
    }

    // 核心邏輯：所有顏色輸入最後都彙整到這裡
    function sendColor(hex) {
      var r = parseInt(hex.slice(1, 3), 16);
      var g = parseInt(hex.slice(3, 5), 16);
      var b = parseInt(hex.slice(5, 7), 16);

      // 1. 更新虛擬燈管 (立刻給回饋)
      var top = document.getElementById('preview-top');
      var btm = document.getElementById('preview-btm');
      
      if (targetMode === 0) { // Sync
        colorA = hex; colorB = hex;
        top.style.backgroundColor = hex; top.style.boxShadow = "0 0 15px " + hex;
        btm.style.backgroundColor = hex; btm.style.boxShadow = "0 0 15px " + hex;
      } else if (targetMode === 1) { // Top Only
        colorA = hex;
        top.style.backgroundColor = hex; top.style.boxShadow = "0 0 15px " + hex;
      } else if (targetMode === 2) { // Btm Only
        colorB = hex;
        btm.style.backgroundColor = hex; btm.style.boxShadow = "0 0 15px " + hex;
      }

      // 2. 發送指令給 ESP32 (這行解決了所有的 bug)
      // 我們把「目標 (target)」跟「顏色」包在一起傳
      fetch("/update?t=" + targetMode + "&r=" + r + "&g=" + g + "&b=" + b);
    }

    // --- 輸入介面連動 ---
    function updateUI(hex) {
      document.getElementById('picker').value = hex;
      document.getElementById('hexInput').value = hex;
      var rgb = hexToRgb(hex);
      document.getElementById('r-s').value = rgb.r;
      document.getElementById('g-s').value = rgb.g;
      document.getElementById('b-s').value = rgb.b;
    }

    function inputPicker(hex) {
      updateUI(hex);
      sendColor(hex);
    }

    function inputHex(hex) {
      if(!hex.startsWith('#')) hex = '#' + hex;
      if(hex.length === 7) {
        updateUI(hex);
        sendColor(hex);
      }
    }

    function inputRGB() {
      var r = parseInt(document.getElementById('r-s').value);
      var g = parseInt(document.getElementById('g-s').value);
      var b = parseInt(document.getElementById('b-s').value);
      var hex = "#" + ((1 << 24) + (r << 16) + (g << 8) + b).toString(16).slice(1);
      
      document.getElementById('picker').value = hex;
      document.getElementById('hexInput').value = hex;
      sendColor(hex);
    }

    function sendBright(val) { fetch("/bright?val=" + val); }
    function turnOff() { 
      fetch("/off"); 
      updateUI("#000000");
      sendColor("#000000"); 
    }
    
    function hexToRgb(hex) {
      var res = /^#?([a-f\d]{2})([a-f\d]{2})([a-f\d]{2})$/i.exec(hex);
      return res ? { r: parseInt(res[1], 16), g: parseInt(res[2], 16), b: parseInt(res[3], 16) } : {r:0,g:0,b:0};
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
  Serial.print("IP: http://"); Serial.println(WiFi.localIP());

  server.on("/", []() { server.send(200, "text/html", index_html); });

  // 統一的更新接口 (Stateless)
  server.on("/update", []() {
    int target = server.arg("t").toInt(); // 0=Sync, 1=Top, 2=Btm
    int r = server.arg("r").toInt();
    int g = server.arg("g").toInt();
    int b = server.arg("b").toInt();
    CRGB color = CRGB(r, g, b);

    if (target == 0) {       // Sync: 全部改
      fill_solid(leds, NUM_LEDS, color);
    } else if (target == 1) { // Top: 改前48顆
      fill_solid(leds, HALF_LEDS, color);
    } else if (target == 2) { // Btm: 改後48顆
      fill_solid(leds + HALF_LEDS, HALF_LEDS, color);
    }
    
    FastLED.show();
    server.send(200, "text/plain", "OK");
  });

  server.on("/bright", []() {
    brightness = server.arg("val").toInt();
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