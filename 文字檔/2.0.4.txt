#include <WiFi.h>
#include <WebServer.h>
#include <FastLED.h>

// ================= 設定區 (保留你的設定) =================
const char* ap_ssid     = "My_Light_Stick"; 
const char* ap_password = "12345678";
// ===================================================

#define LED_PIN     8
#define NUM_LEDS    120      // 你的燈珠數量
#define HALF_LEDS   60       // 一半
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB

CRGB leds[NUM_LEDS];
WebServer server(80);

// --- 系統變數 ---
uint8_t brightness = 100;
uint8_t effectSpeed = 30; // 越小越快

// --- 狀態變數 ---
int modeA = 0; 
int modeB = 0;
CRGB colorA = CRGB::Red;   
CRGB colorB = CRGB::Blue;

// --- 網頁介面 ---
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1, user-scalable=no">
  <meta charset="UTF-8">
  <title>光棍 V7.5 環形版</title>
  <style>
    body { font-family: 'Verdana', sans-serif; background-color: #000; color: white; text-align: center; margin: 0; padding: 10px; user-select: none; touch-action: none; }
    
    /* 動畫定義 */
    @keyframes rainbow-anim { 0% { background-position: 0% 50%; } 100% { background-position: 100% 50%; } }
    @keyframes breathe-anim { 0% { opacity: 0.3; } 50% { opacity: 1; } 100% { opacity: 0.3; } }
    @keyframes chase-anim { 0% { background-position: 0% 0%; } 100% { background-position: 0% 100%; } }

    /* 預覽圖 */
    .stick-container { display: flex; flex-direction: column; align-items: center; margin: 20px 0; gap: 5px; }
    .stick-part { width: 80px; height: 80px; background: #222; border: 2px solid #444; display: flex; align-items: center; justify-content: center; font-size: 16px; color: #fff; font-weight: bold; text-shadow: 1px 1px 2px black; transition: background 0.3s; }
    #preview-top { border-radius: 15px 15px 0 0; border-bottom: none; }
    #preview-btm { border-radius: 0 0 15px 15px; border-top: none; }

    /* 動畫 Class */
    .fx-rainbow { background: linear-gradient(270deg, #f00, #ff0, #0f0, #0ff, #00f, #f0f); background-size: 400%; animation: rainbow-anim 3s ease infinite; color: black; text-shadow: none; }
    .fx-breathe { animation: breathe-anim 2s ease-in-out infinite; }
    .fx-chase { background: linear-gradient(to bottom, transparent 0%, transparent 40%, #fff 50%, transparent 60%, transparent 100%); background-size: 100% 200%; animation: chase-anim 1s linear infinite; }

    /* 控制面板 */
    .panel { background: #1a1a1a; padding: 15px; border-radius: 15px; margin-bottom: 15px; border: 1px solid #333; }
    .mode-group { display: flex; gap: 5px; margin-bottom: 15px; }
    .mode-btn { flex: 1; padding: 12px; background: #333; border: 2px solid #444; color: #aaa; border-radius: 8px; cursor: pointer; font-weight: bold; }
    .mode-btn.active { border-color: #00e5ff; color: #000; background: #00e5ff; box-shadow: 0 0 15px #00e5ff; }

    /* 特效按鈕 */
    .fx-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 10px; margin-bottom: 20px; }
    .fx-btn { padding: 15px; background: #222; border: 1px solid #444; color: #fff; border-radius: 8px; cursor: pointer; font-size: 14px; }
    .fx-btn.active { background: #ff9800; color: #000; border-color: #ff9800; font-weight: bold; }

    /* === [新增] 環形色盤樣式 === */
    .wheel-container { position: relative; width: 280px; height: 280px; margin: 0 auto 20px auto; }
    #colorWheel { width: 100%; height: 100%; cursor: crosshair; touch-action: none; }

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
    <button id="btn-sync" class="mode-btn active" onclick="setTarget(0)">全合</button>
    <button id="btn-top" class="mode-btn" onclick="setTarget(1)">上端</button>
    <button id="btn-btm" class="mode-btn" onclick="setTarget(2)">下端</button>
  </div>

  <div class="panel">
    <div class="fx-grid">
      <button class="fx-btn active" id="fx-0" onclick="setEffect(0)">靜態</button>
      <button class="fx-btn" id="fx-1" onclick="setEffect(1)">彩虹</button>
      <button class="fx-btn" id="fx-2" onclick="setEffect(2)">呼吸</button>
      <button class="fx-btn" id="fx-3" onclick="setEffect(3)">跑馬</button>
    </div>

    <div class="wheel-container">
      <canvas id="colorWheel"></canvas>
    </div>

    <div class="row"><span>⚡ 速度</span><input type="range" min="1" max="100" value="30" oninput="setSpeed(this.value)"></div>
    <div class="row"><span>☀ 亮度</span><input type="range" min="0" max="255" value="100" oninput="setBri(this.value)"></div>
  </div>

  <button class="btn-off" onclick="turnOff()">關閉電源 OFF</button>

  <script>
    var target = 0; 
    var state = { modeA: 0, colorA: '#ff0000', modeB: 0, colorB: '#0000ff' };

    // === [新增] Canvas 色盤邏輯 ===
    var canvas = document.getElementById('colorWheel');
    var ctx = canvas.getContext('2d');
    var size = 280; 
    canvas.width = size; canvas.height = size;
    var cx = size / 2; var cy = size / 2; var radius = size / 2 - 5;
    var isDragging = false;
    var lastSend = 0; // 限流變數

    function drawWheel() {
      // 1. 畫彩色圓環
      for (var angle = 0; angle < 360; angle++) {
        var startAngle = (angle - 2) * Math.PI / 180;
        var endAngle = (angle + 2) * Math.PI / 180;
        ctx.beginPath();
        ctx.moveTo(cx, cy);
        ctx.arc(cx, cy, radius, startAngle, endAngle);
        ctx.closePath();
        ctx.fillStyle = 'hsl(' + angle + ', 100%, 50%)';
        ctx.fill();
      }
      // 2. 畫中間白色漸層 (飽和度)
      var grd = ctx.createRadialGradient(cx, cy, 0, cx, cy, radius);
      grd.addColorStop(0, 'white');
      grd.addColorStop(1, 'transparent');
      ctx.fillStyle = grd;
      ctx.beginPath(); ctx.arc(cx, cy, radius, 0, 2 * Math.PI); ctx.fill();
    }

    function pickColor(e) {
      var rect = canvas.getBoundingClientRect();
      var x = (e.touches ? e.touches[0].clientX : e.clientX) - rect.left;
      var y = (e.touches ? e.touches[0].clientY : e.clientY) - rect.top;
      x = x * (canvas.width / rect.width);
      y = y * (canvas.height / rect.height);

      var pixel = ctx.getImageData(x, y, 1, 1).data;
      if (pixel[3] < 200) return; // 忽略透明區域

      var hex = "#" + ((1 << 24) + (pixel[0] << 16) + (pixel[1] << 8) + pixel[2]).toString(16).slice(1);
      
      // 限流發送 (50ms 一次)
      var now = Date.now();
      if (now - lastSend > 50) {
        setColor(hex);
        lastSend = now;
      }
    }

    drawWheel();
    // 綁定觸控與滑鼠事件
    canvas.addEventListener('mousedown', function(e){ isDragging = true; pickColor(e); });
    canvas.addEventListener('mousemove', function(e){ if(isDragging) pickColor(e); });
    canvas.addEventListener('mouseup', function(){ isDragging = false; });
    canvas.addEventListener('touchstart', function(e){ isDragging = true; pickColor(e); e.preventDefault(); }, {passive: false});
    canvas.addEventListener('touchmove', function(e){ if(isDragging) pickColor(e); e.preventDefault(); }, {passive: false});
    canvas.addEventListener('touchend', function(){ isDragging = false; });
    // =================================

    function setTarget(t) {
      target = t;
      document.querySelectorAll('.mode-btn').forEach(b => b.classList.remove('active'));
      if(t===0) document.getElementById('btn-sync').classList.add('active');
      if(t===1) document.getElementById('btn-top').classList.add('active');
      if(t===2) document.getElementById('btn-btm').classList.add('active');
      
      var currentMode = (t===2) ? state.modeB : state.modeA;
      highlightFxBtn(currentMode);
    }

    function setEffect(mode) {
      highlightFxBtn(mode);
      if(target === 0) { state.modeA = mode; state.modeB = mode; }
      else if(target === 1) state.modeA = mode;
      else if(target === 2) state.modeB = mode;
      applyPreview();
      fetch("/mode?t=" + target + "&m=" + mode);
    }

    function setColor(hex) {
      // 這裡僅更新顏色，不強制切回靜態，讓你能在呼吸模式下換色
      if(target === 0) { state.colorA = hex; state.colorB = hex; }
      else if(target === 1) { state.colorA = hex; }
      else if(target === 2) { state.colorB = hex; }

      applyPreview();
      
      var r = parseInt(hex.slice(1, 3), 16);
      var g = parseInt(hex.slice(3, 5), 16);
      var b = parseInt(hex.slice(5, 7), 16);
      fetch("/color?t=" + target + "&r=" + r + "&g=" + g + "&b=" + b);
    }

    function setSpeed(val) { 
      var sec = (101-val) / 20; 
      document.documentElement.style.setProperty('--anim-speed', sec + 's');
      fetch("/speed?val=" + val); 
    }
    
    function setBri(val) { fetch("/bri?val=" + val); }
    
    function turnOff() { 
      fetch("/off"); 
      state.modeA=0; state.modeB=0; state.colorA='#000000'; state.colorB='#000000';
      applyPreview();
    }

    function applyPreview() {
      applyStyle(document.getElementById('preview-top'), state.modeA, state.colorA);
      applyStyle(document.getElementById('preview-btm'), state.modeB, state.colorB);
    }

    function applyStyle(el, mode, color) {
      el.className = 'stick-part'; el.style.background = ''; el.style.animationDuration = '';
      if (mode === 0) el.style.background = color;
      else if (mode === 1) el.classList.add('fx-rainbow');
      else if (mode === 2) { el.classList.add('fx-breathe'); el.style.backgroundColor = color; }
      else if (mode === 3) { el.classList.add('fx-chase'); el.style.backgroundColor = color; }
    }

    function highlightFxBtn(m) {
      document.querySelectorAll('.fx-btn').forEach(b => b.classList.remove('active'));
      document.getElementById('fx-'+m).classList.add('active');
    }
  </script>
</body>
</html>
)rawliteral";

// --- ESP32 動畫引擎 (保持不變) ---
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
  else if (mode == 3) { // 跑馬燈
    fill_solid(leds + start, len, CRGB::Black);
    int pos = beatsin16(effectSpeed, 0, len-1);
    leds[start + pos] = staticColor;
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

  // [修改] 這裡把原本會重置 mode 的程式碼拿掉了
  // 這樣你可以一邊跑呼吸燈，一邊用圓盤換顏色
  server.on("/color", []() {
    int t = server.arg("t").toInt();
    int r = server.arg("r").toInt();
    int g = server.arg("g").toInt();
    int b = server.arg("b").toInt();
    CRGB c = CRGB(r, g, b);
    if (t == 0) { colorA = c; colorB = c; }
    else if (t == 1) { colorA = c; }
    else if (t == 2) { colorB = c; }
    server.send(200, "text/plain", "OK");
  });

  server.on("/speed", []() { 
    effectSpeed = server.arg("val").toInt(); 
    server.send(200, "text/plain", "OK"); 
  });
  
  server.on("/bri", []() { FastLED.setBrightness(server.arg("val").toInt()); server.send(200, "text/plain", "OK"); });
  server.on("/off", []() { modeA=0; modeB=0; colorA=CRGB::Black; colorB=CRGB::Black; server.send(200, "text/plain", "OK"); });

  server.begin();
}

void loop() {
  server.handleClient(); 

  uint8_t beat = beat8(effectSpeed);
  uint8_t beatSlow = beat8(effectSpeed / 2); 

  drawSegment(0, HALF_LEDS, modeA, colorA, beat, beatSlow);
  drawSegment(HALF_LEDS, HALF_LEDS, modeB, colorB, beat + 128, beatSlow + 128);

  FastLED.show();
  delay(5);
}