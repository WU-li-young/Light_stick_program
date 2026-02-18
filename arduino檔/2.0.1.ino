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
uint8_t effectSpeed = 30; // 越小越快

// --- 狀態變數 (0=靜態, 1=彩虹, 2=呼吸, 3=跑馬燈) ---
int modeA = 0; 
int modeB = 0;
// 為了讓特效更豐富，我們紀錄每一區的「基底顏色」
CRGB colorA = CRGB::Red;   
CRGB colorB = CRGB::Blue;

// --- 網頁介面 (包含 CSS 動畫核心) ---
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1, user-scalable=no">
  <meta charset="UTF-8">
  <title>光棍控制台</title>
  <style>
    body { font-family: 'Verdana', sans-serif; background-color: #000; color: white; text-align: center; margin: 0; padding: 10px; user-select: none; }
    
    /* === 1. 動畫定義區 (讓手機畫面動起來的關鍵) === */
    @keyframes rainbow-anim { 
      0% { background-position: 0% 50%; } 
      100% { background-position: 100% 50%; } 
    }
    @keyframes breathe-anim { 
      0% { opacity: 0.3; } 
      50% { opacity: 1; } 
      100% { opacity: 0.3; } 
    }
    @keyframes chase-anim {
      0% { background-position: 0% 0%; }
      100% { background-position: 0% 100%; }
    }

    /* 預覽圖樣式 */
    .stick-container { display: flex; flex-direction: column; align-items: center; margin: 20px 0; gap: 5px; }
    .stick-part { 
      width: 80px; height: 100px; 
      background: #222; 
      border: 2px solid #444; 
      display: flex; align-items: center; justify-content: center;
      font-size: 16px; color: #fff; font-weight: bold; text-shadow: 1px 1px 2px black;
      transition: background 0.3s;
    }
    #preview-top { border-radius: 15px 15px 0 0; border-bottom: none; }
    #preview-btm { border-radius: 0 0 15px 15px; border-top: none; }

    /* 動畫類別 (JS會把這些class加到預覽圖上) */
    .fx-rainbow {
      background: linear-gradient(270deg, #ff0000, #ff7f00, #ffff00, #00ff00, #0000ff, #8b00ff);
      background-size: 400% 400%;
      animation: rainbow-anim 3s ease infinite;
      color: black; text-shadow: none;
    }
    .fx-breathe {
      animation: breathe-anim 2s ease-in-out infinite;
    }
    .fx-chase {
      background: linear-gradient(to bottom, transparent 0%, transparent 40%, #fff 50%, transparent 60%, transparent 100%);
      background-size: 100% 200%;
      animation: chase-anim 1s linear infinite;
    }

    /* === 2. 控制面板 === */
    .panel { background: #1a1a1a; padding: 15px; border-radius: 15px; margin-bottom: 15px; border: 1px solid #333; }
    
    .mode-group { display: flex; gap: 5px; margin-bottom: 15px; }
    .mode-btn { flex: 1; padding: 12px; background: #333; border: 2px solid #444; color: #aaa; border-radius: 8px; cursor: pointer; font-weight: bold; }
    .mode-btn.active { border-color: #00e5ff; color: #000; background: #00e5ff; box-shadow: 0 0 15px #00e5ff; }

    /* 特效按鈕 */
    .fx-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 10px; margin-bottom: 20px; }
    .fx-btn { padding: 15px; background: #222; border: 1px solid #444; color: #fff; border-radius: 8px; cursor: pointer; font-size: 14px; }
    .fx-btn.active { background: #ff9800; color: #000; border-color: #ff9800; font-weight: bold; }

    /* === 3. 重生的大色盤 === */
    .color-wrapper { position: relative; width: 100%; height: 60px; margin: 10px 0; }
    /* 這是一個覆蓋在上面的透明按鈕，確保一定點得到 */
    input[type=color] { 
      position: absolute; top: 0; left: 0; width: 100%; height: 100%; 
      opacity: 0; cursor: pointer; z-index: 2; 
    }
    /* 這是看得到的按鈕外觀 */
    .color-btn-visual {
      width: 100%; height: 100%; 
      background: linear-gradient(90deg, #f00, #ff0, #0f0, #0ff, #00f, #f0f, #f00);
      border-radius: 10px;
      display: flex; align-items: center; justify-content: center;
      font-weight: bold; text-shadow: 1px 1px 2px black;
      border: 2px solid white;
    }

    /* 拉桿 */
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
      <button class="fx-btn active" id="fx-0" onclick="setEffect(0)">🔵 靜態</button>
      <button class="fx-btn" id="fx-1" onclick="setEffect(1)">🌈 彩虹</button>
      <button class="fx-btn" id="fx-2" onclick="setEffect(2)">🫁 呼吸</button>
      <button class="fx-btn" id="fx-3" onclick="setEffect(3)">🏃 跑馬</button>
    </div>

    <div class="color-wrapper">
      <div class="color-btn-visual" id="color-visual">點擊選擇顏色</div>
      <input type="color" id="picker" value="#ff0000" oninput="setColor(this.value)">
    </div>

    <div class="row">
      <span>⚡ 速度</span>
      <input type="range" min="1" max="100" value="30" oninput="setSpeed(this.value)">
    </div>
    
    <div class="row">
      <span>☀ 亮度</span>
      <input type="range" min="0" max="255" value="100" oninput="setBri(this.value)">
    </div>
  </div>

  <button class="btn-off" onclick="turnOff()">關閉電源 OFF</button>

  <script>
    var target = 0; // 0=Sync, 1=Top, 2=Btm
    // 為了預覽，我們在 JS 也紀錄狀態
    var state = {
      modeA: 0, colorA: '#ff0000',
      modeB: 0, colorB: '#0000ff'
    };

    function setTarget(t) {
      target = t;
      document.querySelectorAll('.mode-btn').forEach(b => b.classList.remove('active'));
      if(t===0) document.getElementById('btn-sync').classList.add('active');
      if(t===1) document.getElementById('btn-top').classList.add('active');
      if(t===2) document.getElementById('btn-btm').classList.add('active');
      
      // 切換目標時，更新按鈕狀態以符合該目標目前的特效
      var currentMode = (t===2) ? state.modeB : state.modeA;
      highlightFxBtn(currentMode);
    }

    function setEffect(mode) {
      highlightFxBtn(mode);
      // 更新 JS 狀態
      if(target === 0) { state.modeA = mode; state.modeB = mode; }
      else if(target === 1) state.modeA = mode;
      else if(target === 2) state.modeB = mode;
      
      applyPreview(); // 更新手機動畫
      fetch("/mode?t=" + target + "&m=" + mode);
    }

    function setColor(hex) {
      // 更新按鈕視覺
      document.getElementById('color-visual').style.background = hex;
      document.getElementById('color-visual').innerText = hex;
      document.getElementById('color-visual').style.color = getContrastYIQ(hex);

      // 選色自動切回靜態
      highlightFxBtn(0);
      if(target === 0) { state.colorA = hex; state.colorB = hex; state.modeA = 0; state.modeB = 0; }
      else if(target === 1) { state.colorA = hex; state.modeA = 0; }
      else if(target === 2) { state.colorB = hex; state.modeB = 0; }

      applyPreview();
      
      var r = parseInt(hex.slice(1, 3), 16);
      var g = parseInt(hex.slice(3, 5), 16);
      var b = parseInt(hex.slice(5, 7), 16);
      fetch("/color?t=" + target + "&r=" + r + "&g=" + g + "&b=" + b);
    }

    function setSpeed(val) { 
      // 修改 CSS 動畫速度
      var sec = (101-val) / 20; // 轉換成秒數
      document.documentElement.style.setProperty('--anim-speed', sec + 's');
      fetch("/speed?val=" + val); 
    }
    
    function setBri(val) { fetch("/bri?val=" + val); }
    
    function turnOff() { 
      fetch("/off"); 
      state.modeA=0; state.modeB=0; state.colorA='#000000'; state.colorB='#000000';
      applyPreview();
    }

    // --- 核心：將 CSS 動畫應用到預覽圖 ---
    function applyPreview() {
      var top = document.getElementById('preview-top');
      var btm = document.getElementById('preview-btm');
      
      applyStyle(top, state.modeA, state.colorA);
      applyStyle(btm, state.modeB, state.colorB);
    }

    function applyStyle(el, mode, color) {
      // 先移除所有動畫 class
      el.className = 'stick-part';
      el.style.background = '';
      el.style.animationDuration = '';

      if (mode === 0) { // 靜態
        el.style.background = color;
      } else if (mode === 1) { // 彩虹
        el.classList.add('fx-rainbow');
      } else if (mode === 2) { // 呼吸
        el.classList.add('fx-breathe');
        el.style.backgroundColor = color; // 呼吸要有底色
      } else if (mode === 3) { // 跑馬
        el.classList.add('fx-chase');
        el.style.backgroundColor = color; // 跑馬底色
      }
    }

    function highlightFxBtn(m) {
      document.querySelectorAll('.fx-btn').forEach(b => b.classList.remove('active'));
      document.getElementById('fx-'+m).classList.add('active');
    }

    // 判斷文字顏色要黑還白
    function getContrastYIQ(hexcolor){
      hexcolor = hexcolor.replace("#", "");
      var r = parseInt(hexcolor.substr(0,2),16);
      var g = parseInt(hexcolor.substr(2,2),16);
      var b = parseInt(hexcolor.substr(4,2),16);
      var yiq = ((r*299)+(g*587)+(b*114))/1000;
      return (yiq >= 128) ? 'black' : 'white';
    }
  </script>
</body>
</html>
)rawliteral";

// --- ESP32 動畫引擎 (負責實際燈條) ---
void drawSegment(int start, int len, int mode, CRGB staticColor, uint8_t beat, uint8_t beatSlow) {
  if (mode == 0) { // 靜態
    fill_solid(leds + start, len, staticColor);
  } 
  else if (mode == 1) { // 彩虹
    fill_rainbow(leds + start, len, beat, 5); // beat快速變化產生流動
  }
  else if (mode == 2) { // 呼吸
    // 利用 sin 波產生呼吸感
    uint8_t bri = cubicwave8(beatSlow); 
    fill_solid(leds + start, len, staticColor);
    // 這裡我們不改變全域亮度，而是改變這個區段的亮度
    for(int i=start; i<start+len; i++) leds[i].nscale8(bri);
  }
  else if (mode == 3) { // 跑馬燈
    fill_solid(leds + start, len, CRGB::Black);
    // 產生來回跑動的效果
    int pos = beatsin16(effectSpeed, 0, len-1);
    leds[start + pos] = staticColor;
    // 增加拖尾
    for(int i=1; i<4; i++) {
        if(pos-i >= 0) leds[start + pos - i] = staticColor;
        leds[start + pos - i].fadeToBlackBy(i*60);
    }
  }
}

void setup() {
  Serial.begin(115200);
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(brightness);
  
  WiFi.softAP(ap_ssid, ap_password);
  
  server.on("/", []() { server.send(200, "text/html", index_html); });

  server.on("/mode", []() {
    int t = server.arg("t").toInt();
    int m = server.arg("m").toInt();
    if (t == 0) { modeA = m; modeB = m; }
    else if (t == 1) { modeA = m; }
    else if (t == 2) { modeB = m; }
    server.send(200, "text/plain", "OK");
  });

  server.on("/color", []() {
    int t = server.arg("t").toInt();
    int r = server.arg("r").toInt();
    int g = server.arg("g").toInt();
    int b = server.arg("b").toInt();
    CRGB c = CRGB(r, g, b);
    if (t == 0) { colorA = c; colorB = c; modeA = 0; modeB = 0; } // 選色強制切回靜態
    else if (t == 1) { colorA = c; modeA = 0; }
    else if (t == 2) { colorB = c; modeB = 0; }
    server.send(200, "text/plain", "OK");
  });

  server.on("/speed", []() { 
    // 反轉數值，因為網頁是 1-100 (大=快)，但 beat8 是頻率 (大=快)
    // 但 delay 或是 累積變數通常需要調整
    // 這裡我們直接用原本數值當 BPM
    effectSpeed = server.arg("val").toInt(); 
    server.send(200, "text/plain", "OK"); 
  });
  
  server.on("/bri", []() { FastLED.setBrightness(server.arg("val").toInt()); server.send(200, "text/plain", "OK"); });
  server.on("/off", []() { modeA=0; modeB=0; colorA=CRGB::Black; colorB=CRGB::Black; server.send(200, "text/plain", "OK"); });

  server.begin();
}

void loop() {
  server.handleClient(); 

  // FastLED 的時間基底函數
  // beat8 產生 0-255 的鋸齒波，適合彩虹滾動
  uint8_t beat = beat8(effectSpeed);
  // beatSlow 產生慢速波，適合呼吸
  uint8_t beatSlow = beat8(effectSpeed / 2); // 呼吸通常慢一點

  drawSegment(0, HALF_LEDS, modeA, colorA, beat, beatSlow);
  drawSegment(HALF_LEDS, HALF_LEDS, modeB, colorB, beat + 128, beatSlow + 128); // +128 讓下半部錯開相位

  FastLED.show();
  delay(5);
}