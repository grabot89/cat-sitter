
#include <LiquidCrystal.h>
#include <Keypad.h>
#include <Servo.h>
//#include <WiFiNINA.h>
//#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "SafeState.h"
#include "icons.h"

/* Locking mechanism definitions */
#define SERVO_PIN 6
#define SERVO_LOCK_POS   20
#define SERVO_UNLOCK_POS 90
Servo lockServo;

// This project is based off an existing Wokwi project that uses a keypad to lock/unlock a safe, I adapted it for the cat sitter and to send events to queue
// to be shown in web-app but physical keypad module would not work for me it would continuously send numbers to the Arduino so it was useless.

//I used wokwi to show the working keypad code as part of my whole solution but could not simulate a wifi connection or send messages to queue like I did for hallway and fireplace boards.

StaticJsonDocument<200> doc;
char ssid[] = "Wokwi-GUEST";        // your network SSID (name)
char pass[] = "";    // your network password (use for WPA, or use as key for WEP)
//int status = WL_IDLE_STATUS;
//const char* mqttServer = "broker.emqx.io";  // Replace with your MQTT Broker address
//const int mqttPort = 1883;                  // typical MQTT port

//WiFiClient wifiClient;
//PubSubClient client(wifiClient);

/* Display */
LiquidCrystal lcd(12, 11, 10, 9, 8, 7);

/* Keypad setup */
const byte KEYPAD_ROWS = 4;
const byte KEYPAD_COLS = 4;
byte rowPins[KEYPAD_ROWS] = {5, 4, 3, 2};
byte colPins[KEYPAD_COLS] = {A3, A2, A1, A0};
char keys[KEYPAD_ROWS][KEYPAD_COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, KEYPAD_ROWS, KEYPAD_COLS);

/* SafeState stores the secret code in EEPROM */
SafeState safeState;

void lock() {
  lockServo.write(SERVO_LOCK_POS);
  safeState.lock();
}

void unlock() {
  lockServo.write(SERVO_UNLOCK_POS);
}

// String getTime() {
//   while (!!!wifiClient.connect("google.com", 80)) {
//     Serial.println("connection failed, retrying...");
//   }

//   wifiClient.print("HEAD / HTTP/1.1\r\n\r\n");
//   while(!!!wifiClient.available()) {
//      yield();
//   }

//   while(wifiClient.available()){
//     if (wifiClient.read() == '\n') {   
//       if (wifiClient.read() == 'D') {   
//         if (wifiClient.read() == 'a') {   
//           if (wifiClient.read() == 't') {   
//             if (wifiClient.read() == 'e') {   
//               if (wifiClient.read() == ':') {   
//                 wifiClient.read();
//                 String theDate = wifiClient.readStringUntil('\r');
//                 wifiClient.stop();
//                 return theDate;
//               }
//             }
//           }
//         }
//       }
//     }
//   }
// }

void showStartupMessage() {
  lcd.setCursor(4, 0);
  lcd.print("Welcome!");
  delay(1000);

  lcd.setCursor(0, 2);
  String message = "Cat Sitter";
  for (byte i = 0; i < message.length(); i++) {
    lcd.print(message[i]);
    delay(100);
  }
  delay(500);
}

String inputSecretCode() {
  lcd.setCursor(5, 1);
  lcd.print("[____]");
  lcd.setCursor(6, 1);
  String result = "";
  while (result.length() < 4) {
    char key = keypad.getKey();
    if (key >= '0' && key <= '9') {
      lcd.print('*');
      result += key;
    }
  }
  return result;
}

void showWaitScreen(int delayMillis) {
  lcd.setCursor(2, 1);
  lcd.print("[..........]");
  lcd.setCursor(3, 1);
  for (byte i = 0; i < 10; i++) {
    delay(delayMillis);
    lcd.print("=");
  }
}

bool setNewCode() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Enter new code:");
  String newCode = inputSecretCode();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Confirm new code");
  String confirmCode = inputSecretCode();

  if (newCode.equals(confirmCode)) {
    safeState.setCode(newCode);
    return true;
  } else {
    lcd.clear();
    lcd.setCursor(1, 0);
    lcd.print("Code mismatch");
    lcd.setCursor(0, 1);
    lcd.print("Safe not locked!");
    delay(2000);
    return false;
  }
}

void showUnlockMessage() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.write(ICON_UNLOCKED_CHAR);
  lcd.setCursor(4, 0);
  lcd.print("Unlocked!");
  lcd.setCursor(15, 0);
  lcd.write(ICON_UNLOCKED_CHAR);
  delay(1000);
}

void safeUnlockedLogic() {
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.write(ICON_UNLOCKED_CHAR);
  lcd.setCursor(2, 0);
  lcd.print(" # to lock");
  lcd.setCursor(15, 0);
  lcd.write(ICON_UNLOCKED_CHAR);

  bool newCodeNeeded = true;

  if (safeState.hasCode()) {
    lcd.setCursor(0, 1);
    lcd.print("  A = new code");
    newCodeNeeded = false;
  }

  auto key = keypad.getKey();
  while (key != 'A' && key != '#') {
    key = keypad.getKey();
  }

  bool readyToLock = true;
  if (key == 'A' || newCodeNeeded) {
    readyToLock = setNewCode();
  }

  if (readyToLock) {
    lcd.clear();
    lcd.setCursor(5, 0);
    lcd.write(ICON_UNLOCKED_CHAR);
    lcd.print(" ");
    lcd.write(ICON_RIGHT_ARROW);
    lcd.print(" ");
    lcd.write(ICON_LOCKED_CHAR);

    safeState.lock();
    lock();
    showWaitScreen(100);
  }
}

void safeLockedLogic() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.write(ICON_LOCKED_CHAR);
  lcd.print(" Cats protected! ");
  lcd.write(ICON_LOCKED_CHAR);

  // doc["type"]="Entry";
  // doc["timestanp"]=getTime();

  String userCode = inputSecretCode();
  bool unlockedSuccessfully = safeState.unlock(userCode);
  showWaitScreen(200);

  if (unlockedSuccessfully) {
    showUnlockMessage();
    unlock();
    //doc["attempt"]="Success";
  } else {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Access Denied!");
    //doc["attempt"]="Failure";
    showWaitScreen(1000);
  }

  // serializeJson(doc,message);
  // client.publish("/cat-sitter/measurements", message);
}

void setup() {
  lcd.begin(16, 2);
  init_icons(lcd);

  lockServo.attach(SERVO_PIN);

  /* Make sure the physical lock is sync with the EEPROM state */
  Serial.begin(115200);
  if (safeState.locked()) {
    lock();
  } else {
    unlock();
  }

  // setupWiFi();
  // client.setServer(mqttServer, mqttPort);

  showStartupMessage();
}

void loop() {
  if (safeState.locked()) {
    safeLockedLogic();
  } else {
    safeUnlockedLogic();
  }
}

// void setupWiFi() {
//  // check for the WiFi module:
//   if (WiFi.status() == WL_NO_MODULE) {
//     Serial.println("Communication with WiFi module failed!");
//     // don't continue
//     while (true);
//   }

//   // attempt to connect to WiFi network:
//   while (status != WL_CONNECTED) {
//     Serial.print("Attempting to connect to WPA SSID: ");
//     Serial.println(ssid);
//     // Connect to WPA/WPA2 network:
//     status = WiFi.begin(ssid, pass);

//     // wait 10 seconds for connection:
//     delay(5000);
//   }
//   // you're connected now, so print out the data:
//   Serial.println("You're connected to the network");
// }
