#include "arduino_secrets.h"
#include "thingProperties.h"   //AUTOMATICALLY GENERATED to read your added Variables
#include <Arduino_MKRIoTCarrier.h>
#include <WiFiNINA.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "pitches.h"

MKRIoTCarrier carrier;
StaticJsonDocument<200> doc;
int status = WL_IDLE_STATUS;
const char* mqttServer = "broker.emqx.io";
const int mqttPort = 1883;

// Variable to save current epoch time
unsigned long epochTime;

WiFiClient wifiClient;
PubSubClient client(wifiClient);
int pir = A0;
bool pirState = LOW;
//bool alarm = false;
//string message = "";
float temperature = 0.0;
float humidity = 0.0;
unsigned long lastMillis;

//Colors
uint32_t colorGreen = carrier.leds.Color(0, 50, 0);  //RED
uint32_t colorRed = carrier.leds.Color(50, 0, 0);    //GREEN

int melody[] = {
  NOTE_C4, NOTE_G3, NOTE_G3, NOTE_A3, NOTE_G3, 0, NOTE_B3, NOTE_C4
};

// note durations: 4 = quarter note, 8 = eighth note, etc.:
int noteDurations[] = {
  4, 8, 8, 4, 4, 4, 4, 4
};

void setup() {

  Serial.begin(9600);  //AUTOMATICALLY GENERATED to handle the serial monitor
  initProperties();  // AUTOMATICALLY GENERATED Defined in thingProperties.h
  setupWiFi();
  ArduinoCloud.begin(ArduinoIoTPreferredConnection);  // AUTOMATICALLY GENERATED Connect to Arduino IoT Cloud
  setDebugMessageLevel(4); // AUTOMATICALLY GENERATED set the amount of details in debug message 0-4
  ArduinoCloud.printDebugInfo(); // AUTOMATICALLY GENERATED print out the debug message

  while (ArduinoCloud.connected() != 1) {
    ArduinoCloud.update();
    delay(500);
  }
  
  client.setServer(mqttServer, mqttPort);
  
  delay(2000);
  pinMode(pir, INPUT);

  carrier.noCase();
  carrier.begin();
}

void loop() {
  ArduinoCloud.update(); // AUTOMATICALLY GENERATED keep updating the Cloud with new data
  //Push events every 5 mins
  if (millis() - lastMillis >= 5*60*1000UL) 
  {
   lastMillis = millis();  //get ready for the next iteration
   publishMeasurements();
  }
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  //reading the status of the PIR sensor
  pirState = digitalRead(pir);
  Serial.println("========");
  Serial.print("PIR State is: ");
  Serial.println(pirState);
  Serial.print("Alarm: ");
  Serial.println(alarm);
  //checking if the PIR sensor has detected movement
  if (pirState == HIGH || alarm==1) {
    message = "Warning! Movement detected, Go to Dashboard to disable the alarm.";
    Serial.println(message);
    carrier.display.fillScreen(ST77XX_BLACK);
    carrier.display.setCursor(30, 100);
    carrier.display.setTextSize(2);
    carrier.display.println(message);
    carrier.leds.fill((colorRed), 0, 5);
    carrier.leds.show();
    alarm=1;
    catEncounter();
    // iterate over the notes of the melody:
    for (int thisNote = 0; thisNote < 8; thisNote++) {

      // to calculate the note duration, take one second divided by the note type.
      //e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
      int noteDuration = 1000 / noteDurations[thisNote];
      carrier.Buzzer.sound(melody[thisNote]);
      delay(noteDuration);
      // to distinguish the notes, set a minimum time between them.
      // the note's duration + 30% seems to work well:
      int pauseBetweenNotes = noteDuration * 1.30;
      delay(pauseBetweenNotes);
      // stop the tone playing:
      carrier.Buzzer.noSound();
    }
  } else {
    message = "No movement detected. Everything is good here.";
    Serial.println(message);
    carrier.display.fillScreen(ST77XX_BLACK);
    carrier.display.setCursor(30, 100);
    carrier.display.setTextSize(2);
    carrier.display.println(message);
    carrier.leds.fill((colorGreen), 0, 5);
    carrier.leds.show();
    alarm=0;
  }
  delay(200);
  // PIR is over sensitive, change PIR state back to 0 manually
  pirState = LOW;
  alarm = 0;
}

void setupWiFi() {
 // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    // don't continue
    while (true);
  }

  // attempt to connect to WiFi network:
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(SSID);
    // Connect to WPA/WPA2 network:
    status = WiFi.begin(SSID, PASS);

    // wait 10 seconds for connection:
    delay(5000);
  }
  // you're connected now, so print out the data:
  Serial.println("You're connected to the network");
}

void reconnect() {
  while (!client.connected()) {
    if (client.connect("ArduinoMKR1010")) {
      Serial.println("Connected to MQTT Broker!");
      Serial.print("Connection Result=");
      Serial.println(client.state());
    } else {
      Serial.print("failed, Connection Result=");
      Serial.println(client.state());
      delay(5000);
    }
  }
}

void publishMeasurements() {
  
  temperature = carrier.Env.readTemperature();
  humidity = carrier.Env.readHumidity();
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.println(" °C");
  
  Serial.print("Humidity: ");
  Serial.println(humidity);
  
  epochTime = WiFi.getTime();
  Serial.println(epochTime);

  char message[120];
  doc["deviceID"]="Fireplace";
  doc["temp"]=temperature;
  doc["humidity"]=humidity;
  doc["timestamp"]=epochTime;
  doc["forbidden"]="false";
  serializeJson(doc,message);
  Serial.println(message);
  // Your MQTT publish code goes here
  client.publish("/cat-sitter/measurements", message);
}

void catEncounter() {
  
  temperature = carrier.Env.readTemperature();
  humidity = carrier.Env.readHumidity();
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.println(" °C");
  
  Serial.print("Humidity: ");
  Serial.println(humidity);
  
  epochTime = WiFi.getTime();
  Serial.println(epochTime);
  
  char message[120];
  doc["deviceID"]="Fireplace";
  doc["temp"]=temperature;
  doc["humidity"]=humidity;
  doc["timestamp"]=epochTime;
  doc["forbidden"]="true";
  serializeJson(doc,message);
  Serial.println(message);
  // Your MQTT publish code goes here
  client.publish("/cat-sitter/measurements", message);
  
  //Send push notification to phone
  trigger = true;
}

// These methods are used to change the alarm and message through the Arduino Cloud portal
void onMessageChange()  {
  // Add your code here to act upon Message change
}
/*
  Since Alarm is READ_WRITE variable, onAlarmChange() is
  executed every time a new value is received from IoT Cloud.
*/
void onAlarmChange()  {
  // Add your code here to act upon Alarm change
}

void onTriggerChange()  {
    trigger = false;
}