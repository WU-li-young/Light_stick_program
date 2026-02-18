#include <WiFi.h>
#include <WebServer.h>
#include <FastLED.h>

// ================= 設定區 =================
const char* ap_ssid     = "My_Light_Stick"; 
const char* ap_password = "12345678";
// =========================================

#define LED_PIN     8
#define NUM_LEDS    120      // 總燈數 (30 * 4)
#define HALF_LEDS   60       // 一組的燈數 (30 * 2)
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB

CRGB leds[NUM_LEDS];
WebServer server(80);

// --- 系統變數 ---
uint8_t effectSpeed = 30; // 預設速度

// --- 狀態變數 ---
// 開機預設為 1 (特效模式)，且特效 ID 為 0 (6色輪替)
int modeA = 1; int modeB = 1;
int presetA = 0; int presetB = 0; 

CRGB colorA = CRGB::Red;   
CRGB colorB = CRGB::Blue;
uint8_t briA = 255;
uint8_t briB = 255;

// 6色定義
CRGB myColors[6] = {CRGB::Red, CRGB::Yellow, CRGB::Green, CRGB::Blue, CRGB::Purple, CRGB::White};

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1, user-scalable=no">
  <meta charset="UTF-8">
  <title>光棍 V20 硬體版</title>
  <style>
    body { font-family: 'Verdana', sans-serif; background-color: #000; color: white; text-align: center; margin: 0; padding: 10px; user-select: none; touch-action: none; }
    
    .stick-container { display: flex; flex-direction: column; align-items: center; margin: 10px 0; gap: 4px; }
    /* 這裡稍微把預覽圖拉長，模擬兩截接在一起的感覺 */
    .stick-part { width: 80px; height: 100px; background: #222; border: 2px solid #555; display: flex; align-items: center; justify-content: center; font-size: 16px; color: #fff; font-weight: bold; transition: background 0.1s; }
    #preview-top { border-radius: 15px 15px 0 0; border-bottom: none; }
    #preview-btm { border-radius: 0 0 15px 15px; border-top: none; }

    .panel { background: #151515; padding: 15px; border-radius: 15px; margin-bottom: 15px; border: 1px solid #333; }
    
    .btn-group { display: flex; gap: 5px; margin-bottom: 10px; }
    .btn { flex: 1; padding: 12px; background: #333; border: 2px solid #444; color: #aaa; border-radius: 8px; cursor: pointer; font-weight: bold; font-size: 14px; }
    .btn.active { border-color: #00e5ff; color: #000; background: #00e5ff; box-shadow: 0 0 10px #00e5ff; }
    .btn-mode.active { border-color: #ff9800; color: #000; background: #ff9800; box-shadow: 0 0 10px #ff9800; }

    .hidden { display: none; }

    select {
      width: 100%; padding: 12px; margin-bottom: 15px;
      background-color: #222; color: #fff; border: 2px solid #ff9800;
      border-radius: 8px; font-size: 16px; text-align: center; appearance: none;
    }
    optgroup { background-color: #444; color: #bbb; font-style: italic; }
    option { background-color: #222; color: #fff; font-style: normal; }

    .quick-grid { display: grid; grid-template-columns: 1fr 1fr 1fr 1fr; gap: 8px; margin-bottom: 15px; }
    .q-btn { height: 50px; border-radius: 8px; border: 2px solid #444; cursor: pointer; box-shadow: 0 2px 5px rgba(0,0,0,0.5); }
    .q-btn:active { transform: scale(0.95); border-color: white; }

    .wheel-container { position: relative; width: 280px; height: 280px; margin: 0 auto 15px auto; }
    canvas { width: 100%; height: 100%; cursor: crosshair; border-radius: 50%; box-shadow: 0 0 20px rgba(255,255,255,0.1); }

    .row { display: flex; align-items: center; gap: 10px; margin: 15px 0; justify-content: space-between; }
    input[type=range] { flex-grow: 1; height: 10px; border-radius: 5px; background: #444; outline: none; -webkit-appearance: none; }
    input[type=range]::-webkit-slider-thumb { -webkit-appearance: none; width: 26px; height: 26px; border-radius: 50%; background: #fff; border: 2px solid #000; }
    
    .btn-off { width: 100%; padding: 15px; background: #222; border: 2px solid #ff4444; color: #ff4444; border-radius: 10px; font-size: 18px; font-weight: bold; margin-top: 10px; }
  </style>
</head>
<body>
  
  <div class="stick-container">
    <div id="preview-top" class="stick-part">TOP</div>
    <div id="preview-btm" class="stick-part">BTM</div>
  </div>

  <div class="btn-group">
    <button id="btn-sync" class="btn active" onclick="setTarget(0)">全合 SYNC</button>
    <button id="btn-top" class="btn" onclick="setTarget(1)">上端 TOP</button>
    <button id="btn-btm" class="btn" onclick="setTarget(2)">下端 BTM</button>
  </div>

  <div class="panel">
    <div class="btn-group">
      <button id="mode-static" class="btn btn-mode" onclick="switchMode(0)">🔵 靜態/跳色</button>
      <button id="mode-effect" class="btn btn-mode" onclick="switchMode(1)">🎨 特效庫</button>
    </div>

    <div id="ui-effect" class="hidden">
      <select id="presetSelect" onchange="setPreset(this.value)">
        <optgroup label="--- 標準特效 ---">
          <option value="0">🔄 6色輪替 (Slow)</option>
          <option value="1">🌈 七彩漸變 (Smooth)</option>
          <option value="2">🫁 單色呼吸 (Breathe)</option>
          <option value="3">⚡ 七彩快閃 (Jump)</option>
          <option value="4">🚓 警車閃爍 (Police)</option>
          <option value="5">🔥 火焰 (Fire)</option>
        </optgroup>
        <optgroup label="--- 🌀 旋轉光影 (Abstract POV) ---">
          <option value="6">🌀 虛線光環 (Strobe)</option>
          <option value="7">💈 紅白分段 (Segment)</option>
          <option value="8">🌈 漸層光帶 (Hyper)</option>
          <option value="9">✨ 碎形星塵 (Dot)</option>
        </optgroup>
      </select>
      <div class="row"><span>⚡ 速度/寬度</span><input type="range" min="1" max="255" value="30" oninput="setSpeed(this.value)"></div>
    </div>

    <div id="ui-static" class="hidden">
      <p style="color:#888; font-size:12px; margin:0 0 5px 0;">點擊色塊手動跳色</p>
      <div class="quick-grid">
        <div class="q-btn" style="background:#ff0000" onclick="quickJump('#ff0000')"></div>
        <div class="q-btn" style="background:#ffaa00" onclick="quickJump('#ffaa00')"></div>
        <div class="q-btn" style="background:#ffff00" onclick="quickJump('#ffff00')"></div>
        <div class="q-btn" style="background:#00ff00" onclick="quickJump('#00ff00')"></div>
        <div class="q-btn" style="background:#00ffff" onclick="quickJump('#00ffff')"></div>
        <div class="q-btn" style="background:#0000ff" onclick="quickJump('#0000ff')"></div>
        <div class="q-btn" style="background:#800080" onclick="quickJump('#800080')"></div>
        <div class="q-btn" style="background:#ffffff" onclick="quickJump('#ffffff')"></div>
      </div>
      <div class="wheel-container"><canvas id="colorWheel"></canvas></div>
    </div>

    <div class="row"><span>☀ 亮度</span><input type="range" id="bri-slider" min="0" max="255" value="255" oninput="setBri(this.value)"></div>
  </div>

  <button class="btn-off" onclick="turnOff()">關閉電源 OFF</button>

  <script>
    var target = 0; 
    var state = { 
      modeA: 1, colorA: '#ff0000', presetA: 0, briA: 255,
      modeB: 1, colorB: '#ff0000', presetB: 0, briB: 255
    };
    
    var canvas = document.getElementById('colorWheel');
    var ctx = canvas.getContext('2d');
    var size = 280; canvas.width = size; canvas.height = size;
    var cx = size/2; cy = size/2; radius = size/2;
    var isDragging = false; var lastSend = 0;

    function drawWheel() {
      for(var angle=0; angle<360; angle++){
        var startAngle = (angle-2)*Math.PI/180; var endAngle = (angle+2)*Math.PI/180;
        ctx.beginPath(); ctx.moveTo(cx, cy); ctx.arc(cx, cy, radius, startAngle, endAngle); ctx.closePath();
        ctx.fillStyle = 'hsl('+angle+', 100%, 50%)'; ctx.fill();
      }
      var grd = ctx.createRadialGradient(cx, cy, 0, cx, cy, radius);
      grd.addColorStop(0, 'white'); grd.addColorStop(1, 'transparent');
      ctx.fillStyle = grd; ctx.beginPath(); ctx.arc(cx, cy, radius, 0, 2*Math.PI); ctx.fill();
    }
    
    function pickColor(e) {
      var rect = canvas.getBoundingClientRect();
      var x = (e.touches ? e.touches[0].clientX : e.clientX) - rect.left;
      var y = (e.touches ? e.touches[0].clientY : e.clientY) - rect.top;
      x = x * (canvas.width/rect.width); y = y * (canvas.height/rect.height);
      var pixel = ctx.getImageData(x, y, 1, 1).data; 
      if(pixel[3]<200) return;
      var hex = "#" + ((1<<24) + (pixel[0]<<16) + (pixel[1]<<8) + pixel[2]).toString(16).slice(1);
      var now = Date.now();
      if(now - lastSend > 50) { setColor(hex); lastSend = now; }
    }
    
    drawWheel();
    canvas.addEventListener('mousedown', function(e){ isDragging=true; pickColor(e); });
    canvas.addEventListener('mousemove', function(e){ if(isDragging) pickColor(e); });
    canvas.addEventListener('mouseup', function(){ isDragging=false; });
    canvas.addEventListener('touchstart', function(e){ isDragging=true; pickColor(e); e.preventDefault(); }, {passive:false});
    canvas.addEventListener('touchmove', function(e){ if(isDragging) pickColor(e); e.preventDefault(); }, {passive:false});
    canvas.addEventListener('touchend', function(){ isDragging=false; });

    window.onload = function() { refreshUI(); };

    function setTarget(t) {
      target = t;
      document.querySelectorAll('.btn').forEach(b => { if(!b.classList.contains('btn-mode')) b.classList.remove('active'); });
      if(t===0) document.getElementById('btn-sync').classList.add('active');
      if(t===1) document.getElementById('btn-top').classList.add('active');
      if(t===2) document.getElementById('btn-btm').classList.add('active');
      refreshUI();
    }

    function switchMode(m) {
      if(target===0) { state.modeA=m; state.modeB=m; }
      else if(target===1) state.modeA=m;
      else if(target===2) state.modeB=m;
      refreshUI();
      fetch("/mode?t="+target+"&m="+m);
    }

    function refreshUI() {
      var currentMode = (target===2) ? state.modeB : state.modeA;
      var currentBri = (target===2) ? state.briB : state.briA;
      var currentPreset = (target===2) ? state.presetB : state.presetA;

      document.getElementById('mode-static').className = (currentMode===0) ? 'btn btn-mode active' : 'btn btn-mode';
      document.getElementById('mode-effect').className = (currentMode===1) ? 'btn btn-mode active' : 'btn btn-mode';

      if(currentMode === 0) {
        document.getElementById('ui-static').style.display = 'block';
        document.getElementById('ui-effect').style.display = 'none';
      } else {
        document.getElementById('ui-static').style.display = 'none';
        document.getElementById('ui-effect').style.display = 'block';
        document.getElementById('presetSelect').value = currentPreset;
      }
      document.getElementById('bri-slider').value = currentBri;
      updatePreview();
    }

    function quickJump(hex) { switchMode(0); setColor(hex); }
    function setPreset(val) {
      if(target===0) { state.presetA=val; state.presetB=val; }
      else if(target===1) state.presetA=val;
      else if(target===2) state.presetB=val;
      fetch("/preset?t="+target+"&val="+val);
    }
    function setColor(hex) {
      if(target===0) { state.colorA=hex; state.colorB=hex; }
      else if(target===1) { state.colorA=hex; }
      else if(target===2) { state.colorB=hex; }
      updatePreview();
      var r = parseInt(hex.slice(1,3),16); var g = parseInt(hex.slice(3,5),16); var b = parseInt(hex.slice(5,7),16);
      fetch("/color?t="+target+"&r="+r+"&g="+g+"&b="+b);
    }
    function setSpeed(val) { fetch("/speed?val="+val); }
    function setBri(val) { 
      if(target===0) { state.briA=val; state.briB=val; }
      else if(target===1) state.briA=val;
      else if(target===2) state.briB=val;
      fetch("/bri?t="+target+"&val="+val); 
    }
    function turnOff() { 
      fetch("/off"); 
      state.modeA=0; state.modeB=0; state.colorA='#000000'; state.colorB='#000000';
      refreshUI();
    }
    function updatePreview() {
      var top = document.getElementById('preview-top');
      var btm = document.getElementById('preview-btm');
      top.style.background = (state.modeA===0) ? state.colorA : '#333';
      btm.style.background = (state.modeB===0) ? state.colorB : '#333';
      if(state.modeA===1) top.innerText = "FX " + state.presetA; else top.innerText = "TOP";
      if(state.modeB===1) btm.innerText = "FX " + state.presetB; else btm.innerText = "BTM";
      top.style.opacity = state.briA / 255;
      btm.style.opacity = state.briB / 255;
    }
  </script>
</body>
</html>
)rawliteral";

void runEffect(int start, int len, int presetID, CRGB baseColor) {
  uint8_t beat = beat8(effectSpeed); 

  switch(presetID) {
    // --- 標準特效 ---
    case 0: // 6色輪替 (慢)
      {
        unsigned long interval = 3000 - (effectSpeed * 10); 
        if(interval < 50) interval = 50;
        int idx = (millis() / interval) % 6;
        fill_solid(leds + start, len, myColors[idx]);
      }
      break;
    case 1: // 七彩漸變
      fill_rainbow(leds + start, len, beat, 5); 
      break;
    case 2: // 單色呼吸
      {
        uint8_t b = cubicwave8(beat8(effectSpeed)); 
        fill_solid(leds + start, len, baseColor);
        for(int i=0; i<len; i++) leds[start+i].nscale8(b);
      }
      break;
    case 3: // 七彩快閃
      fill_solid(leds + start, len, CHSV((beat/42)*42, 255, 255));
      break;
    case 4: // 警車
      {
        int blink = (millis() / (3000 / (effectSpeed+1))) % 2;
        fill_solid(leds + start, len, blink ? CRGB::Red : CRGB::Blue);
      }
      break;
    case 5: // 火焰
      for(int i = 0; i < len; i++) {
        leds[start + i] = CHSV(0 + random8(40), 255, random8(150, 255));
      }
      break;

    // --- 🌀 POV 旋轉光影 ---
    case 6: // 虛線光環
      if ( (millis() % (105 - effectSpeed/3)) < 15 ) fill_solid(leds + start, len, CRGB::White);
      else fill_solid(leds + start, len, CRGB::Black);
      break;
    case 7: // 紅白分段
      if ( (millis() / (105 - effectSpeed/3)) % 2 == 0 ) fill_solid(leds + start, len, CRGB::Red);
      else fill_solid(leds + start, len, CRGB::White);
      break;
    case 8: // 漸層光帶
      fill_rainbow(leds + start, len, millis(), 10);
      break;
    case 9: // 碎形星塵
      fill_solid(leds + start, len, CRGB::Black);
      if(random8() < 50) leds[start + random16(len)] = CHSV(random8(), 255, 255);
      break;
  }
}

void setup() {
  Serial.begin(115200);
  delay(100);
  
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(255); 
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();

  WiFi.mode(WIFI_AP);
  WiFi.setTxPower(WIFI_POWER_11dBm);
  WiFi.softAP(ap_ssid, ap_password);

  server.on("/", []() { server.send(200, "text/html", index_html); });
  
  server.on("/mode", []() {
    int t = server.arg("t").toInt(); int m = server.arg("m").toInt();
    if (t == 0) { modeA = m; modeB = m; } else if (t == 1) { modeA = m; } else if (t == 2) { modeB = m; }
    server.send(200, "text/plain", "OK");
  });

  server.on("/preset", []() {
    int t = server.arg("t").toInt(); int val = server.arg("val").toInt();
    if (t == 0) { presetA=val; presetB=val; modeA=1; modeB=1; }
    else if (t == 1) { presetA=val; modeA=1; }
    else if (t == 2) { presetB=val; modeB=1; }
    server.send(200, "text/plain", "OK");
  });

  server.on("/color", []() {
    int t = server.arg("t").toInt(); 
    int r = server.arg("r").toInt(); int g = server.arg("g").toInt(); int b = server.arg("b").toInt();
    CRGB c = CRGB(r, g, b);
    if (t == 0) { colorA=c; colorB=c; modeA=0; modeB=0; }
    else if (t == 1) { colorA=c; modeA=0; }
    else if (t == 2) { colorB=c; modeB=0; }
    server.send(200, "text/plain", "OK");
  });

  server.on("/speed", []() { effectSpeed = server.arg("val").toInt(); server.send(200, "text/plain", "OK"); });
  
  server.on("/bri", []() { 
    int t = server.arg("t").toInt(); int val = server.arg("val").toInt();
    if(t==0) { briA=val; briB=val; }
    else if(t==1) briA=val;
    else if(t==2) briB=val;
    server.send(200, "text/plain", "OK"); 
  });

  server.on("/off", []() { modeA=0; modeB=0; colorA=CRGB::Black; colorB=CRGB::Black; server.send(200, "text/plain", "OK"); });

  server.begin();
}

void loop() {
  server.handleClient(); 
  
  // 繪製 Group A (上端)
  if(modeA == 0) fill_solid(leds, HALF_LEDS, colorA);
  else runEffect(0, HALF_LEDS, presetA, colorA);

  // 繪製 Group B (下端)
  if(modeB == 0) fill_solid(leds + HALF_LEDS, HALF_LEDS, colorB);
  else runEffect(HALF_LEDS, HALF_LEDS, presetB, colorB);

  // 獨立亮度運算
  for(int i=0; i<HALF_LEDS; i++) leds[i].nscale8(briA);
  for(int i=HALF_LEDS; i<NUM_LEDS; i++) leds[i].nscale8(briB);

  FastLED.show();
  delay(1); 
}