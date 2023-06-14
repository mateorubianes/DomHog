#include <ESP8266WiFi.h>
#include <espnow.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <DetectaFlanco.h>
#include <Ticker.h>
#include <FS.h>

uint8_t broadcastAddress[] = {0x3C, 0x71, 0xBF, 0x2E, 0x39, 0xF9};
String luzEA = "0";
int incomingLuzEA = 0;
int incomingLuzEAanterior = 0;
const uint8_t luzEAPin = 2;
const uint8_t luzEAinput = 15;
int selector = 0;
int selectorAnterior = 0;
const char* ssid = "SoyMateo";
int habilitacion = 0;
int counter = 0;

DetectaFlanco flanco(15);

void setup() 
{
  pinMode(luzEAPin, OUTPUT);
  pinMode(luzEAinput, INPUT);
  Serial.begin(9600);
  WiFi.mode(WIFI_STA);
  wifi_set_channel(getWiFiChannel(ssid));
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

void loop() 
{
  int flanco1 = flanco.comprueba(); 

  if(flanco1 == 1)
  {
    selector = !selector;
    delay(500);
  }  
  if(incomingLuzEA == 1 && incomingLuzEAanterior == 0)
  {
    selector = 1;
  }
  if(incomingLuzEA == 0 && incomingLuzEAanterior == 1)
  {
    selector = 0;
  }
  incomingLuzEAanterior = incomingLuzEA;
  
  if(selector == 1)
  {
    luzEA = "1";
    digitalWrite(luzEAPin, LOW);
    if(selectorAnterior == 0)
    {
      habilitacion = 1;  
    }
  }
  if(selector == 0)
  {
    luzEA = "0";
    digitalWrite(luzEAPin, HIGH);
    if(selectorAnterior == 1)
    {
      habilitacion = 1;  
    }
  }  
  selectorAnterior = selector;
  
  if (habilitacion == 1) 
  {
    habilitacion = 0;
    esp_now_send(broadcastAddress, (uint8_t *) &luzEA, sizeof(luzEA));
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
  Serial.println(" ");
  memcpy(&incomingLuzEA, incomingData, sizeof(incomingLuzEA));
  incomingLuzEA = incomingLuzEA - 48;
  Serial.print("Bytes recibidos: ");
  Serial.println(len);
  Serial.println(" ");
}

void printIncomingReadings(){
  Serial.println(" ");
  // Display Readings in Serial Monitor
  Serial.print("MAC of this ESP8266: ");
  Serial.println(WiFi.macAddress());
  Serial.print("Wi-Fi Channel: ");
  Serial.println(WiFi.channel());
  Serial.println("INCOMING READINGS");
  Serial.print("Estado Luz encendido/apagado MASTER: ");
  Serial.println(incomingLuzEA);
  Serial.print("Estado Luz encendido/apagado SLAVE: ");
  Serial.println(luzEA);
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
