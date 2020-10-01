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

#define PIN_RX 10
#define PIN_TX 11

// Initialize client objects

#ifndef HAVE_HWSERIAL1
#include "SoftwareSerial.h"
SoftwareSerial soft(PIN_RX, PIN_TX); // RX, TX
#endif

WiFiEspClient espClient;
PubSubClient mqttClient(espClient);

char buffer[256];
const char* clientID = "ArduinoESP8266";

void setup() {
  Serial.begin(9600); // initialize serial
  soft.begin(9600);   // initialize serial for ESP module

  Wifi_Setup(); // wifi ESP8266 setup
  delay(1000);
  Mqtt_Setup(); // mqtt (mosquitto) setup

  delay(5000);  // Delay to allow interface to autoconfigure
  
  const int capacity = JSON_OBJECT_SIZE(7);
  StaticJsonDocument<capacity> payload;

  payload["~"] = "homeassistant/light/office";
  payload["name"] = "Office Light";
  payload["unique_id"] = "office_light_1";
  payload["cmd_t"] = "~/set";
  payload["stat_t"] = "~/state";
  payload["schema"] = "json";
  payload["brightness"] = true;

  serializeJson(payload, buffer);

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
    Serial.println("WiFi module not present");
    // stop
    while (true);
  }

  // Connect to WiFi network
  while ( status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network
    status = WiFi.begin(ssid, pass);
  }

  // Wifi is connected
  Serial.print("Connected to: ");
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
  //Serial.println(state);
  
  memset(buffer, 0, sizeof(buffer)); // clear data buffer
  
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
    serializeJsonPretty(mqttPayload, buffer);

    Serial.print("Print mqtt data buffer: ");
    Serial.println(buffer);
    Serial.print("Publishing on topic: ");
    Serial.println(LED_STRIP_STATE);
    mqttClient.publish(LED_STRIP_STATE, buffer);
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
    
    Serial.print("Attempting MQTT connection...");  // Attempt to connect, just a name to identify the client
    
    if (mqttClient.connect(clientID)) {
      Serial.println("connected to mqtt Server");
      Serial.print("Sending Auto Discovery: ");
      Serial.println(LED_STRIP_CONFIG);
      mqttClient.publish(LED_STRIP_CONFIG, buffer);  // Once connected, publish an announcement...
      Serial.print("Listening on topic: ");
      Serial.println(LED_STRIP_SET);
      mqttClient.subscribe(LED_STRIP_SET);  // ... and resubscribe
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
