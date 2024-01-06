#include "arduino_secrets.h"
#include "thingProperties.h"   //AUTOMATICALLY GENERATED to read your added Variables
#include "WiFiS3.h"
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "DHT.h"// Include library for DHT11 sensor for temperature and humidity sensor
#include <WiFiUdp.h>

#define DHTPIN 8
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

StaticJsonDocument<200> doc;
int status = WL_IDLE_STATUS;
const char* mqttServer = "broker.emqx.io";
const int mqttPort = 1883;

// Variable to save current epoch time
unsigned long epochTime;

unsigned int localPort = 2390;      // local port to listen for UDP packets
IPAddress timeServer(162, 159, 200, 123); // pool.ntp.org NTP server
const int NTP_PACKET_SIZE = 48; // NTP timestamp is in the first 48 bytes of the message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets
// A UDP instance to let us send and receive packets over UDP
WiFiUDP Udp;

WiFiClient wifiClient;
PubSubClient client(wifiClient);
float temperature = 0.0;
float humidity = 0.0;
unsigned long lastMillis;

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
  
  dht.begin(); // initialize the sensor
  Udp.begin(localPort);
  client.setServer(mqttServer, mqttPort);
  
  delay(2000);

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
  //reading the status of the Ultrasound sensor
  
  // wait ten seconds before asking for the time again
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
    if (client.connect("ArduinoUnoR4")) {
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

unsigned long getCurrentTime() {
  // WIFIS3 getTime() doesn't work, bug here: https://github.com/arduino/ArduinoCore-renesas/issues/206
  sendNTPpacket(timeServer); // send an NTP packet to a time server
  // wait to see if a reply is available
  delay(1000);
  if (Udp.parsePacket()) {
    Serial.println("packet received");
    // We've received a packet, read the data from it
    Udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer

    //the timestamp starts at byte 40 of the received packet and is four bytes,
    // or two words, long. First, extract the two words:

    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
    // combine the four bytes (two words) into a long integer
    // this is NTP time (seconds since Jan 1 1900):
    unsigned long secsSince1900 = highWord << 16 | lowWord;
    Serial.print("Seconds since Jan 1 1900 = ");
    Serial.println(secsSince1900);

    // now convert NTP time into everyday time:
    Serial.print("Unix time = ");
    // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
    const unsigned long seventyYears = 2208988800UL;
    // subtract seventy years:
    unsigned long epoch = secsSince1900 - seventyYears;
    // print Unix time:
    Serial.println(epoch);
    Serial.print("Time");
    Serial.println(epoch);
    Serial.println(epochTime);
    return epoch;
  }
}

// send an NTP request to the time server at the given address
unsigned long sendNTPpacket(IPAddress& address) {
  //Serial.println("1");
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  //Serial.println("2");
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  //Serial.println("3");

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  //Serial.println("4");
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  //Serial.println("5");
  Udp.endPacket();
  //Serial.println("6");
}


void publishMeasurements() {
  
  temperature = dht.readTemperature();// Read temperature
  humidity = dht.readHumidity();// Read humidity
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.println(" °C");
  
  Serial.print("Humidity: ");
  Serial.println(humidity);
  
  epochTime = getCurrentTime();
  Serial.println(epochTime);

  char message[120];
  doc["deviceID"]="Hallway";
  doc["temp"]=temperature;
  doc["humidity"]=humidity;
  doc["timestamp"]=epochTime;
  doc["encounter"]="false";
  serializeJson(doc,message);
  Serial.println(message);
  // Your MQTT publish code goes here
  client.publish("/cat-sitter/measurements", message);
}

// Problems connecting second PIR or ultrasonic at same time as DHT, won't be able to check litter box usage
// void catEncounter() {
  
//   temperature = dht.readTemperature();// Read temperature
//   humidity = dht.readHumidity();// Read humidity
//   Serial.print("Temperature: ");
//   Serial.print(temperature);
//   Serial.println(" °C");
  
//   Serial.print("Humidity: ");
//   Serial.println(humidity);
  
//   epochTime = getCurrentTime();
//   Serial.println(epochTime);
  
//   char message[120];
//   doc["deviceID"]="Hallway";
//   doc["temp"]=temperature;
//   doc["humidity"]=humidity;
//   doc["timestamp"]=epochTime;
//   doc["encounter"]="true";
//   serializeJson(doc,message);
//   Serial.println(message);
//   // Your MQTT publish code goes here
//   client.publish("/cat-sitter/measurements", message);
// }