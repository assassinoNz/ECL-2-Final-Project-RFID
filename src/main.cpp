#include <Arduino.h>
#include <LiquidCrystal.h>
#include <Keypad.h>

#define DB_SIZE 5

//DEFINE DATA STRUCTURES
class TagDatum {
  public:
    String id = "";
    String name = "";
    bool isActive = false;
    bool isMaster = false;

    TagDatum() {}
    TagDatum(String id, String name, bool isActive, bool isMaster) {
      this->id = id;
      this->name = name;
      this->isActive = isActive;
      this->isMaster = isMaster;
    }
};

//DEFINE GLOBAL VARIABLES
char hexKeys[4][3] = {
    {'1', '2', '3'},
    {'4', '5', '6'},
    {'7', '8', '9'},
    {'*', '0', '#'}
  };
byte rowPins[4] = {A5, A4, A3, A2}; 
byte colPins[3] = {8, 9, 10}; 
Keypad keypad = Keypad(makeKeymap(hexKeys), rowPins, colPins, 4, 3);
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);
String id = "";
int count = 0;
TagDatum tagData[DB_SIZE]; //NOTE: Index 0 must always contain an invalid tagDatum

//HELPER FUNCTIONS
TagDatum getDatumById(String id) {
  for (int i = 0; i < DB_SIZE; i++) {
      if (tagData[i].id == id) {
        return tagData[i];
      }
  }
  return tagData[0];
}

void setup() {
  //SETUP LIBRARIES
  Serial.begin(9600);
  Serial.println("Starting system...");

  lcd.begin(40, 2);
  lcd.print("Please produce your RFID tag");

  //INITIALIZE DATABASE
  tagData[1] = TagDatum("AAAAAAAAAAAA", "Dias", true, true);
  tagData[2] = TagDatum("BBBBBBBBBBBB", "Nirmal", true, false);
  tagData[3] = TagDatum("CCCCCCCCCCCC", "Sandun", true, false);
}

void loop() {
  while(Serial.available()>0) {
    id += (char) Serial.read();
    count++;
    if(count == 12) {
        Serial.println("Tag detected with id " + id);
        TagDatum tagDatum = getDatumById(id);
      
        if(tagDatum.isActive) {
          if(tagDatum.isMaster) {
            lcd.clear();
            lcd.print("Hello " + tagDatum.name + "! What would you like to do?");
            lcd.setCursor(0, 1);
            lcd.print("1. Update user; 2. Change masster");

            char key = keypad.waitForKey();
            Serial.println(key);
          } else {
            lcd.clear();
            lcd.print("Access granted");
            lcd.setCursor(0, 1);
            lcd.print("Hello " + tagDatum.name + "!");
          }
        } else {
          lcd.clear();
          lcd.print("Access denied");
        }
      }
  }

  count = 0;
  id="";

  delay(500);
}