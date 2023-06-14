#include <ESP8266WiFi.h>
#include <espnow.h>
#include <Adafruit_Sensor.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <FS.h>

uint8_t broadcastAddress[] = {0x3C, 0x71, 0xBF, 0x2E, 0x39, 0xF9};
int Per = 0;
int incomingPer = 0;
int incomingPerAnterior = 0;
const long interval = 1000;
unsigned long previousMillis = 0;
const uint8_t PerPin1 = 0;
const uint8_t PerPin2 = 2;
const char* ssid = "SoyMateo";

void setup() {
  //tick.attach(0.001, milliseconds);
  pinMode(PerPin1, OUTPUT);
  pinMode(PerPin2, OUTPUT);
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

  if(incomingPer == 0)
  {
    if(incomingPerAnterior == 1)
    {
      digitalWrite(PerPin1, 0);
      digitalWrite(PerPin2, 1);
      Serial.println("BAJANDO");
      delay(30000);
      incomingPerAnterior = 0;
    }
    if(incomingPerAnterior == 2)
    {
      digitalWrite(PerPin1, 0);
      digitalWrite(PerPin2, 1);
      Serial.println("BAJANDO");
      delay(60000);
      incomingPerAnterior = 0;
    }
  }
  if(incomingPer == 1)
  {
    if(incomingPerAnterior == 0)
    {
      digitalWrite(PerPin1, 1);
      digitalWrite(PerPin2, 0);
      Serial.println("SUBIENDO");
      delay(30000);
      incomingPerAnterior = 1;
    }
    if(incomingPerAnterior == 2)
    {
      digitalWrite(PerPin1, 0);
      digitalWrite(PerPin2, 1);
      Serial.println("BAJANDO");
      delay(30000);
      incomingPerAnterior = 1;
    }
  }
  if(incomingPer == 2)
  {
    if(incomingPerAnterior == 0)
    {
      digitalWrite(PerPin1, 1);
      digitalWrite(PerPin2, 0);
      Serial.println("SUBIENDO");
      delay(60000);
      incomingPerAnterior = 2;
    }
    if(incomingPerAnterior == 1)
    {
      digitalWrite(PerPin1, 1);
      digitalWrite(PerPin2, 0);
      Serial.println("SUBIENDO");
      delay(30000);
      incomingPerAnterior = 2;
    }
  }

  digitalWrite(PerPin1, 0);
  digitalWrite(PerPin2, 0);

  Per = incomingPer;
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
  memcpy(&incomingPer, incomingData, sizeof(incomingPer));
  incomingPer = incomingPer - 48;
  Serial.print("incoming Persiana: ");
  Serial.println(incomingPer);
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
