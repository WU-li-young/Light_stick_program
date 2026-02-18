#include <WiFi.h>
#include <WebServer.h>
#include <FastLED.h>

// ================= 設定區 =================
const char* ap_ssid     = "My_Light_Stick"; 
const char* ap_password = "88888888";

// [單腳位接線] 請將燈條訊號接到 GPIO 4
#define LED_PIN     4        
#define NUM_LEDS    58      // 總燈數 (依據您的修改)
#define HALF_LEDS   29      // 單邊燈數 (依據您的修改)
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB
// =========================================

CRGB leds[NUM_LEDS];
WebServer server(80);

// --- 系統變數 ---
uint8_t effectSpeed = 30; 

// --- 狀態變數 ---
int modeA = 1; int modeB = 1;
int presetA = 0; int presetB = 0; 

CRGB colorA = CRGB::Red;   
CRGB colorB = CRGB::Blue;
uint8_t briA = 255;
uint8_t briB = 255;

CRGB myColors[6] = {CRGB::Red, CRGB::Yellow, CRGB::Green, CRGB::Blue, CRGB::Purple, CRGB::White};

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1, user-scalable=no">
  <meta charset="UTF-8">
  <title>光棍控制台</title>
  <style>
    /* [修改點 1] body 移除了 touch-action: none，現在網頁可以捲動了 */
    body { font-family: 'Verdana', sans-serif; background-color: #000; color: white; text-align: center; margin: 0; padding: 10px; user-select: none; }
    
    .stick-container { display: flex; flex-direction: column; align-items: center; margin: 10px 0; gap: 4px; }
    .stick-part { width: 80px; height: 100px; background: #222; border: 2px solid #555; display: flex; align-items: center; justify-content: center; font-size: 16px; color: #fff; font-weight: bold; transition: background 0.1s; }
    #preview-top { border-radius: 15px 15px 0 0; border-bottom: none; }
    #preview-btm { border-radius: 0 0 15px 15px; border-top: none; }
    .panel { background: #151515; padding: 15px; border-radius: 15px; margin-bottom: 15px; border: 1px solid #333; }
    .btn-group { display: flex; gap: 5px; margin-bottom: 10px; }
    .btn { flex: 1; padding: 12px; background: #333; border: 2px solid #444; color: #aaa; border-radius: 8px; cursor: pointer; font-weight: bold; font-size: 14px; }
    .btn.active { border-color: #00e5ff; color: #000; background: #00e5ff; box-shadow: 0 0 10px #00e5ff; }
    .btn-mode.active { border-color: #ff9800; color: #000; background: #ff9800; box-shadow: 0 0 10px #ff9800; }
    .hidden { display: none; }
    select { width: 100%; padding: 12px; margin-bottom: 15px; background-color: #222; color: #fff; border: 2px solid #ff9800; border-radius: 8px; font-size: 16px; text-align: center; appearance: none; }
    optgroup { background-color: #444; color: #bbb; font-style: italic; }
    option { background-color: #222; color: #fff; font-style: normal; }
    .quick-grid { display: grid; grid-template-columns: 1fr 1fr 1fr 1fr; gap: 8px; margin-bottom: 15px; }
    .q-btn { height: 50px; border-radius: 8px; border: 2px solid #444; cursor: pointer; box-shadow: 0 2px 5px rgba(0,0,0,0.5); }
    .q-btn:active { transform: scale(0.95); border-color: white; }
    .wheel-container { position: relative; width: 280px; height: 280px; margin: 0 auto 15px auto; }
    
    /* [修改點 2] 只有調色盤禁止捲動，避免選色時畫面亂跑 */
    canvas { width: 100%; height: 100%; cursor: crosshair; border-radius: 50%; box-shadow: 0 0 20px rgba(255,255,255,0.1); touch-action: none; }
    
    .row { display: flex; align-items: center; gap: 10px; margin: 15px 0; justify-content: space-between; }
    input[type=range] { flex-grow: 1; height: 10px; border-radius: 5px; background: #444; outline: none; -webkit-appearance: none; }
    input[type=range]::-webkit-slider-thumb { -webkit-appearance: none; width: 26px; height: 26px; border-radius: 50%; background: #fff; border: 2px solid #000; }
    .btn-off { width: 100%; padding: 15px; background: #222; border: 2px solid #ff4444; color: #ff4444; border-radius: 10px; font-size: 18px; font-weight: bold; margin-top: 10px; }
  </style>
</head>
<body>
  <div class="stick-container"><div id="preview-top" class="stick-part">TOP</div><div id="preview-btm" class="stick-part">BTM</div></div>
  <div class="btn-group"><button id="btn-sync" class="btn active" onclick="setTarget(0)">全合 SYNC</button><button id="btn-top" class="btn" onclick="setTarget(1)">上端 TOP</button><button id="btn-btm" class="btn" onclick="setTarget(2)">下端 BTM</button></div>
  <div class="panel">
    <div class="btn-group"><button id="mode-static" class="btn btn-mode" onclick="switchMode(0)">🔵 靜態/跳色</button><button id="mode-effect" class="btn btn-mode" onclick="switchMode(1)">🎨 特效庫</button></div>
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
        <optgroup label="--- 🌀 旋轉特效 (POV) ---">
          <option value="6">🌀 虛線光環 (Strobe)</option>
          <option value="7">💈 紅白分段 (Segment)</option>
          <option value="8">🌈 漸層光帶 (Hyper)</option>
          <option value="9">✨ 碎形星塵 (Dot)</option>
        </optgroup>
      </select>
      <div class="row"><span>⚡ 速度/寬度</span><input type="range" min="1" max="255" value="30" oninput="setSpeed(this.value)"></div>
    </div>
    <div id="ui-static" class="hidden">
      <div class="quick-grid">
        <div class="q-btn" style="background:#ff0000" onclick="quickJump('#ff0000')"></div><div class="q-btn" style="background:#ffaa00" onclick="quickJump('#ffaa00')"></div><div class="q-btn" style="background:#ffff00" onclick="quickJump('#ffff00')"></div><div class="q-btn" style="background:#00ff00" onclick="quickJump('#00ff00')"></div><div class="q-btn" style="background:#00ffff" onclick="quickJump('#00ffff')"></div><div class="q-btn" style="background:#0000ff" onclick="quickJump('#0000ff')"></div><div class="q-btn" style="background:#800080" onclick="quickJump('#800080')"></div><div class="q-btn" style="background:#ffffff" onclick="quickJump('#ffffff')"></div>
      </div>
      <div class="wheel-container"><canvas id="colorWheel"></canvas></div>
    </div>
    <div class="row"><span>☀ 亮度</span><input type="range" id="bri-slider" min="0" max="255" value="255" oninput="setBri(this.value)"></div>
  </div>
  <button class="btn-off" onclick="turnOff()">關閉電源 OFF</button>
  <script>
    var t=0; var s={mA:1,cA:'#ff0000',pA:0,bA:255,mB:1,cB:'#0000ff',pB:0,bB:255};
    var cv=document.getElementById('colorWheel'), ctx=cv.getContext('2d'), sz=280, cx=140, cy=140, drag=false, ls=0;
    cv.width=sz; cv.height=sz;
    function dW(){for(var a=0;a<360;a++){var sa=(a-2)*Math.PI/180,ea=(a+2)*Math.PI/180;ctx.beginPath();ctx.moveTo(cx,cy);ctx.arc(cx,cy,140,sa,ea);ctx.closePath();ctx.fillStyle='hsl('+a+',100%,50%)';ctx.fill();}var g=ctx.createRadialGradient(cx,cy,0,cx,cy,140);g.addColorStop(0,'white');g.addColorStop(1,'transparent');ctx.fillStyle=g;ctx.beginPath();ctx.arc(cx,cy,140,0,2*Math.PI);ctx.fill();}
    function pC(e){var r=cv.getBoundingClientRect(),x=(e.touches?e.touches[0].clientX:e.clientX)-r.left,y=(e.touches?e.touches[0].clientY:e.clientY)-r.top;x*=cv.width/r.width;y*=cv.height/r.height;var p=ctx.getImageData(x,y,1,1).data;if(p[3]<200)return;var h="#"+((1<<24)+(p[0]<<16)+(p[1]<<8)+p[2]).toString(16).slice(1);var n=Date.now();if(n-ls>50){setColor(h);ls=n;}}
    dW(); cv.addEventListener('mousedown',function(e){drag=true;pC(e);}); cv.addEventListener('mousemove',function(e){if(drag)pC(e);}); cv.addEventListener('mouseup',function(){drag=false;}); cv.addEventListener('touchstart',function(e){drag=true;pC(e);e.preventDefault();},{passive:false}); cv.addEventListener('touchmove',function(e){if(drag)pC(e);e.preventDefault();},{passive:false});
    window.onload=rUI;
    function setTarget(v){t=v;document.querySelectorAll('.btn').forEach(b=>{if(!b.classList.contains('btn-mode'))b.classList.remove('active')});if(t==0)document.getElementById('btn-sync').classList.add('active');if(t==1)document.getElementById('btn-top').classList.add('active');if(t==2)document.getElementById('btn-btm').classList.add('active');rUI();}
    function switchMode(m){if(t==0){s.mA=m;s.mB=m;}else if(t==1)s.mA=m;else if(t==2)s.mB=m;rUI();fetch("/mode?t="+t+"&m="+m);}
    function rUI(){var cM=(t==2)?s.mB:s.mA, cB=(t==2)?s.bB:s.bA, cP=(t==2)?s.pB:s.pA;
      document.getElementById('mode-static').className=(cM==0)?'btn btn-mode active':'btn btn-mode';
      document.getElementById('mode-effect').className=(cM==1)?'btn btn-mode active':'btn btn-mode';
      document.getElementById('ui-static').style.display=(cM==0)?'block':'none';
      document.getElementById('ui-effect').style.display=(cM==1)?'block':'none';
      document.getElementById('presetSelect').value=cP; document.getElementById('bri-slider').value=cB; uP();}
    function quickJump(h){switchMode(0);setColor(h);}
    function setPreset(v){if(t==0){s.pA=v;s.pB=v;}else if(t==1)s.pA=v;else if(t==2)s.pB=v;fetch("/preset?t="+t+"&val="+v);}
    function setColor(h){if(t==0){s.cA=h;s.cB=h;}else if(t==1)s.cA=h;else if(t==2)s.cB=h;uP();var r=parseInt(h.slice(1,3),16),g=parseInt(h.slice(3,5),16),b=parseInt(h.slice(5,7),16);fetch("/color?t="+t+"&r="+r+"&g="+g+"&b="+b);}
    function setSpeed(v){fetch("/speed?val="+v);}
    function setBri(v){if(t==0){s.bA=v;s.bB=v;}else if(t==1)s.bA=v;else if(t==2)s.bB=v;fetch("/bri?t="+t+"&val="+v);}
    function turnOff(){fetch("/off");s.mA=0;s.mB=0;s.cA='#000000';s.cB='#000000';rUI();}
    function uP(){var top=document.getElementById('preview-top'),btm=document.getElementById('preview-btm');top.style.background=(s.mA==0)?s.cA:'#333';btm.style.background=(s.mB==0)?s.cB:'#333';top.innerText=(s.mA==1)?"FX":"TOP";btm.innerText=(s.mB==1)?"FX":"BTM";top.style.opacity=s.bA/255;btm.style.opacity=s.bB/255;}
  </script>
</body>
</html>
)rawliteral";

void runEffect(int start, int len, int presetID, CRGB baseColor) {
  uint8_t beat = beat8(effectSpeed); 
  switch(presetID) {
    // 標準特效
    case 0: { unsigned long interval = 3000 - (effectSpeed * 10); if(interval < 50) interval = 50; int idx = (millis() / interval) % 6; fill_solid(leds + start, len, myColors[idx]); } break;
    case 1: fill_rainbow(leds + start, len, beat, 5); break;
    case 2: { uint8_t b = cubicwave8(beat8(effectSpeed)); fill_solid(leds + start, len, baseColor); for(int i=0; i<len; i++) leds[start+i].nscale8(b); } break;
    case 3: fill_solid(leds + start, len, CHSV((beat/42)*42, 255, 255)); break;
    case 4: { int blink = (millis() / (3000 / (effectSpeed+1))) % 2; fill_solid(leds + start, len, blink ? CRGB::Red : CRGB::Blue); } break;
    case 5: for(int i = 0; i < len; i++) leds[start + i] = CHSV(0 + random8(40), 255, random8(150, 255)); break;
    
    // POV 旋轉特效 (加回來了!)
    case 6: if ( (millis() % (105 - effectSpeed/3)) < 15 ) fill_solid(leds + start, len, CRGB::White); else fill_solid(leds + start, len, CRGB::Black); break;
    case 7: if ( (millis() / (105 - effectSpeed/3)) % 2 == 0 ) fill_solid(leds + start, len, CRGB::Red); else fill_solid(leds + start, len, CRGB::White); break;
    case 8: fill_rainbow(leds + start, len, millis(), 10); break;
    case 9: fill_solid(leds + start, len, CRGB::Black); if(random8() < 50) leds[start + random16(len)] = CHSV(random8(), 255, 255); break;
  }
}

void setup() {
  Serial.begin(115200);
  delay(100); 

  // [單腳位初始化] 接 GPIO 4，控制 58 顆燈
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(255);
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();

  WiFi.mode(WIFI_AP);
  WiFi.setTxPower(WIFI_POWER_11dBm);
  WiFi.softAP(ap_ssid, ap_password);
  
  server.on("/", []() { server.send(200, "text/html", index_html); });
  server.on("/mode", []() { int t=server.arg("t").toInt(); int m=server.arg("m").toInt(); if(t==0){modeA=m;modeB=m;}else if(t==1)modeA=m;else if(t==2)modeB=m; server.send(200,"text/plain","OK"); });
  server.on("/preset", []() { int t=server.arg("t").toInt(); int v=server.arg("val").toInt(); if(t==0){presetA=v;presetB=v;modeA=1;modeB=1;}else if(t==1){presetA=v;modeA=1;}else if(t==2){presetB=v;modeB=1;} server.send(200,"text/plain","OK"); });
  server.on("/color", []() { int t=server.arg("t").toInt(); int r=server.arg("r").toInt(); int g=server.arg("g").toInt(); int b=server.arg("b").toInt(); CRGB c=CRGB(r,g,b); if(t==0){colorA=c;colorB=c;modeA=0;modeB=0;}else if(t==1){colorA=c;modeA=0;}else if(t==2){colorB=c;modeB=0;} server.send(200,"text/plain","OK"); });
  server.on("/speed", []() { effectSpeed=server.arg("val").toInt(); server.send(200,"text/plain","OK"); });
  server.on("/bri", []() { int t=server.arg("t").toInt(); int v=server.arg("val").toInt(); if(t==0){briA=v;briB=v;}else if(t==1)briA=v;else if(t==2)briB=v; server.send(200,"text/plain","OK"); });
  server.on("/off", []() { modeA=0;modeB=0;colorA=CRGB::Black;colorB=CRGB::Black; server.send(200,"text/plain","OK"); });

  server.begin();
}

void loop() {
  server.handleClient(); 
  
  if(modeA == 0) fill_solid(leds, HALF_LEDS, colorA); else runEffect(0, HALF_LEDS, presetA, colorA);
  if(modeB == 0) fill_solid(leds + HALF_LEDS, HALF_LEDS, colorB); else runEffect(HALF_LEDS, HALF_LEDS, presetB, colorB);

  for(int i=0; i<HALF_LEDS; i++) leds[i].nscale8(briA);
  for(int i=HALF_LEDS; i<NUM_LEDS; i++) leds[i].nscale8(briB);

  FastLED.show();
  delay(1); 
}