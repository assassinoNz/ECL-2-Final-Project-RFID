#include <Arduino.h>
#include <LiquidCrystal.h>
#include <Keypad.h>

#define DB_SIZE 5
#define OPTIONS_MASTER "1.Edit Database #.Cancel"
#define OPTIONS_UNREGISTERED_TAG "1.Register #.Cancel"
#define OPTIONS_REGISTERED_TAG "1.Unregister 2.Edit tag #.Cancel"
#define OPTIONS_EDIT_TAG "1.Activate 2.Deactivate 3.Make master #.Cancel"

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
byte colPins[3] = {8, 9, 10};
Keypad keypad = Keypad(makeKeymap(hexKeys), rowPins, colPins, 4, 3);
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);
String id = "";
int count = 0;
TagDatum tagData[DB_SIZE]; //NOTE: Index 0 must always contain an invalid tagDatum

//HELPER FUNCTIONS
String waitForTag() {
    count = 0;
    id = "";
    while (true) {
        while (Serial.available() > 0) {
            id += (char)Serial.read();
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

void setup() {
    //SETUP LIBRARIES
    Serial.begin(9600);
    Serial.println("Starting system...");

    lcd.begin(40, 2);

    //INITIALIZE DATABASE
    tagData[0] = TagDatum("AAAAAAAAAAAA", true, true);
    tagData[1] = TagDatum("BBBBBBBBBBBB", true, false);
    tagData[2] = TagDatum("CCCCCCCCCCCC", false, false);
    for (int i = 3; i < DB_SIZE; i++) {
        tagData[i] = TagDatum();
    }
}

void loop() {
    print("Welcome! Please produce your RFID tag", "");
    String id = waitForTag();
    short tagIndex = getIndexById(id);
    if (tagIndex == -1) {
        //CASE: Tag is not registered in the database
        print("Tag is not registered. Access denied", "");
    } else if (tagData[tagIndex].isActive) {
        //CASE: Tag is registered in the database and is active
        if (tagData[tagIndex].isMaster) {
            //CASE: Tag is the master tag
            print("MASTER MENU: Choose an option", OPTIONS_MASTER);
            waitForKey1: char key = keypad.waitForKey();
            if (key == '1') {
                //CASE: Edit database
                print("EDIT DATABASE: Produce an RFID tag", "");
                String id = waitForTag();
                short tagIndex = getIndexById(id);
                if (tagIndex == -1) {
                    //CASE: Tag is not registered
                    print("UNREGISTERED TAG MENU: Choose an option", OPTIONS_UNREGISTERED_TAG);
                    waitForKey2: char key = keypad.waitForKey();
                    if (key == '1') {
                        //CASE: Register new tag
                        short newIndex = getNextEmptyIndex();
                        if (newIndex == -1) {
                            //CASE: Database is full
                            print("Database is full. Choose another option", OPTIONS_UNREGISTERED_TAG);
                            goto waitForKey2;
                        } else {
                            //CASE: Tag registered successfully
                            tagData[newIndex] = TagDatum(id, false, false); 
                            print("Tag registered successfully. Please wait", "");
                            delay(2000);
                        }
                    } else if (key == '#') {
                        //CASE: Cancel setup
                        print("Aborting setup. Please wait", "");
                        delay(2000);
                    } else {
                        //CASE: Undefined option selected
                        print("No option for the key " + String(key) + ". Try again", OPTIONS_UNREGISTERED_TAG);
                        goto waitForKey2;
                    }
                } else {
                    //CASE: Tag is registered
                    print("REGISTERED TAG MENU: Choose an option", OPTIONS_REGISTERED_TAG);
                    waitForKey3: char key = keypad.waitForKey();
                    if (key == '1') {
                        //CASE: Unregister tag
                        tagData[tagIndex] = TagDatum(); 
                        print("Tag removed successfully. Please wait", "");
                        delay(2000);
                    } else if (key == '2') {
                        //CASE: Update tag properties
                        print("EDIT TAG MENU: Choose an option", OPTIONS_EDIT_TAG);
                        waitForKey4: char key = keypad.waitForKey();
                        if (key == '1') {
                            //CASE: Activate tag
                            tagData[tagIndex].isActive = true;
                            print("Tag activated successfully. Anything else?", OPTIONS_EDIT_TAG);
                            goto waitForKey4;
                        } else if (key == '2') {
                            //CASE: Deactivate tag
                            tagData[tagIndex].isActive = false;
                            print("Tag deactivated successfully. Anything else?", OPTIONS_EDIT_TAG);
                            goto waitForKey4;
                        } else if (key == '3') {
                            //CASE: Make tag the master
                            tagData[tagIndex].isMaster = true;
                            tagData[findMasterIndex()].isMaster = false;
                            print("Tag is now the master. Anything else?", OPTIONS_EDIT_TAG);
                            goto waitForKey4;
                        }else if (key == '#') {
                            //CASE: Cancel setup
                            print("Aborting setup. Please wait", "");
                            delay(2000);
                        } else {
                            //CASE: Undefined option selected
                            print("No option for the key " + String(key) + ". Try again", OPTIONS_EDIT_TAG);
                            goto waitForKey4;
                        }
                    } else if (key == '#') {
                        //CASE: Cancel setup
                        print("Aborting setup. Please wait", "");
                        delay(2000);
                    } else {
                        //CASE: Undefined option selected
                        print("No option for the key " + String(key) + ". Try again", OPTIONS_REGISTERED_TAG);
                        goto waitForKey3;
                    }
                }
            } else if (key == '#') {
                //CASE: Cancel setup
                print("Aborting setup. Please wait", "");
                delay(2000);
            } else {
                //CASE: Undefined option selected
                print("No option for the key " + String(key) + ". Try again", OPTIONS_MASTER);
                goto waitForKey1;
            }
        } else {
            //CASE: Tag is not the master tag
            print("Access granted. Have a nice day!", "");
        }
    } else {
        //CASE: Tag is registered in the database and is not active
        print("Tag has no access permissions. Access denied", "");
    }

    delay(500);
}