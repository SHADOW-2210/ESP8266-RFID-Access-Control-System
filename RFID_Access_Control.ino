/*
  =====================================================================
  ESP8266 + MFRC522 RFID Access Control System
  Copyright (C) 2026  Anurag Chanda

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program. If not, see <http://www.gnu.org/licenses/>.
  =====================================================================
  Features:
    - Reads RFID card/keyfob UID via MFRC522 (SPI)
    - Checks UID against a local whitelist of authorized cards
    - Grants access: green LED + buzzer beep + relay pulses (unlock)
    - Denies access: red LED + long buzzer + logs the attempt
    - Connects to WiFi, syncs real time via NTP
    - Hosts a simple web page (http://<device-ip>/) showing the last
      20 access log entries (time, UID, granted/denied)
    - Logs every event to Serial monitor too

  SETUP:
    1. Copy config.h.example to config.h
    2. Fill in your WiFi credentials and authorized card UIDs in config.h
    3. config.h is gitignored, so your secrets stay local

  WIRING (NodeMCU ESP8266):
    MFRC522 SDA(SS) -> D4
    MFRC522 SCK     -> D5
    MFRC522 MOSI    -> D7
    MFRC522 MISO    -> D6
    MFRC522 RST     -> D3
    MFRC522 3.3V    -> 3V3   (NOT 5V!)
    MFRC522 GND     -> GND

    Green LED (+resistor) -> D0
    Red LED   (+resistor) -> D8
    Buzzer                -> D2
    Relay IN               -> D1

  LIBRARIES REQUIRED (install via Arduino IDE Library Manager):
    - MFRC522 by GithubCommunity
    - ESP8266WiFi (bundled with ESP8266 board package)
    - ESP8266WebServer (bundled with ESP8266 board package)
    - NTPClient by Fabrice Weinberg
  =====================================================================
*/

#include <SPI.h>
#include <MFRC522.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include "config.h"   // <-- your secrets live here, not in this file

// Pin definitions
#define SS_PIN   D4
#define RST_PIN  D3
#define GREEN_LED D0
#define RED_LED   D8
#define BUZZER    D2
#define RELAY     D1

MFRC522 rfid(SS_PIN, RST_PIN);
ESP8266WebServer server(80);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", NTP_OFFSET_SECONDS, 60000);

// Simple ring buffer log
#define LOG_SIZE 20
struct LogEntry {
  String time;
  String uid;
  bool granted;
};
LogEntry logBuffer[LOG_SIZE];
int logIndex = 0;
int logCount = 0;

void addLog(String uid, bool granted) {
  logBuffer[logIndex].time = timeClient.getFormattedTime();
  logBuffer[logIndex].uid = uid;
  logBuffer[logIndex].granted = granted;
  logIndex = (logIndex + 1) % LOG_SIZE;
  if (logCount < LOG_SIZE) logCount++;
}

String getUID() {
  String uidStr = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) uidStr += "0";
    uidStr += String(rfid.uid.uidByte[i], HEX);
  }
  uidStr.toUpperCase();
  return uidStr;
}

bool isAuthorized(String uid) {
  for (int i = 0; i < NUM_AUTHORIZED; i++) {
    if (uid == String(AUTHORIZED_UIDS[i])) return true;
  }
  return false;
}

void grantAccess() {
  digitalWrite(GREEN_LED, HIGH);
  tone(BUZZER, 1000, 150);
  digitalWrite(RELAY, HIGH);   // unlock
  delay(3000);                 // stay unlocked 3s
  digitalWrite(RELAY, LOW);    // relock
  digitalWrite(GREEN_LED, LOW);
}

void denyAccess() {
  digitalWrite(RED_LED, HIGH);
  tone(BUZZER, 300, 500);
  delay(600);
  digitalWrite(RED_LED, LOW);
}

// ---------------------- WEB SERVER ----------------------
void handleRoot() {
  String html = "<!DOCTYPE html><html><head><title>RFID Access Log</title>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<meta http-equiv='refresh' content='5'>";
  html += "<style>body{font-family:sans-serif;background:#111;color:#eee;padding:20px}";
  html += "table{width:100%;border-collapse:collapse}th,td{padding:8px;border-bottom:1px solid #333;text-align:left}";
  html += ".ok{color:#4caf50}.no{color:#f44336}h1{color:#4caf50}</style></head><body>";
  html += "<h1>RFID Access Log</h1><p>Device time: " + timeClient.getFormattedTime() + "</p>";
  html += "<table><tr><th>Time</th><th>UID</th><th>Status</th></tr>";

  int shown = 0;
  int i = (logIndex - 1 + LOG_SIZE) % LOG_SIZE;
  while (shown < logCount) {
    html += "<tr><td>" + logBuffer[i].time + "</td><td>" + logBuffer[i].uid + "</td><td class='" ;
    html += logBuffer[i].granted ? "ok'>GRANTED" : "no'>DENIED";
    html += "</td></tr>";
    i = (i - 1 + LOG_SIZE) % LOG_SIZE;
    shown++;
  }
  html += "</table></body></html>";
  server.send(200, "text/html", html);
}

// ---------------------- SETUP ----------------------
void setup() {
  Serial.begin(115200);
  delay(200);

  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(RELAY, OUTPUT);
  digitalWrite(RELAY, LOW);

  SPI.begin();
  rfid.PCD_Init();
  Serial.println("RFID reader initialized.");

  Serial.print("Connecting to WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(400);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected! IP address: ");
  Serial.println(WiFi.localIP());

  timeClient.begin();
  timeClient.update();

  server.on("/", handleRoot);
  server.begin();
  Serial.println("Web server started. Visit the IP above in a browser.");

  Serial.println("Ready. Scan a card...");
}

// ---------------------- LOOP ----------------------
void loop() {
  server.handleClient();
  timeClient.update();

  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
    return;
  }

  String uid = getUID();
  Serial.print("Card scanned. UID: ");
  Serial.println(uid);

  if (isAuthorized(uid)) {
    Serial.println("-> ACCESS GRANTED");
    addLog(uid, true);
    grantAccess();
  } else {
    Serial.println("-> ACCESS DENIED (unknown card)");
    addLog(uid, false);
    denyAccess();
  }

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}
