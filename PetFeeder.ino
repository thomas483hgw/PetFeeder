#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>

const char* ssid = "PetFeeder_AP";
const char* password = "12345678";

// Sensor Pins
#define TRIGGER_PIN_ANIMAL 5
#define ECHO_PIN_ANIMAL 18
#define TRIGGER_PIN_FOOD 19
#define ECHO_PIN_FOOD 21

// Servo Pin
#define SERVO_PIN 13

WebServer server(80);
Servo feederServo;

// Einstellbare Parameter
float minAnimalDistance = 10.0;
float maxAnimalDistance = 50.0;
float minFoodDistance = 5.0;  // Mindestabstand für "Futter vorhanden"
int openingAngle = 90;
int openingDuration = 3000;
int feedingCooldown = 3600000; // 1 Stunde
int maxFeedingsPerDay = 4;

// Zustandsvariablen
unsigned long lastFeedingTime = 0;
int feedingsToday = 0;
unsigned long lastDayCheck = 0;

void setupServo() {
  ESP32PWM::allocateTimer(0);
  feederServo.setPeriodHertz(50); // Standard-Servo-Frequenz
  feederServo.attach(SERVO_PIN, 500, 2400); // SG90 spezifische Pulse
  feederServo.write(90); // Startposition
}

float measureDistance(int triggerPin, int echoPin) {
  digitalWrite(triggerPin, LOW);
  delayMicroseconds(2);
  digitalWrite(triggerPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(triggerPin, LOW);
  
  long duration = pulseIn(echoPin, HIGH, 30000);
  float distance = duration * 0.034 / 2;
  return (distance > 400 || distance < 2) ? -1 : distance;
}

void controlServo() {
  feederServo.write(openingAngle);
  delay(openingDuration);
  feederServo.write(0);
  Serial.println("Servo bewegt - Futter ausgegeben");
}

void checkFeedingConditions() {
  // Tageswechsel prüfen
  if (millis() - lastDayCheck >= 86400000) {
    feedingsToday = 0;
    lastDayCheck = millis();
  }
  
  float animalDist = measureDistance(TRIGGER_PIN_ANIMAL, ECHO_PIN_ANIMAL);
  float foodDist = measureDistance(TRIGGER_PIN_FOOD, ECHO_PIN_FOOD);
  
  if (animalDist >= minAnimalDistance && animalDist <= maxAnimalDistance &&
      foodDist >= minFoodDistance && foodDist != -1 &&
      millis() - lastFeedingTime >= feedingCooldown &&
      feedingsToday < maxFeedingsPerDay) {
    
    controlServo();
    lastFeedingTime = millis();
    feedingsToday++;
    Serial.println("Automatische Fütterung durchgeführt");
  }
}

String getStatusColor(int current, int max) {
  if (current >= max) return "red";
  if (current >= max * 0.75) return "orange";
  return "green";
}

void handleRoot() {
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>Pet Feeder Steuerung</title>";
  html += "<style>";
  html += "body {font-family: Arial, sans-serif; max-width: 800px; margin: 0 auto; padding: 20px; background-color: #f5f5f5;}";
  html += "h1 {color: #333; text-align: center;}";
  html += ".sensor-container {display: flex; justify-content: space-between; margin-bottom: 20px;}";
  html += ".sensor-box {background: white; border-radius: 8px; padding: 15px; width: 48%; box-shadow: 0 2px 5px rgba(0,0,0,0.1);}";
  html += ".sensor-value {font-size: 24px; font-weight: bold; margin: 10px 0;}";
  html += ".status-box {background: white; border-radius: 8px; padding: 15px; margin-bottom: 20px; box-shadow: 0 2px 5px rgba(0,0,0,0.1);}";
  html += ".settings-form {background: white; border-radius: 8px; padding: 15px; box-shadow: 0 2px 5px rgba(0,0,0,0.1);}";
  html += "input, button {width: 100%; padding: 10px; margin: 5px 0; border-radius: 4px; border: 1px solid #ddd;}";
  html += "button {background-color: #4CAF50; color: white; border: none; cursor: pointer; font-weight: bold;}";
  html += "button:hover {background-color: #45a049;}";
  html += ".manual-feed {text-align: center; margin-top: 20px;}";
  html += "</style></head><body>";
  
  html += "<h1>🐾 Automatischer Futterautomat 🐾</h1>";
  
  // Sensorwerte
  html += "<div class='sensor-container'>";
  html += "<div class='sensor-box'><h2>🗨 Tierabstand</h2><div class='sensor-value' id='animalDistance'>--</div><div>cm</div></div>";
  html += "<div class='sensor-box'><h2>🍗 Futterstand</h2><div class='sensor-value' id='foodDistance'>--</div><div>cm</div></div>";
  html += "</div>";
  
  // Status
  html += "<div class='status-box'>";
  html += "<h2>⚙ Systemstatus</h2>";
  html += "<p>Letzte Fütterung: <span id='lastFeed'>";
  if (lastFeedingTime > 0) {
    unsigned long seconds = (millis() - lastFeedingTime) / 1000;
    html += String(seconds) + " Sekunden her";
  } else {
    html += "Noch nicht erfolgt";
  }
  html += "</span></p>";
  
  html += "<p>Fütterungen heute: <span id='feedCount' style='color:";
  html += getStatusColor(feedingsToday, maxFeedingsPerDay);
  html += ";font-weight:bold;'>" + String(feedingsToday) + "</span>/" + String(maxFeedingsPerDay) + "</p>";
  html += "</div>";
  
  // Einstellungen
  html += "<div class='settings-form'>";
  html += "<h2>🔧 Einstellungen</h2>";
  html += "<form action='/updateSettings' method='POST'>";
  html += "<label for='minDist'>Min. Tierabstand (cm):</label>";
  html += "<input type='number' step='0.1' name='minDist' value='" + String(minAnimalDistance, 1) + "' required>";
  
  html += "<label for='maxDist'>Max. Tierabstand (cm):</label>";
  html += "<input type='number' step='0.1' name='maxDist' value='" + String(maxAnimalDistance, 1) + "' required>";
  
  html += "<label for='foodDist'>Min. Futterabstand (cm):</label>";
  html += "<input type='number' step='0.1' name='foodDist' value='" + String(minFoodDistance, 1) + "' required>";
  
  html += "<label for='angle'>Öffnungswinkel (0-180°):</label>";
  html += "<input type='number' name='angle' min='0' max='180' value='" + String(openingAngle) + "' required>";
  
  html += "<label for='duration'>Öffnungsdauer (ms):</label>";
  html += "<input type='number' name='duration' min='500' value='" + String(openingDuration) + "' required>";
  
  html += "<label for='cooldown'>Wartezeit zwischen Fütterungen (ms):</label>";
  html += "<input type='number' name='cooldown' min='60000' value='" + String(feedingCooldown) + "' required>";
  
  html += "<label for='maxFeed'>Max. Fütterungen pro Tag:</label>";
  html += "<input type='number' name='maxFeed' min='1' value='" + String(maxFeedingsPerDay) + "' required>";
  
  html += "<button type='submit'>Einstellungen speichern</button>";
  html += "</form>";
  html += "</div>";
  
  // Manuelle Steuerung
  html += "<div class='manual-feed'>";
  html += "<button onclick=\"feedNow()\">JETZT FÜTTERN</button>";
  html += "</div>";
  
  // JavaScript
  html += "<script>";
  html += "function updateSensors() {";
  html += "fetch('/getAnimalDistance').then(r=>r.text()).then(d=>document.getElementById('animalDistance').innerText=d);";
  html += "fetch('/getFoodDistance').then(r=>r.text()).then(d=>document.getElementById('foodDistance').innerText=d);";
  html += "fetch('/getFeedStatus').then(r=>r.json()).then(d=>{";
  html += "document.getElementById('lastFeed').innerText=d.lastFeed>0?Math.round((Date.now()-d.lastFeed)/1000)+' Sekunden her':'Noch nicht erfolgt';";
  html += "let feedCount=document.getElementById('feedCount');";
  html += "feedCount.innerText=d.feedCount+'/'+d.maxFeed;";
  html += "feedCount.style.color=d.feedCount>=d.maxFeed?'red':d.feedCount>=d.maxFeed*0.75?'orange':'green';";
  html += "});";
  html += "}";
  
  html += "function feedNow() {";
  html += "fetch('/feedNow').then(r=>r.text()).then(alert);";
  html += "setTimeout(updateSensors, 1000);";
  html += "}";
  
  html += "setInterval(updateSensors, 1000);";
  html += "updateSensors();";
  html += "</script>";
  
  html += "</body></html>";
  
  server.send(200, "text/html; charset=utf-8", html);
}

void handleUpdateSettings() {
  if (server.hasArg("minDist")) minAnimalDistance = server.arg("minDist").toFloat();
  if (server.hasArg("maxDist")) maxAnimalDistance = server.arg("maxDist").toFloat();
  if (server.hasArg("foodDist")) minFoodDistance = server.arg("foodDist").toFloat();
  if (server.hasArg("angle")) openingAngle = server.arg("angle").toInt();
  if (server.hasArg("duration")) openingDuration = server.arg("duration").toInt();
  if (server.hasArg("cooldown")) feedingCooldown = server.arg("cooldown").toInt();
  if (server.hasArg("maxFeed")) maxFeedingsPerDay = server.arg("maxFeed").toInt();
  
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleFeedNow() {
  if (feedingsToday < maxFeedingsPerDay) {
    controlServo();
    lastFeedingTime = millis();
    feedingsToday++;
    server.send(200, "text/plain", "Manuelle Fütterung durchgeführt");
  } else {
    server.send(200, "text/plain", "Maximale Fütterungen für heute erreicht!");
  }
}

void handleGetAnimalDistance() {
  server.send(200, "text/plain", String(measureDistance(TRIGGER_PIN_ANIMAL, ECHO_PIN_ANIMAL), 1));
}

void handleGetFoodDistance() {
  server.send(200, "text/plain", String(measureDistance(TRIGGER_PIN_FOOD, ECHO_PIN_FOOD), 1));
}

void handleGetFeedStatus() {
  String json = "{";
  json += "\"lastFeed\":" + String(lastFeedingTime) + ",";
  json += "\"feedCount\":" + String(feedingsToday) + ",";
  json += "\"maxFeed\":" + String(maxFeedingsPerDay);
  json += "}";
  server.send(200, "application/json", json);
}

void setup() {
  Serial.begin(115200);
  
  // GPIO initialisieren
  pinMode(TRIGGER_PIN_ANIMAL, OUTPUT);
  pinMode(ECHO_PIN_ANIMAL, INPUT);
  pinMode(TRIGGER_PIN_FOOD, OUTPUT);
  pinMode(ECHO_PIN_FOOD, INPUT);
  
  // Servo initialisieren
  setupServo();
  
  // WiFi Access Point
  WiFi.softAP(ssid, password);
  Serial.println("Access Point gestartet");
  Serial.print("IP-Adresse: ");
  Serial.println(WiFi.softAPIP());

  // Server Routen
  server.on("/", handleRoot);
  server.on("/updateSettings", HTTP_POST, handleUpdateSettings);
  server.on("/feedNow", handleFeedNow);
  server.on("/getAnimalDistance", handleGetAnimalDistance);
  server.on("/getFoodDistance", handleGetFoodDistance);
  server.on("/getFeedStatus", handleGetFeedStatus);
  
  server.begin();
  Serial.println("HTTP Server gestartet");
}

void loop() {
  server.handleClient();
  checkFeedingConditions();
  delay(100);
}