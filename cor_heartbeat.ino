#include <WiFi.h>
#include <WebSocketsServer.h>
#include <HTTPClient.h>

// 🔹 Home WiFi Credentials (STA Mode) - For Internet & WhatsApp Alerts
const char *ssid_STA = "Redmi";  
const char *password_STA = "Haha123456789";  

// 🔹 ESP32 Hotspot Credentials (AP Mode) - For Local Data Plotting
const char *ssid_AP = "ESP32_ECG";  
const char *password_AP = "12345678";  

// WebSocket Server
WebSocketsServer webSocket = WebSocketsServer(81);

// Moving Average Filter Parameters
const int movingAvgWindowSize = 2;
int movingAvgValues[movingAvgWindowSize];
int movingAvgIndex = 0;
int movingAvgSum = 0;

// CallMeBot WhatsApp API Details
String phoneNumber = "+8801789489050";  
String apiKey = "3700741";  

// WhatsApp Alert Control
bool alertSent = false;
bool firstCheck = true;

// Time Control for WhatsApp Alerts
unsigned long lastAlertTime = 0;  
const unsigned long alertInterval = 5 * 60 * 1000;  

// Heart Rate Detection Variables
const int ecgThreshold = 2300; 
const int maxBPM = 140;
const int minBPM = 55;
const int bpmAvgSize = 5;
int bpmBuffer[bpmAvgSize] = {0};  
int bpmIndex = 0;
int bpmSum = 0;
int bpmCount = 0;
unsigned long lastPeakTime = 0;
int heartRate = 0;

void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length) {
  // Handle WebSocket events if needed
}

void setup() {
  Serial.begin(9600);

  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(ssid_AP, password_AP);
  Serial.println("ESP32 Hotspot Created!");
  Serial.print("AP IP Address: ");
  Serial.println(WiFi.softAPIP());

  WiFi.begin(ssid_STA, password_STA);
  Serial.print("Connecting to Home WiFi...");
  
  int retries = 0;
  while (WiFi.status() != WL_CONNECTED && retries < 30) {
    delay(1000);
    Serial.print(".");
    retries++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected to Home WiFi!");
    Serial.print("ESP32 IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nFailed to connect to Home WiFi.");
  }

  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  pinMode(22, INPUT);
  pinMode(23, INPUT);
  pinMode(34, INPUT);  

  for (int i = 0; i < movingAvgWindowSize; i++) {
    movingAvgValues[i] = 0;
  }
}

int movingAvgFilter(int newValue) {
  movingAvgSum -= movingAvgValues[movingAvgIndex];  
  movingAvgValues[movingAvgIndex] = newValue;  
  movingAvgSum += newValue;  
  movingAvgIndex = (movingAvgIndex + 1) % movingAvgWindowSize;  
  return movingAvgSum / movingAvgWindowSize;  
}

void sendWhatsAppAlert(int ecgValue) {
  String message = "ECG Alert! Abnormal ECG detected: " + String(ecgValue);
  String url = "https://api.callmebot.com/whatsapp.php?phone=+8801789489050&text=ECG+Alert!+Abnormal+ECG+detected:" + String(ecgValue) + "&apikey=3700741";
  //String url = "https://api.callmebot.com/whatsapp.php?phone=" + phoneNumber + "&text=" + message + "&apikey=" + apiKey;

  Serial.println("URL: " + url);  
  Serial.println("Sending WhatsApp alert...");
  
  HTTPClient http;
  http.setTimeout(5000);
  http.begin(url);
  int httpResponseCode = http.GET();  

  Serial.print("HTTP Response Code: ");
  Serial.println(httpResponseCode);

  if (httpResponseCode == 200) {
    Serial.println("WhatsApp Message Sent!");
  } else {
    Serial.print("Error sending message. Response: ");
    Serial.println(httpResponseCode);
  }
  
  http.end();  
}

void loop() {
  webSocket.loop();

  int a = digitalRead(22);
  int b = digitalRead(23);
  int ecgValue = analogRead(34);  
  int filteredECG = movingAvgFilter(ecgValue);  

  Serial.println(filteredECG);

 
if (a == 0 && b == 0) {
  String ecgData = String(filteredECG) + "," + String(heartRate);  // Send ECG and heart rate
  webSocket.broadcastTXT(ecgData);  
} else {
  webSocket.broadcastTXT("0");  
}



  unsigned long currentTime = millis();  
  if (firstCheck && filteredECG > 1000) {  
    sendWhatsAppAlert(filteredECG);  
    alertSent = true;  
    firstCheck = false;  
    lastAlertTime = currentTime;
  }

  if (!firstCheck && filteredECG > 2000 && !alertSent && (currentTime - lastAlertTime >= alertInterval)) {  
    sendWhatsAppAlert(filteredECG);
    alertSent = true;
    lastAlertTime = currentTime;
  }

  if (filteredECG < 2000) {  
    alertSent = false;  
  }

  if (filteredECG > ecgThreshold && (currentTime - lastPeakTime) > 600) {  
    unsigned long timeBetweenPeaks = currentTime - lastPeakTime;
    int currentBPM = 60000 / timeBetweenPeaks;  
    lastPeakTime = currentTime;

    if (currentBPM > minBPM && currentBPM < maxBPM) {
      bpmSum -= bpmBuffer[bpmIndex];
      bpmBuffer[bpmIndex] = currentBPM;
      bpmSum += currentBPM;
      bpmIndex = (bpmIndex + 1) % bpmAvgSize;

      if (bpmCount < bpmAvgSize) bpmCount++;
      heartRate = bpmSum / bpmCount;

      Serial.print("Heart Rate: ");
      Serial.print(heartRate);
      Serial.println(" BPM");
    }
  }
  delay(10);  
}
