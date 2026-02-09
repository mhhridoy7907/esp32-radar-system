#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <ESP32Servo.h> 
#define TRIG 4      
#define ECHO 2      
#define SERVO_PIN 17 

DNSServer dnsServer;
WebServer server(80);
Servo radarServo;

int angle = 0;
int dir = 1;

long getDistance() {
  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);

  long duration = pulseIn(ECHO, HIGH, 30000);
  long cm = duration * 0.034 / 2;
  if (cm <= 0 || cm > 200) cm = 200;
  return cm;
}

void handleData() {
  radarServo.write(angle);
  delay(12);

  long dist = getDistance();
  server.send(200, "text/plain", String(angle) + "," + String(dist));

  angle += dir;
  if (angle >= 180 || angle <= 0) dir = -dir;
}

void handlePortal() {
  server.send(200, "text/html", R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width">
<title>MH2 RADAR</title>
<style>
body{
  margin:0;
  background:black;
  color:#00ff00;
  display:flex;
  flex-direction:column;
  align-items:center;
  justify-content:center;
  height:100vh;
  font-family:monospace;
}
h1{  margin:8px; letter-spacing:3px;}
canvas{  box-shadow:0 0 40px #00ff00;}
</style>
</head>
<body> <h1>MH2 RADAR</h1>
<canvas id="radar" width="420" height="420"></canvas>
<script>
const canvas = document.getElementById("radar");
const ctx = canvas.getContext("2d");
const c = 210;
function drawGrid(){
  ctx.strokeStyle="rgba(0,255,0,0.2)";
for(let r=40;r<=200;r+=40){
    ctx.beginPath();
    ctx.arc(c,c,r,Math.PI,2*Math.PI);
    ctx.stroke();
  }
  for(let a=0;a<=180;a+=30){
    let rad=a*Math.PI/180;
    let x=c+200*Math.cos(rad);
    let y=c-200*Math.sin(rad);

ctx.beginPath();
ctx.moveTo(c,c);
ctx.lineTo(x,y);
ctx.stroke();

 ctx.fillStyle="#00ff00";
 ctx.fillText(a+"°", c+210*Math.cos(rad)-10, c-210*Math.sin(rad)+5);
  }
}
function drawSweep(a){
  let rad=a*Math.PI/180;
  ctx.strokeStyle="rgba(0,255,0,0.9)";
  ctx.lineWidth=2;
  ctx.beginPath();
  ctx.moveTo(c,c);
  ctx.lineTo(c+200*Math.cos(rad),c-200*Math.sin(rad));
  ctx.stroke();
}
function drawDot(a,d){
  if(d<180){
    let rad=a*Math.PI/180;
    let x=c+d*Math.cos(rad);
    let y=c-d*Math.sin(rad);
    ctx.fillStyle="red";
    ctx.beginPath();
    ctx.arc(x,y,5,0,Math.PI*2);
    ctx.fill();
  }
}
function render(a,d){
  ctx.fillStyle="rgba(0,0,0,0.25)";
  ctx.fillRect(0,0,420,420);
  drawGrid();
  drawSweep(a);
  drawDot(a,d);
}
setInterval(()=>{
 fetch("/data")
 .then(r=>r.text())
 .then(t=>{
   let p=t.split(",");
   render(+p[0],+p[1]);
 });
},70);
</script>
</body>
</html>
)rawliteral");
}
void setup() {
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);
  radarServo.attach(SERVO_PIN);
WiFi.softAP("MH2_RADAR");
dnsServer.start(53, "*", WiFi.softAPIP());
  server.on("/", handlePortal);
  server.on("/data", handleData);
  server.onNotFound(handlePortal);
  server.begin();
}
void loop() {
  dnsServer.processNextRequest();
  server.handleClient();
}