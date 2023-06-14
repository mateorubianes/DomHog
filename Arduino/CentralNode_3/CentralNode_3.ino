#include <ESP8266WiFi.h>
#include <espnow.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Wire.h>    
#include <Adafruit_Sensor.h>  
#include <Adafruit_BMP280.h>
#include <Ticker.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <FS.h>

AsyncWebServer server(80);
AsyncEventSource events("/events");
Adafruit_BMP280 bmp;   
Ticker tick;
WiFiUDP ntpUDP;

//Broadcast adresses
uint8_t broadcastAddressLuzEA[] = {0x84, 0xF3, 0xEB, 0x74, 0x30, 0xAA}; //Slave address LuzEA
uint8_t broadcastAddressLuzDim[] = {0x2C, 0x3A, 0xE8, 0x45, 0x32, 0x2A}; //Slave address LuzDim
uint8_t broadcastAddressEnc[] = {0xA0, 0x20, 0xA6, 0x2F, 0x81, 0xB8}; //Slave address enchufE
uint8_t broadcastAddressPer[] = {0xEC, 0xFA, 0xBC, 0x1C, 0xEF, 0xBD}; //Slave address Persiana
//VAR html
const char* PARAM_INPUT_ENC1 = "E1";
const char* PARAM_INPUT_LUZ1 = "L1";
const char* PARAM_INPUT_SLIDERPER = "SP";
const char* PARAM_INPUT_SLIDERLUZDIM = "SL";
const char* PARAM_INPUT_SLIDERTEMP = "ST";
const char* PARAM_INPUT_HORAINICIO = "HI";
const char* PARAM_INPUT_HORAFIN = "HF";
//WiFi variables
const char* ssid = "SoyMateo";
const char* password = "elpibevalderrama";
int32_t canalWifiComunicacion = 0;
int32_t canalWifiPagina = 0;
String ip; //{"$W000000000000#"}
//Pinout variables
const uint8_t enchufe = 2;
//States variables
String luzEA_G = "0";
String encAnterior = "0";
String enc_G = "0";
String sliderTemp_G = "10";
String luzDim_G = "0";
String per_G = "0";
String luzEA_web = "0";
//Wifi communication
int incomingReadings = 0;
//Sensor temp y presion
float TEMPERATURA;    
float PRESION; 
//Comunicación serie
String serie_disp = {"$D0000V#"};    //$DXXXX#
String serie_c_disp = {"$D0000#"};
String serie_temp = {"$T0V#"};    //$MXXXX#
String serie_sensor = {"$M000V#"};
String serie_c_temp  = {"$M0000#"};
unsigned char trama_chksum[3];
//Counter
long unsigned int counter = 0;
//Habilitaciones
int habilitacion = 0;
int habilitacionHora = 0;
//Time
NTPClient timeClient(ntpUDP, "ar.pool.ntp.org", -10800, 60000);
char dayWeek [7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
String tiempo;
String hora_ini;
String hora_fin;

void milliseconds()
{
  counter ++;
}

void setup() 
{
  tick.attach(0.001, milliseconds);
  ConfigurationStarterPack();
  FilesLoad();
  Backend();

  server.addHandler(&events);
  server.begin();
  timeClient.begin();
}

void loop() 
{
  if(counter >= 5000)
  {
    counter = 0;
    
    SerialData();
    
    //Reading temperature
    TEMPERATURA = bmp.readTemperature();    
    PRESION = bmp.readPressure()/100;   // por 100 para covertirlo a hectopascales
    serie_sensor [2] = (int)TEMPERATURA + 36;
    trama_chksum [0] = (int)TEMPERATURA + 36;
    serie_sensor [3] = ((int)PRESION / 100) + 36;
    trama_chksum [1] = ((int)PRESION / 100) + 36;
    serie_sensor [4] = ((int)PRESION % 100) + 36;
    trama_chksum [2] = ((int)PRESION % 100) + 36;
    serie_sensor[5] = checksum_verif(trama_chksum, 3);
    Serial.println(serie_sensor);

    tiempo = timeClient.getFormattedTime();
    timeClient.update();
    tiempo.remove(5,3);

    if ( hora_ini.equals(tiempo) )
    {
      luzEA_web = "1";
      luzDim_G = "9";
      serie_disp [2] = 1; 
      serie_disp [3] = 9;
      habilitacion = 1;
      habilitacionHora = 0;
    }

    if(habilitacionHora != 1)
    {
      if (hora_fin.equals(tiempo))
      {
        luzEA_web = "0";
        luzDim_G = "0";
        serie_disp [2] = 0; 
        serie_disp [3] = 0;
        habilitacion = 1;
        habilitacionHora = 1;
      }
    }

    if (serie_c_disp != serie_disp)
    {
      trama_chksum [0] = serie_disp[2];
      trama_chksum [1] = serie_disp[3];
      trama_chksum [2] = serie_disp[4];
      trama_chksum [3] = serie_disp[5];
      
      serie_disp[6] = checksum_verif(trama_chksum, 4);
      Serial.println(serie_disp);
      serie_c_disp = serie_disp;
    }
  }
 
  if ( habilitacion == 1 ) 
  {
    habilitacion = 0;
    esp_now_send(broadcastAddressLuzDim, (uint8_t *) &luzDim_G, sizeof(luzDim_G));
    esp_now_send(broadcastAddressLuzEA, (uint8_t *) &luzEA_web, sizeof(luzEA_web));
    esp_now_send(broadcastAddressPer, (uint8_t *) &per_G, sizeof(per_G));
    esp_now_send(broadcastAddressEnc, (uint8_t *) &enc_G, sizeof(enc_G));
  }

  if(sliderTemp_G.toInt() > TEMPERATURA)
  {
    enc_G = "1";
    serie_disp [4] = 49;
    if(encAnterior.toInt() == 0)
    {
      habilitacion = 1;
    }
    encAnterior = "1";
  }
  else
  {
    enc_G = "0";
    serie_disp [4] = 48;
    if(encAnterior.toInt() == 1)
    {
      habilitacion = 1;
    }
    encAnterior = "0";
  }
}

/* 
     #########################################################################################
     ####################################### ESPNOW  #########################################
     #########################################################################################
  */

void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus) {
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
  
  memcpy(&incomingReadings, incomingData, sizeof(incomingReadings));
  incomingReadings = incomingReadings - 48;
  Serial.print("informacion recibida: ");
  Serial.println(incomingReadings );

  if( incomingReadings == 1 || incomingReadings == -48 )
  {
    luzEA_G = "1"; 
    luzEA_web = "1";
    events.send("El nombre del padre Abel es....", "new_readings", millis());
    Serial.println("Entre al evento");
  }
  if( incomingReadings == 0 || incomingReadings == -47)
  {
    luzEA_G = "0"; 
    luzEA_web = "0";
    events.send("El nombre del padre Abel es....", "new_readings", millis());
    Serial.println("Entre al evento");
  }
}

/* 
     #########################################################################################
     ####################################### CONFIGURATIONS ##################################
     #########################################################################################
  */

void ConfigurationStarterPack(void)
{
  Serial.begin(9600);
  pinMode(enchufe, OUTPUT);
  if ( !bmp.begin() ) {   
    Serial.println("BMP280 no encontrado !"); 
    while (1);         
  }
  if(!SPIFFS.begin()){
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  
  // Connect to Wi-Fi
  WiFi.mode(WIFI_AP_STA); 
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.println("Connecting to WiFi......");
  }
  
  //Setting chanel
  wifi_set_channel(getWiFiChannel(ssid));

  ip = String("$W") +
  ipConverter ( String(WiFi.localIP()[0]) ) +
  ipConverter ( String(WiFi.localIP()[1]) ) + 
  ipConverter ( String(WiFi.localIP()[2]) ) + 
  ipConverter ( String(WiFi.localIP()[3]) ) +
  String("#");
  
  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
  esp_now_register_send_cb(OnDataSent);
  esp_now_add_peer(broadcastAddressLuzEA, ESP_NOW_ROLE_COMBO, 1, NULL, 0);
  esp_now_add_peer(broadcastAddressLuzDim, ESP_NOW_ROLE_COMBO, 1, NULL, 0);
  esp_now_add_peer(broadcastAddressEnc, ESP_NOW_ROLE_COMBO, 1, NULL, 0);
  esp_now_add_peer(broadcastAddressPer, ESP_NOW_ROLE_COMBO, 1, NULL, 0);
  esp_now_register_recv_cb(OnDataRecv);
  
  Serial.println(ip);
}

String ipConverter(String ip)
{
  if(ip.length() == 2)
  {
    ip = String("0") + ip;
    return ip;
  }
  if(ip.length() == 1)
  {
    ip = String("0") + String("0") + ip;
    return ip;
  }
  else
  {
    return ip;
  }
  return String();
}

/* 
     #########################################################################################
     ####################################### FILES LOAD ######################################
     #########################################################################################
  */

void FilesLoad(void)
{
  // = = = = = = = = > HTML
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });

  server.on("/PAGINAS/S3M.html", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/PAGINAS/S3M.html", String(), false);
  });
  
  // = = = = = = = = > CSS
  server.on("/CSS/estilos.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/CSS/estilos.css", "text/css");
  });

  // = = = = = = = = > JavaScript
  server.on("/JS/script.js", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/JS/script.js", "text/javascript");
  });

  // = = = = = = = = > Event Handler
  events.onConnect([](AsyncEventSourceClient *client){
    if(client->lastId()){
      Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
    }
    client->send("hello!", NULL, millis(), 10000);
  });
}

void Backend(void)
{
  // = = = = = = = = = = = = = = = = = = = = = = = = = = = = =
  // = = = = = = = = > Slider temperatura  = = = = = = = = = = 
  // = = = = = = = = = = = = = = = = = = = = = = = = = = = = =
  
  server.on("/sliderTemp", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    String inputMessage;
    if ( request->hasParam(PARAM_INPUT_SLIDERTEMP) ) 
    {
      inputMessage = request->getParam(PARAM_INPUT_SLIDERTEMP)->value();
    }
    else 
    {
      inputMessage = "No message sent";
    }

    sliderTemp_G = inputMessage;
    serie_temp [2] = inputMessage.toInt() + 36;
    trama_chksum [0] = inputMessage.toInt() + 36;
    serie_temp [3] = checksum_verif(trama_chksum, 1);
    Serial.println(serie_temp);
    
    request->send(200, "text/plain", "OK");
    
  });

  // = = = = = = = = = = = = = = = = = = = = = = = = = = = = =
  // = = = = = = = = = => Luz enc/apagado  = = = = = = = = = =
  // = = = = = = = = = = = = = = = = = = = = = = = = = = = = =
  
  server.on("/luzEA", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    String inputMessage;
    if ( request->hasParam(PARAM_INPUT_LUZ1) ) 
    {
      inputMessage = request->getParam(PARAM_INPUT_LUZ1)->value();
    }
    else 
    {
      inputMessage = "No message sent";
    }

    luzEA_web = inputMessage;
    serie_disp [2] = inputMessage.toInt() + 48;
    request->send(200, "text/plain", "OK");
    habilitacion = 1;
    
  });

  // = = = = = = = = = = = = = = = = = = = = = = = = = = = = =
  // = = = = = = = = = => Slider Persiana  = = = = = = = = = =
  // = = = = = = = = = = = = = = = = = = = = = = = = = = = = =
  
  server.on("/sliderPer", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    String inputMessage;
    if ( request->hasParam(PARAM_INPUT_SLIDERPER) ) 
    {
      inputMessage = request->getParam(PARAM_INPUT_SLIDERPER)->value();
    }
    else 
    {
      inputMessage = "No message sent";
    }

    habilitacion = 1;
    per_G = inputMessage;
    serie_disp [5] = inputMessage.toInt() + 48;
    request->send(200, "text/plain", "OK");
    
  });
  
  // = = = = = = = = = = = = = = = = = = = = = = = = = = = = =
  // = = = = = = = > Slider Luz Dimerizable  = = = = = = = = = 
  // = = = = = = = = = = = = = = = = = = = = = = = = = = = = =
  
  server.on("/sliderLuzDim", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    String inputMessage;
    if ( request->hasParam(PARAM_INPUT_SLIDERLUZDIM) ) 
    {
      inputMessage = request->getParam(PARAM_INPUT_SLIDERLUZDIM)->value();
    }
    else 
    {
      inputMessage = "No message sent";
    }

    luzDim_G = inputMessage;
    serie_disp [3] = inputMessage.toInt() + 48;
    request->send(200, "text/plain", "OK");
    habilitacion = 1;
    
  });

  // = = = = = = = = = = = = = = = = = = = = = = = = = = = = =
  // = = = = = = = = = = = => enchufe  = = = = = = = = = = = =
  // = = = = = = = = = = = = = = = = = = = = = = = = = = = = =
  server.on("/enchufe", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    String inputMessage;
    if ( request->hasParam(PARAM_INPUT_ENC1) ) 
    {
      inputMessage = request->getParam(PARAM_INPUT_ENC1)->value();
    }
    else 
    {
      inputMessage = "No message sent";
    }
    
    request->send(200, "text/plain", "OK");
    
  });

  // = = = = = = = = = = = = = = = = = = = = = = = = = = = = =
  // = = = = = = = = = = = => Hora = = = = = = = = = = = = = =
  // = = = = = = = = = = = = = = = = = = = = = = = = = = = = =
  server.on("/hora", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    if ( request->hasParam(PARAM_INPUT_HORAINICIO) ) 
    {
        hora_ini = request->getParam(PARAM_INPUT_HORAINICIO)->value();
        Serial.print("hora de inicio = ");
        Serial.println(hora_ini);
        
    }
    if ( request->hasParam(PARAM_INPUT_HORAFIN) ) 
    {
        hora_fin = request->getParam(PARAM_INPUT_HORAFIN)->value();
        Serial.print("hora de fin = ");
        Serial.println(hora_fin);
    }

    request->send(200, "text/plain", "OK");
  });
  
}

/* 
     #########################################################################################
     ###################################### Receiving Data ###################################
     #########################################################################################
*/

void SerialData (void)
{
  String incomingByte;
  incomingByte = Serial.readString();
  
  if(incomingByte[0] == '$' && incomingByte[1] == 'D')
  {
      if(incomingByte[2] == '1' )
      {
        luzEA_G = '1';
        luzEA_web = '1';
      }
      else
      {
        luzEA_G = '0';
        luzEA_web = '0';
      }
      
      if(incomingByte[3] == '1' )
        enc_G = '1';
      else
        enc_G = '0';

      if(incomingByte[4] == '1' )
        per_G = '1';
      else if(incomingByte[4] == '2')
        per_G = '2';
      else
        per_G = '0';
      
      switch(incomingByte[5])
      {
        case '0':
          luzDim_G = '0';
          break;
        case '1':
          luzDim_G = '1';
          break;
        case '2':
          luzDim_G = '2';
          break;
        case '3':
          luzDim_G = '3';
          break;     
        case '4':
          luzDim_G = '4';
          break;
        case '5':
          luzDim_G = '5';
          break;
        case '6':
          luzDim_G = '6';
          break;
        case '7':
          luzDim_G = '7';
          break;
        case '8':
          luzDim_G = '8';
          break;     
        case '9':
          luzDim_G = '9';
          break;
        default:
          luzDim_G = "10";
          break;
      }
      habilitacion = 1;
      events.send("El nombre del padre Abel es....", "new_readings", millis());
      Serial.println("Entre al evento");
  }
  if(incomingByte[0] == '$' && incomingByte[1] == 'M')
  {
    sliderTemp_G[0] = incomingByte[2];
    sliderTemp_G[1] = incomingByte[3];
    habilitacion = 1;
    events.send("El nombre del padre Abel es....", "new_readings", millis());
    Serial.println("Entre al evento");
  }
}

char checksum_verif(unsigned char * dataframe, int largo)// Paso el vector de datos
{
    char chksum = 0;
    // Hago el XOR de cada uno de los bytes de la trama
    for(int i = 0; i < largo; i++)
        chksum = chksum ^ int(dataframe[i]); // XOR Byte a byte
    
    return chksum; // Devuelvo el byte de verificacion hecho localmente
}

/* 
     #########################################################################################
     ###################################### PROCESSOR #######################################
     #########################################################################################
*/

String processor(const String& var)
{
  String buttons = "";
  if(var == "BUTTONENC1")
  {
    if(enc_G.toInt() == 0)
    {
      String buttons = "";
      buttons += "<label class=\"switch\"><input type=\"checkbox\" onchange=\"EncEnc(this)\" id =\"ENC\" ><span class=\"slider\"></span></label>";
      return buttons;
    }
    if(enc_G.toInt() == 1)
    {
      String buttons = "";
      buttons += "<label class=\"switch\"><input type=\"checkbox\" onchange=\"EncEnc(this)\"  id =\"ENC\" checked=\"true\"><span class=\"slider\"></span></label>";
      return buttons;
    }
  }
  if(var == "BUTTONLUZEA1")
  {
    if(luzEA_G.toInt() == 0)
    {
      String buttons = "";
      buttons += "<div id=\"EA\"><label class=\"switch\"><input type=\"checkbox\" onchange=\"EncLuzEA(this)\"><span class=\"slider\"></span></label></div>";
      return buttons;
    }
    if(luzEA_G.toInt() == 1)
    {
      String buttons = "";
      buttons += "<div id=\"EA\"><label class=\"switch\"><input type=\"checkbox\" onchange=\"EncLuzEA(this)\" checked=\"true\"><span class=\"slider\"></span></label></div>";
      return buttons;
    }
  }
  if(var == "SLIDERPER")
  {
    String buttons = "";
    buttons += "<input type=\"range\" min=\"0\" max=\"2\" step=\"1\" value=" + per_G + " class=\"sliderdim\" onchange=\"slider_Per(this)\">";
    return buttons;
  }
  if(var == "SLIDERLUZDIM")
  {
    String buttons = "";
    buttons += "<div id =\"DIM\"><input type=\"range\" min=\"0\" max=\"10\" step=\"1\" value=" + luzDim_G + " class=\"sliderdim\" onchange=\"slider_LuzDim(this)\"></div>";
    return buttons;
  }
  if(var == "SLIDERTEMP")
  {
    String buttons = "";
    buttons += "<input type=\"range\" min=\"10\" max=\"30\" step=\"1\" value=" + sliderTemp_G + " class=\"sliderdim\" onchange=\"slider_Temp(this)\" >";
    return buttons;
  }
  if(var == "LABELTEMP")
  {
    String buttons = "";
    buttons += "<p>Valor: <span id=\"Temp\">" + sliderTemp_G + "</span></p>";
    return buttons;
  }
  if(var == "LABELDIM")
  {
    String buttons = "";
    buttons += "<p id=\"LuzDim\">Valor: " + luzDim_G + "</p>";
    return buttons;
  }
  if(var == "LABELPER")
  {
    String buttons = "";
    switch(per_G.toInt())
    {
      case 0:
        buttons += "<p>Valor: <span id=\"Per\">BAJA</span></p>";
        break;
      case 1:
        buttons += "<p>Valor: <span id=\"Per\">MEDIA</span></p>";
        break;
      case 2:
        buttons += "<p>Valor: <span id=\"Per\">ALTA</span></p>";
        break; 
    }
    return buttons;
  }
  if(var == "TEMPSEN")
  {
    String buttons = "";
    String temp = String(TEMPERATURA,2);
    buttons += "<p class=\"pApp\">Temperatura:&ensp;<span id=\"tempsen\">" + temp + "</span>ºC</p>";
    return buttons;
  }
  if(var == "PRESIONSEN")
  {
    String buttons = "";
    String pres = String(PRESION, 2);
    buttons += "<p class=\"pApp\">Presión:&ensp;<span id=\"humsen\">" + pres + "</span>psi</p>";
    return buttons;
  }
  return String();
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
