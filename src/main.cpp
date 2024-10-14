#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <DHT.h>
#include <LittleFS.h>
#include <Wire.h>
#include <ESPmDNS.h>  // mDNS-Bibliothek einbinden
#include <esp_now.h>  // ESP-Now-Bibliothek

#define DHTPIN 4          
#define DHTTYPE DHT11     

DHT dht(DHTPIN, DHTTYPE);

// WLAN-Konfiguration (Station Mode)
const char* ssid = "NTGR_05C1"; 
const char* password = "E665u3A6";

// WLAN-Konfiguration (Access Point)
const char* ap_ssid = "ESP32_AccessPoint";
const char* ap_password = "12345678";

AsyncWebServer server(80);
bool isAPMode = false;  // Flag, um zu verfolgen, ob der AP-Modus aktiv ist
unsigned long previousMillis = 0;  // Timer für die Überprüfung
const long interval = 10000;  // 10 Sekunden-Intervall für WLAN-Überprüfung

float externalTemperature = 0.0;  // Variable für Außentemperatur von ESP-Now
float externalHumidity = 0.0;     // Variable für Außenluftfeuchtigkeit von ESP-Now

struct struct_message {
  float temperature;
  float humidity;
};

// Funktion zum Abrufen von Uhrzeit und Datum
String getFormattedTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return "Fehler: Keine Zeit";
  }
  char buffer[80];
  strftime(buffer, sizeof(buffer), "%H:%M:%S - %d.%m.%Y", &timeinfo);
  return String(buffer);
}

// Funktion zum Abrufen der DHT-Daten (interne Daten)
String getDHTData() {
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  if (isnan(temperature) || isnan(humidity)) {
    return "{\"error\": \"Sensor nicht verfügbar\"}";
  }

  String data = "{";
  data += "\"internal_temperature\": " + String(temperature) + ",";
  data += "\"internal_humidity\": " + String(humidity) + ",";
  data += "\"external_temperature\": " + String(externalTemperature) + ",";
  data += "\"external_humidity\": " + String(externalHumidity);  // Außentemperatur und -feuchtigkeit vom ESP-Now
  data += "}";
  
  return data;
}

// Funktion zur Verbindung mit dem WLAN (Station Mode)
void connectToWiFi() {
  Serial.println("Versuche, mit WLAN zu verbinden...");
  WiFi.begin(ssid, password);

  int attemptCount = 0;
  while (WiFi.status() != WL_CONNECTED && attemptCount < 10) {
    delay(1000);
    Serial.print(".");
    attemptCount++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nMit WLAN verbunden!");
    Serial.println("IP-Adresse: " + WiFi.localIP().toString());
    if (isAPMode) {
      // Schalte den Access Point aus, wenn er noch läuft
      WiFi.softAPdisconnect(true);
      isAPMode = false;
      Serial.println("Access Point deaktiviert.");
    }
  } else {
    // Starte Access Point-Modus, wenn WLAN nicht verfügbar ist
    Serial.println("\nVerbindung zum WLAN fehlgeschlagen, Access Point wird gestartet...");
    WiFi.softAP(ap_ssid, ap_password);
    Serial.println("Access Point gestartet!");
    Serial.println("AP IP-Adresse: " + WiFi.softAPIP().toString());
    isAPMode = true;
  }
}

// Funktion zur Überprüfung der WLAN-Verfügbarkeit und Wechseln zwischen AP und WLAN
void checkWiFiReconnect() {
  if (WiFi.status() != WL_CONNECTED && !isAPMode) {
    // Wenn die WLAN-Verbindung verloren geht und der Access Point nicht aktiv ist
    Serial.println("\nWLAN-Verbindung verloren. Wechsel zum Access Point-Modus...");
    WiFi.softAP(ap_ssid, ap_password);
    Serial.println("Access Point gestartet!");
    Serial.println("AP IP-Adresse: " + WiFi.softAPIP().toString());
    isAPMode = true;
  } else if (WiFi.status() == WL_CONNECTED && isAPMode) {
    // Wenn WLAN wieder verfügbar ist und der Access Point noch läuft
    Serial.println("\nWLAN wiederhergestellt. Wechsel zum WLAN-Modus...");
    WiFi.softAPdisconnect(true);
    isAPMode = false;
    Serial.println("Access Point deaktiviert.");
  }
}

// ESP-Now Callback-Funktion zum Empfangen von Daten
void onDataReceive(const uint8_t *mac, const uint8_t *incomingData, int len) {
  struct_message receivedData;
  memcpy(&receivedData, incomingData, sizeof(receivedData));  // Empfängt Temperatur und Luftfeuchtigkeit
  externalTemperature = receivedData.temperature;  // Außentemperatur speichern
  externalHumidity = receivedData.humidity;        // Außenluftfeuchtigkeit speichern
  Serial.print("Empfangene Außentemperatur: ");
  Serial.println(externalTemperature);
  Serial.print("Empfangene Außenluftfeuchtigkeit: ");
  Serial.println(externalHumidity);
}

void setup() {
  Serial.begin(115200);

  // DHT-Sensor starten
  dht.begin();

  // LittleFS starten
  if (!LittleFS.begin()) {
    Serial.println("Fehler beim Starten von LittleFS");
    return;
  }

  // WLAN verbinden (Station Mode)
  connectToWiFi();

  // mDNS starten und Hostnamen setzen
  if (!MDNS.begin("FahrradwohnwagenSteuerung")) {
    Serial.println("Fehler beim Starten von mDNS");
    return;
  }
  Serial.println("mDNS-Dienst gestartet. Zugriff über FahrradwohnwagenSteuerung.local");

  // Webserver als Dienst registrieren
  MDNS.addService("http", "tcp", 80);

  // HTML-Datei laden
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/index.html", "text/html");
  });

  // CSS-Datei laden
  server.on("/styles.css", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/styles.css", "text/css");
  });

  // JavaScript-Datei laden
  server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/script.js", "application/javascript");
  });

  // Sensordaten als JSON abrufen
  server.on("/data", HTTP_GET, [](AsyncWebServerRequest *request) {
    String jsonData = getDHTData();
    request->send(200, "application/json", jsonData);
  });

  // ESP-Now initialisieren
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-Now Initialisierung fehlgeschlagen");
    return;
  }
  esp_now_register_recv_cb(onDataReceive);  // Callback-Funktion zum Empfangen von Daten registrieren

  // Webserver starten
  server.begin();
}

void loop() {
  // Überprüfen, ob das WLAN alle 10 Sekunden noch verbunden ist
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    checkWiFiReconnect();
  }
}
