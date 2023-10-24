#include <Arduino.h>
#include <SoftwareSerial.h>
#define DEBUG_DFPLAYER_COMMUNICATION
#include "DFMiniMp3.h"
#include <Keypad.h>

SoftwareSerial mySoftwareSerial(11, 10); // RX, TX

volatile boolean error = false;
volatile uint16_t errCode = 0;
class Mp3Notify;
DFMiniMp3<SoftwareSerial, Mp3Notify> player(mySoftwareSerial);

unsigned int lastKey = 0;
unsigned long lastNext = 0;
int songFolder = 1;

const byte ROWS = 4; //four rows
const byte COLS = 4; //three columns
char keys[ROWS][COLS] = {
{15,1,2,16},
{3,4,5,6},
{7,8,9,10},
{11,12,13,14}
};
byte rowPins[ROWS] = {9, 8, 7, 6}; //connect to the row pinouts of the kpd
byte colPins[COLS] = {5, 4, 3, 2}; //connect to the column pinouts of the kpd

Keypad kpd = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );

void setup()
{
  Serial.begin(115200);
  
  Serial.println();
  Serial.println(F("Initializing DFPlayer..."));
  delay(0000);
  player.setACK(false);
  player.begin();
  
  while (true) {
    error = false;
    player.getStatus();
    if (!error) break;
    if (errCode == DfMp3_Error_RxTimeout) {
        Serial.println(F("==========================="));
        Serial.println(F("\n No communication possible"));
        Serial.println(F(" !!! CHECK YOUR WIRING !!!"));
        Serial.println(F("\n==========================="));
      }
      if (errCode == DfMp3_Error_Busy) {
        Serial.println(F("=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-="));
        Serial.println(F("\n No medium found!!!"));
        Serial.println(F(" Check SD card and/or USB stick!"));
        Serial.println(F("\n=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-="));
      }
    delay(0000); // Code to compatible with ESP8266 watch dog.
  }
  Serial.println(F("DFPlayer Mini online."));
  player.setPlaybackSource(DfMp3_PlaySource_Sd);
  delay(100);
  player.setEq(DfMp3_Eq_Normal);
  delay(100);
  player.enableDac();
  delay(250);
  player.setVolume(15); //Set volume value. From 0 to 30

  int r = (analogRead(A1) % 14) + 1;

  Serial.print(F("random "));
  Serial.println(r);
  lastKey = r;
  delay(100);
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
                    case 16:
                    Serial.print("Volume Down");
                    player.decreaseVolume();
                return;
                    case 15:
                    Serial.print("Volume Up");
                    player.increaseVolume();
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

    // nothing pressed
    if (i1 == 0) {
      busyWait(100);    
      return;
    }

    int n = 0;
    if (i1 > i2) {
      n = i1;
      i1 = i2;
      i2 = n;
      // i1 < i2
    }
    n = (i1*14)+i2-i1*(i1+1)/2; // 1..105
    if (n>=100) n=99;
    
    Serial.print("n: ");
    Serial.println(n);
                      
    if (lastKey == n) {
      next();
    } else {
      n--;
      lastKey = n;
      nextFolder();
    }
}

void play() {
  Serial.print("Play Song ");
  Serial.print(lastKey);
  Serial.print(" ");
  Serial.println(songFolder);
  player.playFolderTrack(lastKey, songFolder);
}

void nextFolder() {
  int count = -1;
  while (count <= 0) {
    songFolder = 1;
    lastKey++;
    lastKey = lastKey % 100;
    count = -1;
    while (count < 0) count = player.getFolderTrackCount(lastKey);

    Serial.print("Next Folder: ");
    Serial.print(lastKey);
    Serial.print(" count: ");
    Serial.println(count);
  }
  play();
}

void next() {
  Serial.print("next: ");
  int count = -1;
  while (count < 0) { count = player.getFolderTrackCount(lastKey); }
  if (count == 0) {
    nextFolder();
    return;
  }
  Serial.print(songFolder);
  Serial.print("/");
  Serial.print(count);  
  songFolder = (songFolder % count) + 1;
  Serial.print("->");
  play();
}
boolean isPlaying() {
      uint16_t state = 0;
      int retries = 3;

      error = true;
      while (retries > 0 && error) {
        --retries;
        error = false;
        state = player.getStatus() & 0xFF;
        if (error) {
          delay(100);
        }
      }
      error = false;

      return (state & 1) == 1;
}

    void busyWait(long ms) {
      long startMs = millis();
      while (millis() < startMs + ms) {
        player.loop();
        delay(50);
      }
    }


class Mp3Notify
{
  public:
    static void OnError(uint16_t errorCode) {
      Serial.print(F("--------------\n ERROR "));
      Serial.println(errorCode);
      Serial.println(F("--------------"));
      error = true;
      errCode = errorCode;
      nextFolder();
    }

    static void OnPlayFinished(uint16_t globalTrack) {
      Serial.print(F("Callback global track finished: "));
      Serial.println(globalTrack);
      next();
    }

    static void OnCardOnline(uint16_t code) {
      Serial.print(F("Callback OnCardOnline: "));
      Serial.println(code);
      play();
    }

    static void OnCardInserted(uint16_t code) {
      Serial.print(F("Callback OnCardInserted: "));
      Serial.println(code);
      play();
    }

    static void OnCardRemoved(uint16_t code) {
      Serial.print(F("Callback OnCardRemoved: "));
      Serial.println(code);
    }

    static void OnUsbOnline(uint16_t code) {
      Serial.print(F("Callback OnUsbOnline: "));
      Serial.println(code);
    }

    static void OnUsbInserted(uint16_t code) {
      Serial.print(F("Callback OnUsbInserted: "));
      Serial.println(code);
    }

    static void OnUsbRemoved(uint16_t code) {
      Serial.print(F("Callback OnUsbRemoved: "));
      Serial.println(code);
    }
};
