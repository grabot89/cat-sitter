#include "arduino_secrets.h"
#include "thingProperties.h"   //AUTOMATICALLY GENERATED to read your added Variables
#include <Arduino_MKRIoTCarrier.h>
#include "pitches.h"
MKRIoTCarrier carrier;
int pir = A0;
bool pirState = LOW;
//bool alarm = false;
//string message = "";
float temperature = 0.0;


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
  ArduinoCloud.begin(ArduinoIoTPreferredConnection);  // AUTOMATICALLY GENERATED Connect to Arduino IoT Cloud
  setDebugMessageLevel(4); // AUTOMATICALLY GENERATED set the amount of details in debug message 0-4
  ArduinoCloud.printDebugInfo(); // AUTOMATICALLY GENERATED print out the debug message

  while (ArduinoCloud.connected() != 1) {
    ArduinoCloud.update();
    delay(500);
  }
  delay(2000);

  pinMode(pir, INPUT);
  carrier.withCase();
  carrier.begin();
}

void loop() {
  ArduinoCloud.update(); // AUTOMATICALLY GENERATED keep updating the Cloud with new data
  temperature = carrier.Env.readTemperature();
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.println(" °C");
  //reading the status of the PIR sensor
  //pirState = digitalRead(pir);
  pirState = digitalRead(pir);
  Serial.println(pirState);
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
    // iterate over the notes of the melody:
    // for (int thisNote = 0; thisNote < 8; thisNote++) {

    //   // to calculate the note duration, take one second divided by the note type.
    //   //e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
    //   int noteDuration = 1000 / noteDurations[thisNote];
    //   carrier.Buzzer.sound(melody[thisNote]);
    //   delay(noteDuration);
    //   // to distinguish the notes, set a minimum time between them.
    //   // the note's duration + 30% seems to work well:
    //   int pauseBetweenNotes = noteDuration * 1.30;
    //   delay(pauseBetweenNotes);
    //   // stop the tone playing:
    //   carrier.Buzzer.noSound();
    // }
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
}

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