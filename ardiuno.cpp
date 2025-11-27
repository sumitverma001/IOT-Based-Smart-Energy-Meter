#include <WiFi.h>
#include <HTTPClient.h>
#include <PZEM004Tv30.h>
#include <time.h>

// WIFI
const char* ssid = "realme C12";
const char* password = "88888880";

// FIREBASE
const String projectID = "smart-energy-meter-90347";
const String apiKey    = "AIzaSyDxypQr2gKxrQmio8a6d35m3zRkHFEW2Aw";

String livePath     = "meterReadings/METER001/readings/live";
String readingsPath = "meterReadings/METER001/readings";

// TIME SETTINGS
const char* ntpServer = "pool.ntp.org";
long gmtOffset_sec = 5.5 * 3600;
int daylightOffset_sec = 0;

// PZEM board
PZEM004Tv30 pzem(Serial2, 16, 17);

// Convert time to Firestore timestamp format
String getTimestamp() {
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)) return "";  

  char buffer[40];
  strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%S%z", &timeinfo);

  // Firestore expects colon in timezone offset => +0530 → +05:30
  String ts = buffer;
  if(ts.length() > 22) ts = ts.substring(0,22) + ":" + ts.substring(22);

  return ts;
}

// ----------------------
// UPDATE LIVE DOCUMENT
// ----------------------
void updateLive(float v, float c, float p, float e, float pf, float freq) {
  HTTPClient http;
  String t = getTimestamp();

  String url = "https://firestore.googleapis.com/v1/projects/" + projectID +
               "/databases/(default)/documents/" + livePath + "?key=" + apiKey;

  http.begin(url);
  http.addHeader("Content-Type", "application/json");

  String json =
  "{ \"fields\": {"
    "\"voltage\":   {\"doubleValue\": " + String(v) + "},"
    "\"current\":   {\"doubleValue\": " + String(c) + "},"
    "\"power\":     {\"doubleValue\": " + String(p) + "},"
    "\"energy\":    {\"doubleValue\": " + String(e) + "},"
    "\"pf\":        {\"doubleValue\": " + String(pf) + "},"
    "\"frequency\": {\"doubleValue\": " + String(freq) + "},"
    "\"timestamp\": {\"stringValue\": \"" + t + "\"}"
  "}}";

  int code = http.PATCH(json);
  Serial.println("LIVE PATCH: " + String(code));
  http.end();
}

// ----------------------
// ADD HISTORY DOCUMENT
// ----------------------
void sendHistory(float v, float c, float p, float e, float pf, float freq) {

  HTTPClient http;
  String t = getTimestamp();

  String url = "https://firestore.googleapis.com/v1/projects/" + projectID +
               "/databases/(default)/documents/" + readingsPath + "?key=" + apiKey;

  http.begin(url);
  http.addHeader("Content-Type", "application/json");

  String json =
  "{ \"fields\": {"
    "\"voltage\":   {\"doubleValue\": " + String(v) + "},"
    "\"current\":   {\"doubleValue\": " + String(c) + "},"
    "\"power\":     {\"doubleValue\": " + String(p) + "},"
    "\"energy\":    {\"doubleValue\": " + String(e) + "},"
    "\"pf\":        {\"doubleValue\": " + String(pf) + "},"
    "\"frequency\": {\"doubleValue\": " + String(freq) + "},"
    "\"timestamp\": {\"timestampValue\": \"" + t + "\"}"
  "}}";

  int code = http.POST(json);
  Serial.println("HISTORY POST: " + String(code));
  http.end();
}

void setup() {
  Serial.begin(115200);

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while(WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  Serial.println("\nWiFi Connected!");

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
}

void loop() {
  float v = pzem.voltage();
  float c = pzem.current();
  float p = pzem.power();
  float e = pzem.energy();
  float pf   = pzem.pf();
  float freq = pzem.frequency();

  updateLive(v, c, p, e, pf, freq);
  sendHistory(v, c, p, e, pf, freq);

  delay(2500);
}