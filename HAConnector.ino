#include <WiFiEsp.h>
#include <WiFiEspClient.h>
#include <WiFiEspUdp.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

char ssid[] = "Vipernet";           // network SSID
char pass[] = "nadlor290228";       // network password
int status = WL_IDLE_STATUS;        // Wifi radio's status
IPAddress MQTT_SERVER(192, 168, 2, 221); // MQTT server address

#define LED_STRIP_STATE "homeassistant/light/office/state"
#define LED_STRIP_CONFIG "homeassistant/light/office/config"
#define LED_STRIP_SET "homeassistant/light/office/set"
#define MQTT_MAX_PACKET_SIZE 327
#define DEBUG
#define PIN_RX 10
#define PIN_TX 11

// Initialize client objects

#ifndef HAVE_HWSERIAL1
#include "SoftwareSerial.h"
SoftwareSerial soft(PIN_RX, PIN_TX); // RX, TX
#endif

WiFiEspClient espClient;
PubSubClient mqttClient(espClient);

char mqttPayloadBuffer[256];
const char* clientID = "ArduinoESP8266";

void setup() {
  Serial.begin(9600); // initialize serial
  soft.begin(9600);   // initialize serial for ESP module

  Wifi_Setup(); // wifi ESP8266 setup
  delay(1000);
  Mqtt_Setup(); // mqtt (mosquitto) setup

  delay(5000);  // Delay to allow interface to autoconfigure
  
  Discovery_Init();
}

void Discovery_Init(){

  const int capacity = JSON_ARRAY_SIZE(1) + JSON_OBJECT_SIZE(4) + JSON_OBJECT_SIZE(7);
  StaticJsonDocument<capacity> payload;
  //DynamicJsonDocument payload(capacity);

  payload["~"] = "homeassistant/light/office";
  payload["name"] = "Office Light";
  payload["unique_id"] = "office_light_1";
  payload["cmd_t"] = "~/set";
  payload["stat_t"] = "~/state";
  payload["schema"] = "json";
  //payload["brightness"] = true;
  JsonObject device = payload.createNestedObject("dev");
  JsonArray identifiers = device.createNestedArray("ids");
  identifiers.add("led_1");  
  device["name"] = "ledstrip";
  device["mdl"] = "StripController";
  device["mf"] = "OpenmindSoftware";

  serializeJson(payload, mqttPayloadBuffer, sizeof(mqttPayloadBuffer));
  //serializeJsonPretty(payload, mqttPayloadBuffer);
}

void Mqtt_Setup(){
  //connect to MQTT server
  mqttClient.setServer(MQTT_SERVER, 1883);
  mqttClient.setCallback(callback);
}


void Wifi_Setup(){
  WiFi.init(&soft);   // initialize ESP module

  // check for the presence of the module
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println(F("WiFi module not present"));
    // stop
    while (true);
  }

  // Connect to WiFi network
  while ( status != WL_CONNECTED) {
    Serial.print(F("Attempting to connect to SSID: "));
    Serial.println(ssid);
    // Connect to WPA/WPA2 network
    status = WiFi.begin(ssid, pass);
  }

  // Wifi is connected
  Serial.print(F("Connected to: "));
  Serial.println(ssid);
}


//print any message received for subscribed topic
void callback(char* topic, byte* payload, unsigned int length) {
  //buffer[0] = '\0'; // elimna o conteudo do array
  //buffer[0] = (char)0;
  //memcpy(received_payload, payload, length); // copy payload
   
  // handle message arrived
  /*char inData[128];
  Serial.print("payload: ");
  for(int i =0; i<length; i++){
    //Serial.print((char)payload[i]);
    inData[i]= (char)payload[i];
    Serial.print(inData[i]);
  }
  Serial.println("");*/

  StaticJsonDocument<10> doc;
  deserializeJson(doc, payload,length);
  JsonObject root = doc.as<JsonObject>();
  const char* state = root["state"];
  #ifdef DEBUG
    Serial.println(state);
  #endif
  
  memset(mqttPayloadBuffer, 0, sizeof(mqttPayloadBuffer)); // clear data buffer
  
  // action for topic
  if(strcmp(topic, LED_STRIP_SET) == 0){

    const int capacity = JSON_OBJECT_SIZE(2);
    StaticJsonDocument<capacity> mqttPayload;

    if(strcmp("ON", state) == 0){
      mqttPayload["state"] = "ON";  
    } else {
      mqttPayload["state"] = "OFF";  
    } 
    mqttPayload["brightness"] = 255;

    //serializeJson(mqttPayload, buffer);
    serializeJsonPretty(mqttPayload, mqttPayloadBuffer);

    #ifdef DEBUG
      Serial.print(F("Print mqtt data buffer: "));
      Serial.println(mqttPayloadBuffer);
      Serial.print(F("Publishing on topic: "));
      Serial.println(LED_STRIP_STATE);
    #endif
    
    mqttClient.publish(LED_STRIP_STATE, mqttPayloadBuffer, true);
  }

  Serial.println();
}

void loop() {
  if (!mqttClient.connected()) {
    reconnect();
  }
  mqttClient.loop();
}

void reconnect() {
  // Loop until we're reconnected
  while (!mqttClient.connected()) {

    #ifdef DEBUG
      Serial.print(F("Attempting MQTT connection..."));  // Attempt to connect, just a name to identify the client
    #endif
    
    if (mqttClient.connect(clientID)) {
      #ifdef DEBUG
        Serial.println(F("connected to mqtt Server"));
        Serial.print(F("Sending Auto Discovery: "));
        Serial.println(LED_STRIP_CONFIG);
        Serial.print(F("mqtt Auto Discovery: "));
        Serial.println(mqttPayloadBuffer);
      #endif
        
      mqttClient.publish(LED_STRIP_CONFIG, mqttPayloadBuffer);  // Once connected, publish an announcement...

      #ifdef DEBUG
        Serial.print(F("Listening on topic: "));
        Serial.println(LED_STRIP_SET);
      #endif
      
      mqttClient.subscribe(LED_STRIP_SET);  // ... and resubscribe
    } else {
      
      Serial.print(F("failed, rc="));
      Serial.print(mqttClient.state());
      Serial.println(F(" try again in 5 seconds"));
      
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
