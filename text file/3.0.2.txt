#include <WiFi.h>
#include <WebServer.h>
#include <FastLED.h>

// ================= 設定區 =================
const char* ap_ssid     = "My_Light_Stick"; 
const char* ap_password = "12345678";
// =========================================

#define LED_PIN     8
#define NUM_LEDS    120
#define HALF_LEDS   60
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB

CRGB leds[NUM_LEDS];
WebServer server(80);

// --- 系統變數 ---
uint8_t brightness = 100;
uint8_t effectSpeed = 20; // 速度變數

// --- 狀態變數 ---
// 開機預設為 3 (指定色輪替)
int modeA = 3; 
int modeB = 3;
CRGB colorA = CRGB::Red;   
CRGB colorB = CRGB::Blue;

// 定義指定的 6 種顏色 (紅, 黃, 綠, 藍, 紫, 白)
CRGB cycleColors[] = {CRGB::Red, CRGB::Yellow, CRGB::Green, CRGB::Blue, CRGB::Purple, CRGB::White};

// --- 網頁介面 ---
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1, user-scalable=no">
  <meta charset="UTF-8">
  <title>光棍控制台 V13</title>
  <style>
    body { font-family: 'Verdana', sans-serif; background-color: #000; color: white; text-align: center; margin: 0; padding: 10px; user-select: none; }
    
    /* 動畫定義 */
    @keyframes rainbow-anim { 0% { background-position: 0% 50%; } 100% { background-position: 100% 50%; } }
    @keyframes breathe-anim { 0% { opacity: 0.3; } 50% { opacity: 1; } 100% { opacity: 0.3; } }
    
    /* [修改] 6色輪替動畫 (紅黃綠藍紫白) */
    @keyframes cycle-anim { 
      0%, 16% { background-color: #ff0000; } 
      17%, 32% { background-color: #ffff00; } 
      33%, 48% { background-color: #00ff00; } 
      49%, 64% { background-color: #0000ff; } 
      65%, 80% { background-color: #800080; } 
      81%, 100% { background-color: #ffffff; } 
    }
    
    /* [新增] 派對動感動畫 (模擬閃爍) */
    @keyframes party-anim {
      0% { background-color: #f00; } 25% { background-color: #0f0; } 50% { background-color: #00f; } 75% { background-color: #fff; } 100% { background-color: #f00; }
    }

    .stick-container { display: flex; flex-direction: column; align-items: center; margin: 20px 0; gap: 5px; }
    .stick-part { width: 80px; height: 100px; background: #222; border: 2px solid #444; display: flex; align-items: center; justify-content: center; font-size: 16px; color: #fff; font-weight: bold; text-shadow: 1px 1px 2px black; transition: background 0.3s; }
    #preview-top { border-radius: 15px 15px 0 0; border-bottom: none; }
    #preview-btm { border-radius: 0 0 15px 15px; border-top: none; }

    .fx-rainbow { background: linear-gradient(270deg, #f00, #ff0, #0f0, #0ff, #00f, #f0f, #f00); background-size: 400%; animation: rainbow-anim 3s ease infinite; color: black; text-shadow: none; }
    .fx-breathe { animation: breathe-anim 2s ease-in-out infinite; }
    .fx-cycle { animation: cycle-anim 6s step-end infinite; } /* step-end 創造硬切換效果 */
    .fx-party { animation: party-anim 0.5s steps(1) infinite; } /* 快速閃爍 */

    .panel { background: #1a1a1a; padding: 15px; border-radius: 15px; margin-bottom: 15px; border: 1px solid #333; }
    .mode-group { display: flex; gap: 5px; margin-bottom: 15px; }
    .mode-btn { flex: 1; padding: 12px; background: #333; border: 2px solid #444; color: #aaa; border-radius: 8px; cursor: pointer; font-weight: bold; }
    .mode-btn.active { border-color: #00e5ff; color: #000; background: #00e5ff; box-shadow: 0 0 15px #00e5ff; }

    /* 按鈕網格 */
    .fx-grid { display: grid; grid-template-columns: 1fr 1fr 1fr; gap: 8px; margin-bottom: 20px; }
    .fx-btn { padding: 15px 5px; background: #222; border: 1px solid #444; color: #fff; border-radius: 8px; cursor: pointer; font-size: 14px; }
    .fx-btn.active { background: #ff9800; color: #000; border-color: #ff9800; font-weight: bold; }

    .color-wrapper { position: relative; width: 100%; height: 60px; margin: 10px 0; }
    input[type=color] { position: absolute; top: 0; left: 0; width: 100%; height: 100%; opacity: 0; cursor: pointer; z-index: 2; }
    .color-btn-visual { width: 100%; height: 100%; background: linear-gradient(90deg, #f00, #ff0, #0f0, #0ff, #00f, #f0f, #f00); border-radius: 10px; display: flex; align-items: center; justify-content: center; font-weight: bold; text-shadow: 1px 1px 2px black; border: 2px solid white; }
    
    .row { display: flex; align-items: center; gap: 10px; margin: 15px 0; justify-content: space-between; text-align: left; }
    input[type=range] { flex-grow: 1; height: 10px; border-radius: 5px; background: #444; outline: none; -webkit-appearance: none; }
    input[type=range]::-webkit-slider-thumb { -webkit-appearance: none; width: 26px; height: 26px; border-radius: 50%; background: #fff; border: 2px solid #000; }
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
      <button class="fx-btn" id="fx-0" onclick="setEffect(0)">🔵 靜態</button>
      <button class="fx-btn" id="fx-1" onclick="setEffect(1)">🌈 彩虹</button>
      <button class="fx-btn" id="fx-2" onclick="setEffect(2)">🫁 呼吸</button>
      <button class="fx-btn active" id="fx-3" onclick="setEffect(3)">🔄 6色輪替</button>
      <button class="fx-btn" id="fx-4" onclick="setEffect(4)">🎉 派對動感</button>
    </div>
    <div class="color-wrapper">
      <div class="color-btn-visual" id="color-visual">點擊選擇顏色</div>
      <input type="color" id="picker" value="#ff0000" oninput="setColor(this.value)">
    </div>
    <div class="row"><span>⚡ 速度</span><input type="range" min="1" max="100" value="30" oninput="setSpeed(this.value)"></div>
    <div class="row"><span>☀ 亮度</span><input type="range" min="0" max="255" value="100" oninput="setBri(this.value)"></div>
  </div>
  <button class="btn-off" onclick="turnOff()">關閉電源 OFF</button>
  <script>
    var target = 0; var state = { modeA: 3, colorA: '#ff0000', modeB: 3, colorB: '#0000ff' };
    window.onload = function() { applyPreview(); };

    function setTarget(t) {
      target = t;
      document.querySelectorAll('.mode-btn').forEach(b => b.classList.remove('active'));
      if(t===0) document.getElementById('btn-sync').classList.add('active');
      if(t===1) document.getElementById('btn-top').classList.add('active');
      if(t===2) document.getElementById('btn-btm').classList.add('active');
      var currentMode = (t===2) ? state.modeB : state.modeA; highlightFxBtn(currentMode);
    }
    function setEffect(mode) {
      highlightFxBtn(mode);
      if(target === 0) { state.modeA = mode; state.modeB = mode; }
      else if(target === 1) state.modeA = mode;
      else if(target === 2) state.modeB = mode;
      applyPreview(); fetch("/mode?t=" + target + "&m=" + mode);
    }
    function setColor(hex) {
      document.getElementById('color-visual').style.background = hex;
      document.getElementById('color-visual').innerText = hex;
      document.getElementById('color-visual').style.color = getContrastYIQ(hex);
      highlightFxBtn(0);
      if(target === 0) { state.colorA = hex; state.colorB = hex; state.modeA = 0; state.modeB = 0; }
      else if(target === 1) { state.colorA = hex; state.modeA = 0; }
      else if(target === 2) { state.colorB = hex; state.modeB = 0; }
      applyPreview();
      var r = parseInt(hex.slice(1, 3), 16); var g = parseInt(hex.slice(3, 5), 16); var b = parseInt(hex.slice(5, 7), 16);
      fetch("/color?t=" + target + "&r=" + r + "&g=" + g + "&b=" + b);
    }
    function setSpeed(val) { var sec = (101-val) / 20; document.documentElement.style.setProperty('--anim-speed', sec + 's'); fetch("/speed?val=" + val); }
    function setBri(val) { fetch("/bri?val=" + val); }
    function turnOff() { fetch("/off"); state.modeA=0; state.modeB=0; state.colorA='#000000'; state.colorB='#000000'; applyPreview(); }
    
    function applyPreview() { applyStyle(document.getElementById('preview-top'), state.modeA, state.colorA); applyStyle(document.getElementById('preview-btm'), state.modeB, state.colorB); }
    
    function applyStyle(el, mode, color) {
      el.className = 'stick-part'; el.style.background = ''; el.style.animationDuration = '';
      if (mode === 0) el.style.background = color;
      else if (mode === 1) el.classList.add('fx-rainbow');
      else if (mode === 2) { el.classList.add('fx-breathe'); el.style.backgroundColor = color; }
      else if (mode === 3) { el.classList.add('fx-cycle'); } /* 6色輪替 */
      else if (mode === 4) { el.classList.add('fx-party'); } /* 派對動感 */
    }
    function highlightFxBtn(m) { document.querySelectorAll('.fx-btn').forEach(b => b.classList.remove('active')); var btn = document.getElementById('fx-'+m); if(btn) btn.classList.add('active'); }
    function getContrastYIQ(hexcolor){ hexcolor = hexcolor.replace("#", ""); var r = parseInt(hexcolor.substr(0,2),16); var g = parseInt(hexcolor.substr(2,2),16); var b = parseInt(hexcolor.substr(4,2),16); var yiq = ((r*299)+(g*587)+(b*114))/1000; return (yiq >= 128) ? 'black' : 'white'; }
  </script>
</body>
</html>
)rawliteral";

// --- ESP32 動畫引擎 ---
void drawSegment(int start, int len, int mode, CRGB staticColor, uint8_t beat, uint8_t beatSlow) {
  if (mode == 0) { // 靜態
    fill_solid(leds + start, len, staticColor);
  } 
  else if (mode == 1) { // 彩虹
    fill_rainbow(leds + start, len, beat, 5); 
  }
  else if (mode == 2) { // 呼吸
    uint8_t bri = cubicwave8(beatSlow); 
    fill_solid(leds + start, len, staticColor);
    for(int i=start; i<start+len; i++) leds[i].nscale8(bri);
  }
  else if (mode == 3) { 
    // [修改] 6色硬切換輪替 (紅, 黃, 綠, 藍, 紫, 白)
    // 計算方式：利用 millis() 讓它隨著真實時間切換
    // 速度公式：effectSpeed 越大 -> denominator 越小 -> 變化越快
    // 為了讓它"慢"，我們把分母設大一點
    unsigned long interval = 3000 - (effectSpeed * 25); // 速度 1~100，間隔約 0.5秒 ~ 3秒
    if (interval < 100) interval = 100; // 最小間隔保護
    
    int index = (millis() / interval) % 6; 
    fill_solid(leds + start, len, cycleColors[index]);
  }
  else if (mode == 4) {
    // [新增] 派對動感 (模擬 duoCo StripX 的 Strobe/Jump)
    // 快速閃爍效果
    uint8_t flash = beat8(effectSpeed * 3); // 速度加倍
    if(flash > 128) {
      fill_rainbow(leds + start, len, millis()/10, 20); // 隨機彩虹
    } else {
      fill_solid(leds + start, len, CRGB::White); // 閃白光
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(100); 
  Serial.println("\n=== ESP32 V13 啟動 ===");
  
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(100); 
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();

  WiFi.mode(WIFI_AP);
  WiFi.setTxPower(WIFI_POWER_11dBm); // 穩定連線設定
  WiFi.softAP(ap_ssid, ap_password);
  
  Serial.print("IP: http://"); Serial.println(WiFi.softAPIP());

  server.on("/", []() { server.send(200, "text/html", index_html); });
  server.on("/mode", []() {
    int t = server.arg("t").toInt(); int m = server.arg("m").toInt();
    if (t == 0) { modeA = m; modeB = m; } else if (t == 1) { modeA = m; } else if (t == 2) { modeB = m; }
    server.send(200, "text/plain", "OK");
  });
  server.on("/color", []() {
    int t = server.arg("t").toInt(); int r = server.arg("r").toInt(); int g = server.arg("g").toInt(); int b = server.arg("b").toInt();
    CRGB c = CRGB(r, g, b);
    if (t == 0) { colorA = c; colorB = c; modeA = 0; modeB = 0; }
    else if (t == 1) { colorA = c; modeA = 0; }
    else if (t == 2) { colorB = c; modeB = 0; }
    server.send(200, "text/plain", "OK");
  });
  server.on("/speed", []() { effectSpeed = server.arg("val").toInt(); server.send(200, "text/plain", "OK"); });
  server.on("/bri", []() { FastLED.setBrightness(server.arg("val").toInt()); server.send(200, "text/plain", "OK"); });
  server.on("/off", []() { modeA=0; modeB=0; colorA=CRGB::Black; colorB=CRGB::Black; server.send(200, "text/plain", "OK"); });

  server.begin();
}

void loop() {
  server.handleClient(); 

  uint8_t beat = beat8(effectSpeed);
  uint8_t beatSlow = beat8(effectSpeed / 2); 

  drawSegment(0, HALF_LEDS, modeA, colorA, beat, beatSlow);
  drawSegment(HALF_LEDS, HALF_LEDS, modeB, colorB, beat, beatSlow);

  FastLED.show();
  delay(10); 
}