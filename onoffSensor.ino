#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <ESP8266WiFi.h>

#include <PubSubClient.h>
#include <Msgflo.h>


struct Config {
  const String role = "onoffsensor";

  const String wifiSsid = "mySsid";
  const String wifiPassword = "myPassword";

  const char *mqttHost = "mqtt.bitraf.no";
  const int mqttPort = 1883;

  //const char *mqttUsername = "myuser";
  //const char *mqttPassword = "mypassword";
} cfg;

WiFiClient wifiClient;
PubSubClient mqttClient;
msgflo::Engine *engine;
msgflo::OutPort *onoffPort;
long nextOnOffCheck = 0;

auto participant = msgflo::Participant("iot/onoffsensor", cfg.role);

double Voltage = 0;
int limit = 512; // Change to a suitable value depending on your power sensor
const int sensorIn = A0;
const int sendStatusDelay = 30000; //Send status every 30000 ms when there is no change
boolean lastStatus = true;
bool onoff = false;
long nextOnOffSend = 0;

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println();
  Serial.println();
  Serial.println();

  Serial.printf("Configuring wifi: %s\r\n", cfg.wifiSsid.c_str());
  WiFi.begin(cfg.wifiSsid.c_str(), cfg.wifiPassword.c_str());
  
  mqttClient.setServer(cfg.mqttHost, cfg.mqttPort);
  mqttClient.setClient(wifiClient);

  String clientId = cfg.role;
  clientId += WiFi.macAddress();

  participant.icon = "bolt";
  engine = msgflo::pubsub::createPubSubClientEngine(participant, &mqttClient, clientId.c_str());//, cfg.mqttUsername, cfg.mqttPassword);

  onoffPort = engine->addOutPort("onoff", "boolean", "public/" + cfg.role + "/onoff");
}

void loop() {
  static bool connected = false;

  if (WiFi.status() == WL_CONNECTED) {
    if (!connected) {
      Serial.printf("Wifi connected: ip=%s\r\n", WiFi.localIP().toString().c_str());
    }
    connected = true;
    engine->loop();
  } else {
    if (connected) {
      connected = false;
      Serial.println("Lost wifi connection.");
    }
  }

  //Check the state every 1000ms
  if (millis() > nextOnOffCheck)
  {
    onoff = checkOnOff();
    nextOnOffCheck += 1000;
  }

  if ((millis() > nextOnOffSend) || (onoff != lastStatus)) 
  {
    //Send status on MQTT
    onoffPort->send(onoff ? "true" : "false");
    lastStatus = onoff;
    nextOnOffSend = millis() + sendStatusDelay;
  }
}

boolean checkOnOff() //Put your code for reading the power sensor here, return true or false
{
  Voltage = getVoltage();
  if (Voltage > limit)
  {
    return true;
  }
  else
  {
    return false;
  }
}


int getVoltage() //Return maximum voltage from ACS712 power sensor, sample for 1 sec
{
  int readValue;             //value read from the sensor
  int maxValue = 0;          // store max value here
  
   uint32_t start_time = millis();
   while((millis()-start_time) < 60) //sample for 60 ms (3 periods)
   {
       readValue = analogRead(sensorIn);
       // see if you have a new maxValue
       if (readValue > maxValue) 
       {
           /*record the maximum sensor value*/
           maxValue = readValue;
       }
   }
   return maxValue;
 }
