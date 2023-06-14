#include <ESP8266WiFi.h>
#include <espnow.h>
#include <Adafruit_Sensor.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <FS.h>

uint8_t broadcastAddress[] = {0x3C, 0x71, 0xBF, 0x2E, 0x39, 0xF9};
String luzDim = "0";
int incomingLuzDim = 0;
int incomingLuzDimAnterior = 0;
int habilitacion = 0;
const uint8_t luzDimPin = 15;
const char* ssid = "SoyMateo";
//PWM
int PWMv = 0;           // Ton entre 0 y 255
const int freq = 50;      //frecuencia


void setup() {
  pinMode(luzDimPin, OUTPUT);
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
  
  if (habilitacion == 1) 
  {
    habilitacion = 0;
    
    //esp_now_send(broadcastAddress, (uint8_t *) &luzDim, sizeof(luzDim));
    Serial.print("Estado Luz dimerizable: ");
    Serial.println(incomingLuzDim);
    PWMv = (incomingLuzDim*255)/10;
    if(incomingLuzDim > 9)
    {
      PWMv = 255;
    }
    Serial.print("PWMv = ");
    Serial.println(PWMv);
    analogWrite(luzDimPin, PWMv);
  } 
}

void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus) {
}

void OnDataRecv(uint8_t * mac, uint8_t *incomingData, uint8_t len) {
  memcpy(&incomingLuzDim, incomingData, sizeof(incomingLuzDim));
  incomingLuzDim = incomingLuzDim - 48;
  Serial.print("Incoming data:");
  Serial.println(incomingLuzDim);
  if( incomingLuzDim != incomingLuzDimAnterior )
  {
    habilitacion = 1;
  }
  incomingLuzDimAnterior = incomingLuzDim;
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
