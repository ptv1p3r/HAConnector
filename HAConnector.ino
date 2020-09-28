#include <WiFiEsp.h>
#include <WiFiEspClient.h>
#include <WiFiEspUdp.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

char ssid[] = "Vipernet";           // network SSID
char pass[] = "nadlor290228";       // network password
int status = WL_IDLE_STATUS;        // Wifi radio's status
IPAddress server(192, 168, 2, 221); // MQTT server address

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
PubSubClient client(espClient);

//unsigned long time;
//String sPayload;
//char* cPayload;
char buffer[256];
const char* clientID = "ArduinoESP8266";

void setup() {
  Serial.begin(9600); // initialize serial
  soft.begin(9600);   // initialize serial for ESP module
  WiFi.init(&soft);   // initialize ESP module

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
  
  // check for the presence of the module
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi module not present");
    // stop
    while (true);
  }

  // Connect to WiFi network
  while ( status != WL_CONNECTED) {
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network
    status = WiFi.begin(ssid, pass);
  }

  // Wifi is connected
  Serial.println("You're connected to the network");

  //connect to MQTT server
  client.setServer(server, 1883);
  client.setCallback(callback);
}

void Wifi_Setup(){
  
}


//print any message received for subscribed topic
void callback(char* topic, byte* payload, unsigned int length) {

  buffer[0] = '\0'; // elimna o conteudo do array
  //buffer[0] = (char)0;
  
  Serial.print((char)payload);
  
  StaticJsonDocument<256> doc2;
  deserializeJson(doc2, payload, length);
  JsonObject root = doc2.as<JsonObject>();
  
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  //for (int i=0;i<length;i++) {
  //  Serial.print((char)payload[i]);
  for (JsonPair p : root) {
    Serial.println(p.key().c_str()); // is a JsonString
    Serial.println(p.value().as<char*>()); // is a JsonVariant
  }

  // action for topic
  if(strcmp(topic, LED_STRIP_SET) == 0){
    
    const int capacity = JSON_OBJECT_SIZE(2);
    StaticJsonDocument<capacity> payload2;

    payload2["state"] = "ON";
    payload2["brightness"] = 255;

    serializeJson(payload2, buffer);
    //Serial.print(buffer);
    client.publish(LED_STRIP_STATE,buffer);

  }

  
  Serial.println();
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // mqqt test
  //client.publish(out_topic, String(random(2)).c_str(), true);
  //delay(1000);
  //client.subscribe(in_topic);
  //delay(1000);
  
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    
    Serial.print("Attempting MQTT connection...");  // Attempt to connect, just a name to identify the client
    
    if (client.connect(clientID)) {
      Serial.println("connected");
      client.publish(LED_STRIP_CONFIG,buffer);  // Once connected, publish an announcement...
      client.subscribe(LED_STRIP_SET);  // ... and resubscribe
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
