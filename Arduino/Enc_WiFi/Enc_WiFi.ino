#include <ESP8266WiFi.h>
#include <espnow.h>
#include <Adafruit_Sensor.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <FS.h>

uint8_t broadcastAddress[] = {0x3C, 0x71, 0xBF, 0x2E, 0x39, 0xF9};
int Enc = 0;
int incomingEnc = 0;
const long interval = 100;
unsigned long previousMillis = 0;
String success; 
const uint8_t EncPin = 2;
const char* ssid = "SoyMateo";

void setup() {
  pinMode(EncPin, OUTPUT);
  Serial.begin(9600);
  WiFi.mode(WIFI_STA);
  int32_t channel = getWiFiChannel(ssid);
  WiFi.printDiag(Serial); // Uncomment to verify channel number before
  wifi_set_channel(channel);
  WiFi.printDiag(Serial);
  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
  esp_now_register_send_cb(OnDataSent);
  esp_now_add_peer(broadcastAddress, ESP_NOW_ROLE_COMBO, 1, NULL, 0);
  esp_now_register_recv_cb(OnDataRecv);
}

void loop() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) 
  {
    previousMillis = currentMillis;
 
    //esp_now_send(broadcastAddress, (uint8_t *) &Enc, sizeof(Enc));

    // Print incoming readings
    printIncomingReadings();
  } 
}

void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus) 
{
  Serial.print("Estado del ultimo enviado: ");
  if (sendStatus == 0){
    Serial.println("Delivery success");
  }
  else{
    Serial.println("Delivery fail");
  }
}

void OnDataRecv(uint8_t * mac, uint8_t *incomingData, uint8_t len) 
{
  memcpy(&incomingEnc, incomingData, sizeof(incomingEnc));
  incomingEnc = incomingEnc - 48;
  Serial.print("Bytes recibidos: ");
  Serial.println(len);
}

void printIncomingReadings(){
  // Display Readings in Serial Monitor
  Serial.print("MAC of this ESP8266: ");
  Serial.println(WiFi.macAddress());
  Serial.println("INCOMING READINGS");
  Serial.print("Estado enchufe: ");
  Serial.println(incomingEnc);
  digitalWrite(EncPin, incomingEnc);
}

int32_t getWiFiChannel(const char *ssid) {
  if (int32_t n = WiFi.scanNetworks()) {
      for (uint8_t i=0; i<n; i++) {
          if (!strcmp(ssid, WiFi.SSID(i).c_str())) {
              return WiFi.channel(i);
          }
      }
  }
  return 0;
}
