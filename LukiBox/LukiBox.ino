/*
Using DFRobotDFPlayerMini Library V 1.0.5
*/

#include "Arduino.h"
#include "SoftwareSerial.h"
#include "DFRobotDFPlayerMini.h"
#include <Keypad.h>

SoftwareSerial mySoftwareSerial(10, 11); // RX, TX
DFRobotDFPlayerMini myDFPlayer;
void printDetail(uint8_t type, int value);

unsigned int lastKey = 0;
unsigned long lastNext = 0;
int songFolder = 1;

const byte ROWS = 4; //four rows
const byte COLS = 4; //three columns
char keys[ROWS][COLS] = {
{1,2,3,10},
{4,5,6,11},
{7,8,9,12},
{15,13,16,14}
};
byte rowPins[ROWS] = {9, 8, 7, 6}; //connect to the row pinouts of the kpd
byte colPins[COLS] = {5, 4, 3, 2}; //connect to the column pinouts of the kpd

Keypad kpd = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );

void setup()
{
  mySoftwareSerial.begin(9600);
  Serial.begin(115200);
  
  Serial.println();
  Serial.println(F("DFRobot DFPlayer Mini Demo"));
  Serial.println(F("Initializing DFPlayer ... (May take 3~5 seconds)"));
  
  if (!myDFPlayer.begin(mySoftwareSerial)) {  //Use softwareSerial to communicate with mp3.
    Serial.println(F("Unable to begin:"));
    Serial.println(F("1.Please recheck the connection!"));
    Serial.println(F("2.Please insert the SD card!"));
    while(true){
      delay(0); // Code to compatible with ESP8266 watch dog.
    }
  }
  Serial.println(F("DFPlayer Mini online."));
  //myDFPlayer.reset();
  myDFPlayer.outputDevice(DFPLAYER_DEVICE_SD);
  int r = (analogRead(A1) % 14) + 1;
  //int r = random(1, 15);
  Serial.print(F("random "));
  Serial.println(r);
  lastKey = r;
  myDFPlayer.volume(15);  //Set volume value. From 0 to 30
  play();
}

void loop()
{
  //if (!kpd.getKeys()) return;

  int i1 = 0, i2 = 0;

  unsigned long lasttime = 0;
  while (kpd.getKeys() || millis()-lasttime < 200)
    {
        for (int i=0; i<LIST_MAX; i++)   // Scan the whole key list.
        {
            if ( kpd.key[i].kstate == PRESSED )
            {
                switch (kpd.key[i].kchar) {  // Report active key state : IDLE, PRESSED, HOLD, or RELEASED
                    case 15:
                    myDFPlayer.volumeDown();
                return;
                    case 16:
                    myDFPlayer.volumeUp();
                return;
                    default:
                    
                    if (i1 == 0) {
                      i1 = kpd.key[i].kchar;
                    } else if (i1 != kpd.key[i].kchar) {
                      i2 = kpd.key[i].kchar;
                    }
                    Serial.print("Keys ");
                    Serial.print(i1);
                    Serial.print(", ");
                    Serial.println(i2);

                    lasttime = millis();
                }
            }
        }
    }

    int n = 0;
    if (i1 == 0) {
      if (myDFPlayer.available() /*&& (millis()-lastNext > 2000)*/) {
        printDetail(myDFPlayer.readType(), myDFPlayer.read());
        Serial.print("idle ");
        Serial.print(millis());
        Serial.print(" - ");
        Serial.print(lastNext);
        Serial.print(" -> ");
        lastNext = millis();

        uint8_t type = myDFPlayer.readType();
        if (type == DFPlayerPlayFinished) {
          next();
        } else if (type == DFPlayerError) {
          // folder not found, try next
          songFolder = 1;
          lastKey++;
          play();
        }
        
      }
      return;
    } else if (i2 == 0) {
      n = i1;
    } else if (i1 > i2) {
      n = (i2*14)-i2+i1;
    } else {
      n = (i1*14)-i1+i2;       
    }

    Serial.print("n: ");
    Serial.println(n);
                      
    if (lastKey == n) {
      next();
    } else {
      songFolder = 1;
      lastKey = n;
      play();
    }
}

void play() {
  Serial.print("Play Song ");
  Serial.print(lastKey);
  Serial.print(" ");
  Serial.println(songFolder);
  myDFPlayer.playFolder(lastKey, songFolder);
}


void next() {
  Serial.print("next: ");
  int count = -1;
  while (count < 0) { count = myDFPlayer.readFileCountsInFolder(lastKey); }
  Serial.print(songFolder);
  Serial.print("/");
  Serial.print(count);  
  songFolder = (songFolder % count) + 1;
  Serial.print("->");
  play();
}

void printDetail(uint8_t type, int value){
  Serial.print(type); Serial.print(" "); Serial.print(value); Serial.print(" ");
  switch (type) {
    case TimeOut:
      Serial.println(F("Time Out!"));
      break;
    case WrongStack:
      Serial.println(F("Stack Wrong!"));
      break;
    case DFPlayerCardInserted:
      Serial.println(F("Card Inserted!"));
      break;
    case DFPlayerCardRemoved:
      Serial.println(F("Card Removed!"));
      break;
    case DFPlayerCardOnline:
      Serial.println(F("Card Online!"));
      break;
    case DFPlayerUSBInserted:
      Serial.println("USB Inserted!");
      break;
    case DFPlayerUSBRemoved:
      Serial.println("USB Removed!");
      break;
    case DFPlayerPlayFinished:
      Serial.print(F("Number:"));
      Serial.print(value);
      Serial.println(F(" Play Finished!"));
      break;
    case DFPlayerError:
      Serial.print(F("DFPlayerError:"));
      switch (value) {
        case Busy:
          Serial.println(F("Card not found"));
          break;
        case Sleeping:
          Serial.println(F("Sleeping"));
          break;
        case SerialWrongStack:
          Serial.println(F("Get Wrong Stack"));
          break;
        case CheckSumNotMatch:
          Serial.println(F("Check Sum Not Match"));
          break;
        case FileIndexOut:
          Serial.println(F("File Index Out of Bound"));
          break;
        case FileMismatch:
          Serial.println(F("Cannot Find File"));
          break;
        case Advertise:
          Serial.println(F("In Advertise"));
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }  
}
