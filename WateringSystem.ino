#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <Ticker.h> //for LED status
#include <WiFiManager.h>
#include <String.h> 
#include "ThingSpeak.h"
#include <PubSubClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
//MQTT - mosquitto

const long utcOffsetInSeconds = 25200;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;

const char* mqtt_server = "broker.mqtt-dashboard.com";

unsigned long myChannelNumber = 1260973;
const char * myWriteAPIKey = "H3LR4F1ZRWMPOSF0";
const char * myReadAPIKey = "MT8FRNAUN4GJL8F5";

//Ultra Sonic Sensor
const int trig_pin = 5;
const int echo_pin = 4;

//Light Sensor
const int light_sensor = 0;

//LED for tree
const int led_tree = 2;

//LED for remaining water
const int led_red = 14;
const int led_yellow = 12;
const int led_green = 13;

//Soil Moisture Sensor
const int soil_moisture = A0;
int Soil_Value;
int limit = 300;

//Relay
int relay_pin = 15;

float getDistance(){
  float duration, distanceCm;

  digitalWrite(trig_pin, LOW);
  delayMicroseconds(2);
  digitalWrite(trig_pin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig_pin, LOW);

  duration = pulseIn(echo_pin, HIGH);

  distanceCm = duration * 0.034 / 2;
  
  return distanceCm;
}

//gets called when WiFiManager enters configuration mode
void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      client.subscribe("Pump_water");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
    String message;
    for (int i = 0; i < length; i++) {
      message += (char)payload[i];
    }
    Serial.println(topic);
    Serial.println(message);  
    if (digitalRead(relay_pin) == 1)
    {
       digitalWrite(relay_pin, LOW);
       delay(2000);
       digitalWrite(relay_pin, HIGH);
       delay(2000);
    }
    else
    {
      digitalWrite(relay_pin, HIGH);
      delay(2000);
    }
      
}
  

void setup() {
  // put your setup code here, to run once:
//  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);
  // Ultra Sonic Sensor
  pinMode(trig_pin, OUTPUT);
  pinMode(echo_pin, INPUT);
  // Led for tree
  pinMode(led_tree, OUTPUT);
  // Light Sensor
  pinMode(light_sensor, INPUT);
  // Relay 
  pinMode(relay_pin, OUTPUT);
  //led for showing remain water
  pinMode(led_red, OUTPUT);
  pinMode(led_yellow, OUTPUT);
  pinMode(led_green, OUTPUT);
  
  ThingSpeak.begin(espClient);
  timeClient.begin();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  
  WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP
  // put your setup code here, to run once:

  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wm;
  //reset settings - for testing
  //wm.resetSettings();

  //set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
  wm.setAPCallback(configModeCallback);

  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  if (!wm.autoConnect()) {
    Serial.println("failed to connect and hit timeout");
    //reset and try again, or maybe put it to deep sleep
    ESP.restart();
    delay(1000);
  }

  //if you get here you have connected to the WiFi
  Serial.println("connected...yeey :)");
}

float waterRemaining(int distance) {
  float r = 3.7;
  float h = 13;
  float V = 3.14*r*r*(h - distance);
  return V;
}

void loop() {
  // put your main code here, to run repeatedly:
  float distance = getDistance();
  float light = digitalRead(light_sensor);
  String light_value;
  if (light == 1){
    light_value = "The light is enough for tree";
  }
  else {
    light_value = "The light is not enough for tree, turn on the LED";
  }
  float led_status = digitalRead(led_tree);
  float soil = analogRead(soil_moisture);
  float soil_percent = map(soil, 0, 1023, 0, 100);
  float water_remain = waterRemaining(distance);
  
  //Write data to field
//  ThingSpeak.setField(1, distance);
//  ThingSpeak.setField(2, light);
//  
//  int returncode = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
//  // Check return Code
//  if (returncode == 200) {
//    Serial.println("Channel update sucessful.");
//  }
//  else {
//    Serial.println("Problem updating channel. HTTP error code " + String(returncode));
//  }
  

  // LED for showing remaining water
  
  float h_box = 13;
  float res = h_box - distance;
  if (res < 0)
    res = 0;

  if (res < 5)
  {
    digitalWrite(led_red, HIGH);
    digitalWrite(led_yellow, LOW);
    digitalWrite(led_green, LOW);
  }
  else if (res < 10)
  {
    digitalWrite(led_yellow, HIGH);
    digitalWrite(led_green, LOW);
    digitalWrite(led_red, LOW);
  }
  else 
  {
    digitalWrite(led_green, HIGH);
    digitalWrite(led_red, LOW);
    digitalWrite(led_yellow, LOW);
  }
  // Relay pump water
  if (soil_percent > 60)
  {
    digitalWrite(relay_pin, LOW);
    delay(2000);
    digitalWrite(relay_pin, HIGH);
    delay(2000);
  }
  else 
  {
    digitalWrite(relay_pin, HIGH);
    delay(2000);
  }
    
 
  timeClient.update();

  
  if (digitalRead(light_sensor) == 1 && 4 < timeClient.getHours() < 18)
  {
    digitalWrite(led_tree, HIGH);
  }
  else if (digitalRead(light_sensor) == 0)
  {
    digitalWrite(led_tree, LOW);
  }
  
   if (!client.connected()) {
    reconnect();
  }
  client.loop();
  client.publish("soil_moisture", String(soil_percent).c_str());  
  client.publish("water_remain", String(water_remain).c_str());
  client.publish("light_sensor", String(light_value).c_str()); 
  Serial.println(soil_percent);
  Serial.println(digitalRead(relay_pin));
  Serial.println(water_remain);
  delay(2000);
  
}
