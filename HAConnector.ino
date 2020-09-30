#include <WiFiEsp.h>
#include <WiFiEspClient.h>
#include <WiFiEspUdp.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

char ssid[] = "Vipernet";           // network SSID
char pass[] = "nadlor290228";       // network password
int status = WL_IDLE_STATUS;        // Wifi radio's status
IPAddress MQTT_SERVER(192, 168, 2, 221); // MQTT server address

//const char * LED_STRIP_STATE2 = "homeassistant/light/office/state";
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
  Mqtt_Setup(); // mqtt (mosquitto) setup

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
  String response;
  
  memset(buffer, 0, sizeof(buffer));
    
  for (int i = 0; i < length; i++) {
    response += (char)payload[i];
  }

  Serial.print("#####################");
  Serial.print(response);
  Serial.println("#####################");
  
  /*StaticJsonDocument<256> doc2;
  deserializeJson(doc2, payload, length);
  JsonObject root = doc2.as<JsonObject>();
  
  Serial.println("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  
  //for (int i=0;i<length;i++) {
  //  Serial.print((char)payload[i]);
  for (JsonPair p : root) {
    Serial.println(p.key().c_str()); // is a JsonString
    Serial.println(p.value().as<char*>()); // is a JsonVariant
  }*/

  // action for topic
  if(strcmp(topic, LED_STRIP_SET) == 0){

    const int capacity = JSON_OBJECT_SIZE(2);
    StaticJsonDocument<capacity> mqttPayload;

    mqttPayload["state"] = "ON";
    mqttPayload["brightness"] = 255;

    serializeJson(mqttPayload, buffer);

    Serial.print("Print mqtt data buffer: ");
    Serial.println(buffer);

    mqttClient.publish("homeassistant/light/office/state", buffer);
  }

  
  Serial.println();
}

void loop() {
  if (!mqttClient.connected()) {
    reconnect();
  }
  mqttClient.loop();

  // mqqt test
  //client.publish(out_topic, String(random(2)).c_str(), true);
  //delay(1000);
  //client.subscribe(in_topic);
  //delay(1000);
  
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
