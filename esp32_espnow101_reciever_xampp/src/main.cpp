/*++++++++++++
  thanet Y.
  16/9/2024 
  for test espnow
  -------------*/

#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include "ESPAsyncWebServer.h"
#include <Arduino_JSON.h>
#include <HTTPClient.h> 

// Network configurations
const char* ssid1 = "True Enjoy";
const char* password1 = "enjoy7777777777";
String URL1 = "http://192.168.1.57/espdata_00/upload_01.php";

const char* ssid2 = "ENJMesh";
const char* password2 = "enjoy042611749";
String URL2 = "http://192.168.0.113/EspData/upload_01.php";

// Variables to hold the current configuration
const char* ssid = "";
const char* password = "";
String URL = "";


// Public variables
int readmoduleno = 0;
float temperature = 0; 
float humidity = 0;
int readId = 0;
bool readytoupload = false;

// Prototype function
void initializeEspNow();
void UploadData2Xampp();
void selectNetworkConfiguration();
void wifi_init();

// Structure to receive data
typedef struct struct_message {
  int read_module_no;
  float temp;
  float hum;
  unsigned int readingId;
} struct_message;

struct_message incomingReadings;

JSONVar board;

// Callback function for received data
void OnDataRecv(const uint8_t * mac_addr, const uint8_t *incomingData, int len) { 
  char macStr[18];
  Serial.print("Packet received from: ");
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.println(macStr);
  memcpy(&incomingReadings, incomingData, sizeof(incomingReadings));
  
  board["read_module_no"] = incomingReadings.read_module_no;
  board["temperature"] = incomingReadings.temp;
  board["humidity"] = incomingReadings.hum;
  board["readingId"] = incomingReadings.readingId;
  String jsonString = JSON.stringify(board);
  //events.send(jsonString.c_str(), "new_readings", millis());
  
  Serial.printf("Board ID %u: %u bytes\n", incomingReadings.read_module_no, len);
  Serial.printf("Temperature: %4.2f \n", incomingReadings.temp);
  Serial.printf("Humidity: %4.2f \n", incomingReadings.hum);
  Serial.printf("Reading ID: %d \n", incomingReadings.readingId);
  Serial.println();

  readmoduleno = incomingReadings.read_module_no;
  temperature = incomingReadings.temp;
  humidity = incomingReadings.hum;
  readId = incomingReadings.readingId;
  readytoupload = true;
}

void setup(){
  Serial.begin(115200);

   // Initialize WiFi and select network configuration
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(WIFI_PS_NONE);

  
  // for initial loop 2 wifi sidd 
  selectNetworkConfiguration();

  // for initial wifi connection
  wifi_init();

  
}
 
void loop(){
  Serial.println("waiting for data to come..");
  Serial.println("..");

  // ++ upload data to xampp
  if (readytoupload && temperature > 0 && humidity > 0 ){
    // for initial EspNow
  initializeEspNow();
    UploadData2Xampp();

    WiFi.disconnect();

  } else {
    Serial.println("No data to upload..");
    
  }

  delay(5000);

}

void initializeEspNow() {
// Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Register for received data callback
  esp_now_register_recv_cb(OnDataRecv);

}

// Function to select network configuration
void selectNetworkConfiguration() {
  // Scan for available networks
  int n = WiFi.scanNetworks();
  Serial.println("Scan done");

  bool network1Found = false;
  bool network2Found = false;

  // Check if the desired networks are found
  for (int i = 0; i < n; i++) {
    if (WiFi.SSID(i) == ssid1) {
      network1Found = true;
      Serial.println("Network 1 found");
    }
    if (WiFi.SSID(i) == ssid2) {
      network2Found = true;
      Serial.println("Network 2 found");
    }
  }

  // Select network configuration based on availability
  if (network1Found) {
    ssid = ssid1;
    password = password1;
    URL = URL1;
  } else if (network2Found) {
    ssid = ssid2;
    password = password2;
    URL = URL2;
  } else {
    Serial.println("No preferred networks found, using default configuration.");
    ssid = ssid2;  // Default to second network if none of the preferred ones are found
    password = password2;
    URL = URL2;
  }
}

void UploadData2Xampp() {
  Serial.println("Uploading data to server...");

  String postData = "read_module_no=" + String(readmoduleno) + 
                    "&temperature=" + String(temperature) + 
                    "&humidity=" + String(humidity) + 
                    "&readingId=" + String(readId); 

  HTTPClient http; 
  http.begin(URL);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  
  int httpCode = http.POST(postData); 
  if (httpCode > 0) {
    String payload = http.getString(); 
    Serial.print("HTTP Code: "); Serial.println(httpCode); 
    Serial.print("Response: "); Serial.println(payload); 
  } else {
    Serial.print("HTTP POST Error: "); Serial.println(httpCode); 
  }
  
  http.end();  // Close connection
  readytoupload = false;  // reset status to prevent double upload
  delay(20000); // Delay between uploads
}

void wifi_init() {
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("Connected");
  Serial.print("Station IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.print("Wi-Fi Channel: ");
  Serial.println(WiFi.channel());
}