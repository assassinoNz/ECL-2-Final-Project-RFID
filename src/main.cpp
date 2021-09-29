#include <Arduino.h>
#include <LiquidCrystal.h>
#include <Keypad.h>
#include <Servo.h>

#define DOOR A3
#define RED_LED A0
#define GREEN_LED A1
#define BUZZER 10

#define DB_SIZE 5
#define DELAY 2000
#define OPTIONS_MASTER "1.Tag management *.Quit"
#define OPTIONS_UNREGISTERED_TAG "1.Register *.Quit"
#define OPTIONS_REGISTERED_TAG "1.Unregister 2.Edit tag *.Quit"
#define OPTIONS_EDIT_TAG_ACTIVE "1.Deactivate 2.Make master *.Quit"
#define OPTIONS_EDIT_TAG_INACTIVE "1.Activate 2.Make master *.Quit"
#define OPTIONS_AWAIT_TAG "*.Quit"

//DEFINE DATA STRUCTURES
class TagDatum {
public:
    String id = "";
    bool isActive = false;
    bool isMaster = false;

    TagDatum() {}
    TagDatum(String id, bool isActive, bool isMaster) {
        this->id = id;
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
byte colPins[3] = {6, 7, 8};
Keypad keypad = Keypad(makeKeymap(hexKeys), rowPins, colPins, 4, 3);
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);
TagDatum tagData[DB_SIZE]; //NOTE: Index 0 must always contain an invalid tagDatum
Servo servo;
String id = "";
short count = 0;

//HELPER FUNCTIONS
String waitForTag() {
    id = "";
    count = 0;
    while (true) {
        if (keypad.getKey() == '*') {
            return "";
        }
        if (Serial.available()) {
            id += (char) Serial.read();
            count++;
            if (count == 12) {
                Serial.println("Tag detected with id " + id);
                return id;
            }
        }
    }
}

void print(String line1, String line2) {
    lcd.clear();
    lcd.print(line1);
    lcd.setCursor(0, 1);
    lcd.print(line2);
}

short getIndexById(String id) {
    for (int i = 0; i < DB_SIZE; i++) {
        if (tagData[i].id == id) {
            return i;
        }
    }
    return -1;
}

short findMasterIndex() {
    for (int i = 0; i < DB_SIZE; i++) {
        if (tagData[i].isMaster) {
            return i;
        }
    }
    return -1;
}

short getNextEmptyIndex() {
    for (int i = 0; i < DB_SIZE; i++) {
        if (tagData[i].id == "") {
            return i;
        }
    }
    return -1;
}

void abortSetup(String line1, String line2) {
    print(line1, line2);
    digitalWrite(RED_LED, LOW);
    digitalWrite(GREEN_LED, LOW);
    delay(DELAY);
}

void setup() {
    //SETUP PINS
    pinMode(DOOR, INPUT);
    pinMode(RED_LED, OUTPUT);
    pinMode(GREEN_LED, OUTPUT);
    pinMode(BUZZER, OUTPUT);

    //SETUP LIBRARIES
    Serial.begin(9600);
    Serial.println("Starting system...");

    lcd.begin(40, 2);

    servo.attach(9);
    servo.write(0);

    //INITIALIZE DATABASE
    tagData[0] = TagDatum("AAAAAAAAAAAA", true, true);
    for (int i = 1; i < DB_SIZE; i++) {
        tagData[i] = TagDatum();
    }
}

void loop() {
    print("Welcome!", "Please produce your RFID tag");
    String id = waitForTag();
    short tagIndex = getIndexById(id);
    if (tagIndex == -1) {
        //CASE: Tag is not registered in the database
        print("Tag is not registered", "Access denied");
        tone(BUZZER, 500);
        digitalWrite(RED_LED, HIGH);
        delay(5000);
        noTone(BUZZER);
        digitalWrite(RED_LED, LOW);
    } else if (tagData[tagIndex].isActive) {
        //CASE: Tag is registered in the database and is active
        if (tagData[tagIndex].isMaster) {
            //CASE: Tag is the master tag
            waitForKey1: print("MASTER MENU: Choose an option", OPTIONS_MASTER);
            digitalWrite(RED_LED, HIGH);
            digitalWrite(GREEN_LED, HIGH);
            char key = keypad.waitForKey();
            if (key == '1') {
                //CASE: Edit database
                waitForTag1: print("TAG MANAGEMENT: Produce an RFID tag", OPTIONS_AWAIT_TAG);
                String id = waitForTag();
                if (id == "") {
                    abortSetup("Quitting", "Please wait");
                } else {
                    short tagIndex = getIndexById(id);
                    if (tagIndex == -1) {
                        //CASE: Tag is not registered
                        waitForKey2: print("UNREGISTERED TAG MENU: Choose an option", OPTIONS_UNREGISTERED_TAG);
                        char key = keypad.waitForKey();
                        if (key == '1') {
                            //CASE: Register new tag
                            short newIndex = getNextEmptyIndex();
                            if (newIndex == -1) {
                                //CASE: Database is full
                                print("Database is full", "Choose another option");
                                delay(DELAY);
                                goto waitForKey2;
                            } else {
                                //CASE: Tag registered successfully
                                tagData[newIndex] = TagDatum(id, false, false);
                                print("Tag registered successfully", "Please wait");
                                delay(DELAY);
                                tagIndex = newIndex;
                                goto waitForKey3;
                            }
                        } else if (key == '*') {
                            //CASE: Cancel setup
                            abortSetup("Aborting setup", "Please wait");
                        } else {
                            //CASE: Undefined option selected
                            print("No option for the key " + String(key), "Try again");
                            delay(DELAY);
                            goto waitForKey2;
                        }
                    } else if (tagIndex == findMasterIndex()) {
                        //CASE: Tag is the master tag
                        print("Cannot edit the master tag itself", "Try with another tag");
                        delay(DELAY);
                        goto waitForTag1;
                    } else {
                        //CASE: Tag is registered
                        waitForKey3: print("REGISTERED TAG MENU: Choose an option", OPTIONS_REGISTERED_TAG);
                        char key = keypad.waitForKey();
                        if (key == '1') {
                            //CASE: Unregister tag
                            tagData[tagIndex] = TagDatum();
                            abortSetup("Tag removed successfully", "Quitting setup");
                        } else if (key == '2') {
                            //CASE: Update tag properties
                            waitForKey4: if (tagData[tagIndex].isActive) {
                                print("TAG MENU: Choose an option", OPTIONS_EDIT_TAG_ACTIVE);
                            } else {
                                print("TAG MENU: Choose an option", OPTIONS_EDIT_TAG_INACTIVE);
                            }
                            char key = keypad.waitForKey();
                            if (key == '1' && tagData[tagIndex].isActive) {
                                //CASE: Deactivate tag
                                tagData[tagIndex].isActive = false;
                                print("Tag deactivated successfully", "Please continue");
                                delay(DELAY);
                                goto waitForKey4;
                            } else if (key == '1' && !tagData[tagIndex].isActive) {
                                //CASE: Activate tag
                                tagData[tagIndex].isActive = true;
                                print("Tag activated successfully", "Please continue");
                                delay(DELAY);
                                goto waitForKey4;
                            } else if (key == '2') {
                                //CASE: Make tag the master
                                tagData[findMasterIndex()].isMaster = false;
                                tagData[tagIndex].isActive = true;
                                tagData[tagIndex].isMaster = true;
                                abortSetup("Master tag is replaced successfully", "Quitting setup");
                            }else if (key == '*') {
                                //CASE: Cancel setup
                                abortSetup("Aborting setup", "Please wait");
                            } else {
                                //CASE: Undefined option selected
                                print("No option for the key " + String(key), "Try again");
                                delay(DELAY);
                                goto waitForKey4;
                            }
                        } else if (key == '*') {
                            //CASE: Cancel setup
                            abortSetup("Aborting setup", "Please wait");
                        } else {
                            //CASE: Undefined option selected
                            print("No option for the key " + String(key), "Try again");
                            delay(DELAY);
                            goto waitForKey3;
                        }
                    }
                }
            } else if (key == '*') {
                //CASE: Cancel setup
                abortSetup("Aborting setup", "Please wait");
            } else {
                //CASE: Undefined option selected
                print("No option for the key " + String(key), "Try again");
                delay(DELAY);
                goto waitForKey1;
            }
        } else {
            //CASE: Tag is not the master tag
            servo.write(180);
            print("Access granted", "Have a nice day!");
            digitalWrite(GREEN_LED, HIGH);
            delay(DELAY);
            print("The door will be locked down soon", "Hurry!");
            delay(10000);
            servo.write(0);
            print("Locking down", "Please wait");
            digitalWrite(GREEN_LED, LOW);
            delay(DELAY);
        }
    } else {
        //CASE: Tag is registered in the database and is not active
        print("Tag has no access permissions", "Access denied");
        tone(BUZZER, 10000);
        digitalWrite(RED_LED, HIGH);
        delay(5000);
        noTone(BUZZER);
        digitalWrite(RED_LED, LOW);
    }
}