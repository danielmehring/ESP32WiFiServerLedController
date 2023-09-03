#include <WiFi.h>
#include <EEPROM.h>
#include <Arduino.h>

#define EEPROM_SIZE 2

#if FLASHEND <= 0x1FFF
#define EXCLUDE_EXOTIC_PROTOCOLS
#if !defined(DIGISTUMPCORE)
#define EXCLUDE_UNIVERSAL_PROTOCOLS
# endif
#endif

#define MARK_EXCESS_MICROS 20

#include <IRremote.h>

#include "PinDefinitionsAndMore.h"

#if defined(APPLICATION_PIN)
#define DEBUG_BUTTON_PIN APPLICATION_PIN // if low, print timing for each received data set
#else
#define DEBUG_BUTTON_PIN 6
#endif

#if defined(ARDUINO_ARCH_SAMD)
#define Serial SerialUSB
#endif

const char * ssid = "YOUR_SSID";
const char * password = "YOUR_WIFI_PASSWORD";

WiFiServer server(80);

/* Set your Static IP address
IPAddress local_IP(192, 168, 2, 1101;
// Set your Gateway IP address
IPAddress gateway(192, 168, 2, 1);

IPAddress subnet(255, 255, 255, 0);
*/

int ledRPin = 5;
int ledGPin = 18;
int ledBPin = 19;

const int channelR = 0;
const int channelG = 1;
const int channelB = 2;
const int frequency = 1000;
const int resolution = 10;

bool iscurrentlyon = false;
double brightness = 1.0;
int slowfactor = 1;

TaskHandle_t Task2;

//DIY: generell 8 bit, wird erst vor posten in 11 bit umgeändert
  int diy1R = 255;int diy1G = 255;int diy1B = 255;int diy2R = 255;int diy2G = 255;int diy2B = 255;int diy3R = 255;int diy3G = 255;int diy3B = 255;int diy4R = 255;int diy4G = 255;
  int diy4B = 255;int diy5R = 255;int diy5G = 255;int diy5B = 255;int diy6R = 255;int diy6G = 255;int diy6B = 255; 


//Value

int valueR = 0;
int valueG = 0;
int valueB = 0;
  
  //States
  int currentState = 0; //0 = vorprogrammiertefarben, 1 - 6 DIY, 7 fade jump etc.
  int colorState= 1; //R: 1-5, G: 6-10, B: 11-15, W: 16-20
  
  //Fade mit phasenverschiebung machen
  int phaseShiftDIYR, phaseShiftDIYG, phaseShiftDIYB = 0;
  
  //Alle vorprogrammierten Farben:
  //R1
  int R1R = 1023;int R1G = 0;int R1B = 0;
  //R2
  int R2R = 1023;int R2G = 256;int R2B = 0;
  //R3
  int R3R = 1023;int R3G = 512;int R3B = 0;
  //R4
  int R4R = 1023;int R4G = 768;int R4B = 0;
  //R5
  int R5R = 1023;int R5G = 1023;int R5B = 0;
  
  //G1
  int G1G = 1023;int G1R = 0;int G1B = 0;
  //G2
  int G2R = 0;int G2G = 1023;int G2B = 256;
  //G3
  int G3R = 0;int G3G = 1023;int G3B = 512;
  //G4
  int G4R = 0;int G4G = 1023;int G4B = 768;
  //G5
  int G5B = 1023;int G5G = 1023;int G5R = 0;
  
  //B1
  int B1B = 1023;int B1R = 0;int B1G = 0;
  //B2
  int B2G = 0;int B2B = 1023;int B2R = 256;
  //B3
  int B3G = 0;int B3B = 1023;int B3R = 512;
  //B4
  int B4G = 0;int B4B = 1023;int B4R = 768;
  //B5
  int B5B = 1023;int B5R = 1023;int B5G = 0;
  
  
  //W1
  int W1R = 1023;int W1G = 1023;int W1B = 1023;
  //W2
  int W2G = 924;int W2B = 1023;int W2R = 1023;
  //W3
  int W3G = 824;int W3B = 1023;int W3R = 1023;
  //W4
  int W4G = 1023;int W4B = 1023;int W4R = 924;
  //W5
  int W5G = 1023;int W5B = 1023;int W5R = 824;



void setup() {
  #if defined(IR_MEASURE_TIMING) && defined(IR_TIMING_TEST_PIN)
  pinMode(IR_TIMING_TEST_PIN, OUTPUT);
  #endif
  #if FLASHEND >= 0x3FFF // For 16k flash or more, like ATtiny1604. Code does not fit in program space of ATtiny85 etc.
  pinMode(DEBUG_BUTTON_PIN, INPUT_PULLUP);
  #endif
  Serial.begin(115200);

  #if defined(__AVR_ATmega32U4__) || defined(SERIAL_PORT_USBVIRTUAL) || defined(SERIAL_USB) || defined(ARDUINO_attiny3217)
  delay(4000); // To be able to connect Serial monitor after reset or power up and before first print out. Do not wait for an attached Serial Monitor!
  #endif
  // Just to know which program is running on my Arduino
  Serial.println(F("START "
    __FILE__ " from "
    __DATE__ "\r\nUsing library version "
    VERSION_IRREMOTE));

  // In case the interrupt driver crashes on setup, give a clue
  // to the user what's going on.
  Serial.println(F("Enabling IRin..."));

  /*
   * Start the receiver, enable feedback LED and (if not 3. parameter specified) take LED feedback pin from the internal boards definition
   */
  IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK);

  Serial.print(F("Ready to receive IR signals at pin "));
  #if defined(ARDUINO_ARCH_STM32) || defined(ESP8266)
  Serial.println(IR_RECEIVE_PIN_STRING);
  #else
  Serial.println(IR_RECEIVE_PIN);
  #endif

  #if FLASHEND >= 0x3FFF // For 16k flash or more, like ATtiny1604. Code does not fit in program space of ATtiny85 etc.
  Serial.print(F("Debug button pin is "));
  Serial.println(DEBUG_BUTTON_PIN);
  #endif

  // infos for receive
  Serial.print(RECORD_GAP_MICROS);
  Serial.println(F(" us is the (minimum) gap, after which the start of a new IR packet is assumed"));
  Serial.print(MARK_EXCESS_MICROS);
  Serial.println(F(" us are subtracted from all marks and added to all spaces for decoding"));

  ledcSetup(channelR, frequency, resolution);
  ledcAttachPin(ledRPin, channelR);
  ledcSetup(channelG, frequency, resolution);
  ledcAttachPin(ledGPin, channelG);
  ledcSetup(channelB, frequency, resolution);
  ledcAttachPin(ledBPin, channelB);

  delay(10);
  // We start by connecting to a WiFi network

  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);
  WiFi.setSleep(false);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  server.begin();

  delay(500);
  xTaskCreatePinnedToCore(
                    Task2code,   /* Task function. */
                    "Task2",     /* name of task. */
                    10000,       /* Stack size of task */
                    NULL,        /* parameter of the task */
                    1,           /* priority of the task */
                    &Task2,      /* Task handle to keep track of created task */
                    1);          /* pin task to core 1 */
                
}


void turnon() {
  if (!iscurrentlyon) {
    iscurrentlyon = true;
    if(iscurrentlyon) {
      Serial.println("Turned on.");
    }
  }
}

void turnoff() {
  if (iscurrentlyon) {
    iscurrentlyon = false;
    if(!iscurrentlyon) {
      Serial.println("Turned off.");
    }
    ledcWrite(channelR, 0);
    ledcWrite(channelG, 0);
    ledcWrite(channelB, 0);
  }
}

int convert8to11(int bit8) {
  return (int)(4.011764705882352941176470588235294 * bit8);
}


void bodycontent(String s) {
  s.replace(" ", "");
  s = getValue(s, ':', 1);

  if (getValue(s, ';', 0) != "n") {
    if (getValue(s, ';', 0).toInt() == 1) {
      turnon();
    } else {
      turnoff();
    }
  }
  if (getValue(s, ';', 1) != "n") {
    if (getValue(s, ';', 1).toInt() <= 7) {
      currentState = getValue(s, ';', 1).toInt();
    }
  }
  if (getValue(s, ';', 2) != "n") {
    brightness = getValue(s, ';', 2).toDouble();
  }

  
  if (getValue(s, ';', 3) != "n") {
    if (iscurrentlyon) {
      ledcWrite(channelR, getValue(s, ';', 3).toInt());
    }
  }
  if (getValue(s, ';', 4) != "n") {
    if (iscurrentlyon) {
      ledcWrite(channelG, getValue(s, ';', 4).toInt());
    }
  }
  if (getValue(s, ';', 5) != "n") {
    if (iscurrentlyon) {
      ledcWrite(channelB, getValue(s, ';', 5).toInt());
    }
  }

  if (getValue(s, ';', 6) != "n") {
    slowfactor = getValue(s, ';', 6).toInt();
  }

  if (getValue(s, ';', 7) != "n") {
    diy1R = getValue(s, ';', 7).toInt();
  }
  if (getValue(s, ';', 8) != "n") {
    diy1G = getValue(s, ';', 8).toInt();
  }
  if (getValue(s, ';', 9) != "n") {
    diy1B = getValue(s, ';', 9).toInt();
  }

  if (getValue(s, ';', 10) != "n") {
    diy2R = getValue(s, ';', 10).toInt();
  }
  if (getValue(s, ';', 11) != "n") {
    diy2G = getValue(s, ';', 11).toInt();
  }
  if (getValue(s, ';', 12) != "n") {
    diy2B = getValue(s, ';', 12).toInt();
  }

  if (getValue(s, ';', 13) != "n") {
    diy3R = getValue(s, ';', 13).toInt();
  }
  if (getValue(s, ';', 14) != "n") {
    diy3G = getValue(s, ';', 14).toInt();
  }
  if (getValue(s, ';', 15) != "n") {
    diy3B = getValue(s, ';', 15).toInt();
  }

  if (getValue(s, ';', 16) != "n") {
    diy4R = getValue(s, ';', 16).toInt();
  }
  if (getValue(s, ';', 17) != "n") {
    diy4G = getValue(s, ';', 17).toInt();
  }
  if (getValue(s, ';', 18) != "n") {
    diy4B = getValue(s, ';', 18).toInt();
  }

  if (getValue(s, ';', 19) != "n") {
    diy5R = getValue(s, ';', 19).toInt();
  }
  if (getValue(s, ';', 20) != "n") {
    diy5G = getValue(s, ';', 20).toInt();
  }
  if (getValue(s, ';', 21) != "n") {
    diy5B = getValue(s, ';', 21).toInt();
  }

  if (getValue(s, ';', 22) != "n") {
    diy6R = getValue(s, ';', 22).toInt();
  }
  if (getValue(s, ';', 23) != "n") {
    diy6G = getValue(s, ';', 23).toInt();
  }
  if (getValue(s, ';', 24) != "n") {
    diy6B = getValue(s, ';', 24).toInt();
  }

  

  if (getValue(s, ';', 25) != "n") {
    phaseShiftDIYR = getValue(s, ';', 26).toInt();
  }
  if (getValue(s, ';', 26) != "n") {
    phaseShiftDIYG = getValue(s, ';', 27).toInt();
  }
  if (getValue(s, ';', 27) != "n") {
    phaseShiftDIYB = getValue(s, ';', 28).toInt();
  }

  if (getValue(s, ';', 3) == "n") {
    updateLeds();
  }

  Serial.println(s);
}



void quick() {
  if (slowfactor > 1) {
    slowfactor--;
  }
}

void slow() {
  if (slowfactor < 250) {
    slowfactor++;
  }
}

void brightnessup() {
  if (brightness < 1.0) {
    brightness += 0.005;
  }
}

void brightnessdown() {
  if (brightness > 0.005) {
    brightness -= 0.005;
  }
}

void diyRup() {
  switch (currentState) {
    case 1:
      if (diy1R < 255) {
        diy1R++;
      }
      break;
    case 2:
      if (diy2R < 255) {
        diy2R++;
      }
      break;
    case 3:
      if (diy3R < 255) {
        diy3R++;
      }
      break;
    case 4:
      if (diy4R < 255) {
        diy4R++;
      }
      break;
    case 5:
      if (diy5R < 255) {
        diy5R++;
      }
      break;
    case 6:
      if (diy6R < 255) {
        diy6R++;
      }
      break;
  }
  updateLeds();
}
void diyGup() {
  switch (currentState) {
    case 1:
      if (diy1G < 255) {
        diy1G++;
      }
      break;
    case 2:
      if (diy2G < 255) {
        diy2G++;
      }
      break;
    case 3:
      if (diy3G < 255) {
        diy3G++;
      }
      break;
    case 4:
      if (diy4G < 255) {
        diy4G++;
      }
      break;
    case 5:
      if (diy5G < 255) {
        diy5G++;
      }
      break;
    case 6:
      if (diy6G < 255) {
        diy6G++;
      }
      break;
  }
  updateLeds();
}
void diyBup() {
  switch (currentState) {
    case 1:
      if (diy1B < 255) {
        diy1B++;
      }
      break;
    case 2:
      if (diy2B < 255) {
        diy2B++;
      }
      break;
    case 3:
      if (diy3B < 255) {
        diy3B++;
      }
      break;
    case 4:
      if (diy4B < 255) {
        diy4B++;
      }
      break;
    case 5:
      if (diy5B < 255) {
        diy5B++;
      }
      break;
    case 6:
      if (diy6B < 255) {
        diy6B++;
      }
      break;
  }
  updateLeds();
}

void diyRdown() {
  switch (currentState) {
    case 1:
      if (diy1R > 0) {
        diy1R--;
      }
      break;
    case 2:
      if (diy2R > 0) {
        diy2R--;
      }
      break;
    case 3:
      if (diy3R > 0) {
        diy3R--;
      }
      break;
    case 4:
      if (diy4R > 0) {
        diy4R--;
      }
      break;
    case 5:
      if (diy5R > 0) {
        diy5R--;
      }
      break;
    case 6:
      if (diy6R > 0) {
        diy6R--;
      }
      break;
  }
  updateLeds();
}
void diyGdown() {
  switch (currentState) {
    case 1:
      if (diy1G > 0) {
        diy1G--;
      }
      break;
    case 2:
      if (diy2G > 0) {
        diy2G--;
      }
      break;
    case 3:
      if (diy3G > 0) {
        diy3G--;
      }
      break;
    case 4:
      if (diy4G > 0) {
        diy4G--;
      }
      break;
    case 5:
      if (diy5G > 0) {
        diy5G--;
      }
      break;
    case 6:
      if (diy6G > 0) {
        diy6G--;
      }
      break;
  }
  updateLeds();
}
void diyBdown() {
  switch (currentState) {
    case 1:
      if (diy1B > 0) {
        diy1B--;
      }
      break;
    case 2:
      if (diy2B > 0) {
        diy2B--;
      }
      break;
    case 3:
      if (diy3B > 0) {
        diy3B--;
      }
      break;
    case 4:
      if (diy4B > 0) {
        diy4B--;
      }
      break;
    case 5:
      if (diy5B > 0) {
        diy5B--;
      }
      break;
    case 6:
      if (diy6B > 0) {
        diy6B--;
      }
      break;
  }
  updateLeds();
}

void writeLeds(int r, int g, int b, bool eightbit) {
  if (iscurrentlyon) {
    if (eightbit) {
      r = convert8to11(r);
      g = convert8to11(g);
      b = convert8to11(b);
    }
    ledcWrite(channelR, (int) r * brightness);
    ledcWrite(channelG, (int) g * brightness);
    ledcWrite(channelB, (int) b * brightness);
  } else {
    ledcWrite(channelR, 0);
    ledcWrite(channelG, 0);
    ledcWrite(channelB, 0);
  }
}

int phaseshift(int i) {
  if (i < 0) {
    i = (-1) * i;
  }
  if (i <= 1023) {
    return i;
  } else {
    if ((i % 2046) >= 1023) {
      return 1023 - (i % 1023);
    } else {
      return i % 1023;
    }
 }
}

void updateLeds() {
  if (currentState != 7) {
    switch (currentState) {
      case 0:
        writeLeds(valueR, valueG, valueB, false);
        break;
      case 1:
        writeLeds(diy1R, diy1G, diy1B, true);
        break;
      case 2:
        writeLeds(diy2R, diy2G, diy2B, true);
        break;
      case 3:
        writeLeds(diy3R, diy3G, diy3B, true);
        break;
      case 4:
        writeLeds(diy4R, diy4G, diy4B, true);
        break;
      case 5:
        writeLeds(diy5R, diy5G, diy5B, true);
        break;
      case 6:
        writeLeds(diy6R, diy6G, diy6B, true);
        break;
    }
  }
}

String getValue(String data, char separator, int index) {
  int found = 0;
  int strIndex[] = {
    0,
    -1
  };
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }
  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}



int value = 0;

int counteri = 0;
int counterj = 0;


//Task2code: blinks an LED every 700 ms
void Task2code( void * pvParameters ){
  Serial.print("Task2 running on core ");
  Serial.println(xPortGetCoreID());

  int i = 0;
  int j = 0;
  for(;;){
    if(iscurrentlyon) {
    } else {
    }

    
    if (iscurrentlyon && currentState == 7) {
    
    if (i < 2046) {
      i++;
      if (i <= 1023) {
        j++;
      } else {
        j--;
      }
    } else if (i >= 2046) {
      i = 0;
    }
    /*switch(modeforj) {
      case 0: //j3
        
      break; 
        
      case 1: //j7
        
      break;
      
      case 2: //f3
        
      break;
      
      case 3: //f7*/
        writeLeds(phaseshift(j - phaseShiftDIYR), phaseshift(j - phaseShiftDIYG), phaseshift(j - phaseShiftDIYB), false);
      /*break;
    }*/
    Serial.println(i);
    Serial.println(j);
    delay(1 * slowfactor);
  }
  }
}


void loop() {
  WiFiClient client = server.available(); // listen for incoming clients

  if (client) { // if you get a client,
    Serial.println("New Client."); // print a message out the serial port
    String currentLine = ""; // make a String to hold incoming data from the client
    while (client.connected()) { // loop while the client's connected
      if (client.available()) { // if there's bytes to read from the client,
        char c = client.read(); // read a byte, then
        //Serial.write(c);                    // print it out the serial monitor

        if (currentLine.startsWith("body") && currentLine.endsWith(";end")) {
          bodycontent(currentLine);
          delay(5);
        }

        if (c == '\n') {
          if (currentLine.length() == 0) {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();

            if (iscurrentlyon) {
              client.print("1");
            } else {
              client.print("0");
            }
            client.print(";");
            client.print(currentState);
            
            client.print(";");
            client.print(brightness);
            client.print(";");
            
            client.print(slowfactor);
            client.print(";");
            client.print(diy1R);
            client.print(";");
            client.print(diy1G);
            client.print(";");
            client.print(diy1B);
            client.print(";");
            client.print(diy2R);
            client.print(";");
            client.print(diy2G);
            client.print(";");
            client.print(diy2B);
            client.print(";");
            client.print(diy3R);
            client.print(";");
            client.print(diy3G);
            client.print(";");
            client.print(diy3B);
            client.print(";");
            client.print(diy4R);
            client.print(";");
            client.print(diy4G);
            client.print(";");
            client.print(diy4B);
            client.print(";");
            client.print(diy5R);
            client.print(";");
            client.print(diy5G);
            client.print(";");
            client.print(diy5B);
            client.print(";");
            client.print(diy6R);
            client.print(";");
            client.print(diy6G);
            client.print(";");
            client.print(diy6B);
            client.print(";");

            client.print(phaseShiftDIYR);
            client.print(";");
            client.print(phaseShiftDIYG);
            client.print(";");
            client.print(phaseShiftDIYB);
            
            client.println();
            break;
          } else { // if you got a newline, then clear currentLine:
            currentLine = "";
          }
        } else if (c != '\r') { // if you got anything else but a carriage return character,
          currentLine += c; // add it to the end of the currentLine
        }

        // Check to see if the client request was "GET /H" or "GET /L":
      }
    }
    // close the connection:
    client.stop();
    Serial.println("Client Disconnected.");
  }

  if (IrReceiver.decode()) {
    //Serial.println();
    #if FLASHEND >= 0x3FFF // For 16k flash or more, like ATtiny1604
    if (IrReceiver.decodedIRData.flags & IRDATA_FLAGS_WAS_OVERFLOW) {
      IrReceiver.decodedIRData.flags = false; // yes we have recognized the flag :-)
      Serial.println(F("Overflow detected"));
      #if !defined(ESP32) && !defined(ESP8266) && !defined(NRF5)
      /*
       * do double beep
       */
      IrReceiver.stop();
      tone(TONE_PIN, 1100, 10);
      delay(50);
      # endif

    } else {
      // Print a short summary of received data
      IrReceiver.printIRResultShort( & Serial);

      if (IrReceiver.decodedIRData.protocol == UNKNOWN || digitalRead(DEBUG_BUTTON_PIN) == LOW) {
        // We have an unknown protocol, print more info
        IrReceiver.printIRResultRawFormatted( & Serial, true);
      }
    }

    #if !defined(ESP32) && !defined(ESP8266) && !defined(NRF5)
    if (IrReceiver.decodedIRData.protocol != UNKNOWN) {
      /*
       * Play tone, wait and restore IR timer, if a valid protocol was received
       * Otherwise do not disturb the detection of the gap between transmissions. This will give
       * the next printIRResult* call a chance to report about changing the RECORD_GAP_MICROS value.
       */
      IrReceiver.stop();
      tone(TONE_PIN, 2200, 10);
      delay(8);
      IrReceiver.start(8000); // to compensate for 8 ms stop of receiver. This enables a correct gap measurement.
    }
    # endif
    #else
    // Print a minimal summary of received data
    IrReceiver.printIRResultMinimal( & Serial);
    #endif // FLASHEND

    /*
     * !!!Important!!! Enable receiving of the next value,
     * since receiving has stopped after the end of the current received data packet.
     */
    IrReceiver.resume();

    /*
     * Finally check the received data and perform actions according to the received address and commands
     */
    if (IrReceiver.decodedIRData.address == 0) {
      if (IrReceiver.decodedIRData.command == 0x40) {
        Serial.println("ONOFF");
        if (iscurrentlyon) {
          turnoff();
        } else {
          turnon();
        }
      }
      if (iscurrentlyon) {
         if (IrReceiver.decodedIRData.command == 0x5C) {
          Serial.println("BU");
          brightnessup();
        } else if (IrReceiver.decodedIRData.command == 0x5D) {
          Serial.println("BD");
          brightnessdown();
        } else if (IrReceiver.decodedIRData.command == 0x58) {
          Serial.println("R1");
          currentState = 0;
          writeLeds(R1R, R1G, R1B, false);
        } else if (IrReceiver.decodedIRData.command == 0x54) {
          Serial.println("R2");
          currentState = 0;
          writeLeds(R2R, R2G, R2B, false);
        } else if (IrReceiver.decodedIRData.command == 0x50) {
          Serial.println("R3");
          currentState = 0;
          writeLeds(R3R, R3G, R3B, false);
        } else if (IrReceiver.decodedIRData.command == 0x1C) {
          Serial.println("R4");
          currentState = 0;
          writeLeds(R4R, R4G, R4B, false);
        } else if (IrReceiver.decodedIRData.command == 0x18) {
          Serial.println("R5");
          currentState = 0;
          writeLeds(R5R, R5G, R5B, false);
  
        } else if (IrReceiver.decodedIRData.command == 0x59) {
          Serial.println("G1");
          currentState = 0;
          writeLeds(G1R, G1G, G1B, false);
        } else if (IrReceiver.decodedIRData.command == 0x55) {
          Serial.println("G2");
          currentState = 0;
          writeLeds(G2R, G2G, G2B, false);
        } else if (IrReceiver.decodedIRData.command == 0x51) {
          Serial.println("G3");
          currentState = 0;
          writeLeds(G3R, G3G, G3B, false);
        } else if (IrReceiver.decodedIRData.command == 0x1D) {
          Serial.println("G4");
          currentState = 0;
          writeLeds(G4R, G4G, G4B, false);
        } else if (IrReceiver.decodedIRData.command == 0x19) {
          Serial.println("G5");
          currentState = 0;
          writeLeds(G5R, G5G, G5B, false);
  
        } else if (IrReceiver.decodedIRData.command == 0x45) {
          Serial.println("B1");
          currentState = 0;
          writeLeds(B1R, B1G, B1B, false);
        } else if (IrReceiver.decodedIRData.command == 0x49) {
          Serial.println("B2");
          currentState = 0;
          writeLeds(B2R, B2G, B2B, false);
        } else if (IrReceiver.decodedIRData.command == 0x4D) {
          Serial.println("B3");
          currentState = 0;
          writeLeds(B3R, B3G, B3B, false);
        } else if (IrReceiver.decodedIRData.command == 0x1E) {
          Serial.println("B4");
          currentState = 0;
          writeLeds(B4R, B4G, B4B, false);
        } else if (IrReceiver.decodedIRData.command == 0x1A) {
          Serial.println("B5");
          currentState = 0;
          writeLeds(B5R, B5G, B5B, false);
  
        } else if (IrReceiver.decodedIRData.command == 0x44) {
          Serial.println("W1");
          currentState = 0;
          writeLeds(W1R, W1G, W1B, false);

        } else if (IrReceiver.decodedIRData.command == 0x48) {
          Serial.println("W2");
          currentState = 0;
          writeLeds(W2R, W2G, W2B, false);
          
        } else if (IrReceiver.decodedIRData.command == 0x4C) {
          Serial.println("W3");
          currentState = 0;
          writeLeds(W3R, W3G, W3B, false);
          
        } else if (IrReceiver.decodedIRData.command == 0x1F) {
          Serial.println("W4");
          currentState = 0;
          writeLeds(W4R, W4G, W4B, false);
          
        } else if (IrReceiver.decodedIRData.command == 0x1B) {
          Serial.println("W5");
          currentState = 0;
          writeLeds(W5R, W5G, W5B, false);
  
        } else if (IrReceiver.decodedIRData.command == 0x14) {
          Serial.println("RU");
          diyRup();
        } else if (IrReceiver.decodedIRData.command == 0x10) {
          Serial.println("RD");
          diyRdown();
        } else if (IrReceiver.decodedIRData.command == 0x15) {
          Serial.println("GU");
          diyGup();
        } else if (IrReceiver.decodedIRData.command == 0x11) {
          Serial.println("GD");
          diyGdown();
        } else if (IrReceiver.decodedIRData.command == 0x16) {
          Serial.println("BU");
          diyGup();
        } else if (IrReceiver.decodedIRData.command == 0x12) {
          Serial.println("BD");
          diyGdown();
  
        } else if (IrReceiver.decodedIRData.command == 0xC) {
          Serial.println("DIY1");
          currentState = 1;
          updateLeds();
        } else if (IrReceiver.decodedIRData.command == 0xD) {
          Serial.println("DIY2");
          currentState = 2;
          updateLeds();
        } else if (IrReceiver.decodedIRData.command == 0xE) {
          Serial.println("DIY3");
          currentState = 3;
          updateLeds();
        } else if (IrReceiver.decodedIRData.command == 0x8) {
          Serial.println("DIY4");
          currentState = 4;
          updateLeds();
        } else if (IrReceiver.decodedIRData.command == 0x9) {
          Serial.println("DIY5");
          currentState = 5;
          updateLeds();
        } else if (IrReceiver.decodedIRData.command == 0xA) {
          Serial.println("DIY6");
          currentState = 6;
          updateLeds();
  
        } else if (IrReceiver.decodedIRData.command == 0x4) {
          Serial.println("JUMP3");
          currentState = 7;
        } else if (IrReceiver.decodedIRData.command == 0x5) {
          Serial.println("JUMP7");
          currentState = 7;
        } else if (IrReceiver.decodedIRData.command == 0x6) {
          Serial.println("FADE3");
          currentState = 7;
        } else if (IrReceiver.decodedIRData.command == 0x7) {
          Serial.println("FADE7");
          currentState = 7;
        } else if (IrReceiver.decodedIRData.command == 0xB) {
          Serial.println("FLASH");
        } else if (IrReceiver.decodedIRData.command == 0xF) {
          Serial.println("AUTO");
  
        } else if (IrReceiver.decodedIRData.command == 0x17) {
          Serial.println("QUICK");
          quick();
        } else if (IrReceiver.decodedIRData.command == 0x13) {
          Serial.println("SLOW");
          slow();
        }
      }
    }
  }



  /*if (iscurrentlyon && !constantcolor) {
    // 0 = j3, 1 = j7, 2 = f3, 3 = f7
    
    if (counteri < 2048) {
      counteri++;
      if (counteri <= 1024) {
        counterj++;
      } else {
        counterj--;
      }
    } else if (counteri >= 2048) {
      counteri = 0;
    }
    switch(modeforj) {
      case 0: //j3
        
      break; 
        
      case 1: //j7
        
      break;
      
      case 2: //f3
        
      break;
      
      case 3: //f7
        writeLeds(phaseshift(counterj - 100), phaseshift(counterj - 1024), counterj);
      break;
    }
    
    delay(1 * slowfactor);
  }*/

}
