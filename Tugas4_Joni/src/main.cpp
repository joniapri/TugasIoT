#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <EEPROM.h>

#define LED1_HIJAU1 2
#define LED2_MERAH 4
#define LED3_PUTIH 16
#define LED4_HIJAU2 17
#define TOMBOL_AUTO 15

#define BH1750_ADDRESS 0x23
#define BH1750_DATALEN 2
#define EEPROM_SIZE 96
#define AUTOBRIGHTNESS_ADDRESS 0
#define SSIDPASS_ADDRESS 32
#define SSIDPASS_SIZE 64

bool autoBrightness = false, buttonPressed = false;
byte buff[2] = "";
byte Led_Array[4] = {LED1_HIJAU1, LED2_MERAH, LED3_PUTIH, LED4_HIJAU2};
unsigned short lux = 0;
String dataReceived, ssid, password;
char ssidPassRead[SSIDPASS_SIZE];

void bh1750Request(int address);
int bh1750Read(int address);
void LedON(int LedNumber);
void readEEPROM(int address, char *data, int size);
void writeEEPROM(int address, const char *data, int size);

void IRAM_ATTR gpioISR() {
  buttonPressed = true;
}

void setup() {
  Serial.begin(9600);
  for (int i = 0; i < 4; i++) {
    pinMode(Led_Array[i], OUTPUT);
  }
  pinMode(TOMBOL_AUTO, INPUT_PULLUP);
  attachInterrupt(TOMBOL_AUTO, &gpioISR, FALLING); 

  Wire.begin();
  EEPROM.begin(EEPROM_SIZE);
  
  //Read autobrightness status
  Serial.println("[Setup] Start reading autobrightness status....");
  char readChar = EEPROM.read(0);
  if (readChar == 1) {
    Serial.println("[Setup] Autobrightness is active");
    autoBrightness = true;
  }
  else {
    Serial.println("[Setup] Autobrightness is non active");
    autoBrightness = false;
    LedON(4);
  }
  delay(1000);

  //Read SSID and Password
  readEEPROM(SSIDPASS_ADDRESS, ssidPassRead, SSIDPASS_SIZE);
  String ssidPassString = String(ssidPassRead);
  int firstPos = 0, secondPos = 0;

  firstPos = ssidPassString.indexOf(";");
  ssid = ssidPassString.substring(0, firstPos);
  Serial.println("SSID: " + ssid);

  secondPos = ssidPassString.indexOf(";", firstPos + 1);
  password = ssidPassString.substring(firstPos + 1, secondPos);
  Serial.println("Password: " + password);
}

void loop() {
  //Read Light Intensity
  bh1750Request(BH1750_ADDRESS);
  delay(200);
  if (bh1750Read(BH1750_ADDRESS) == BH1750_DATALEN) {
    lux = (((unsigned short)buff[0] << 8) | (unsigned short)buff[1]) / 1.2;
    Serial.println("Nilai intensitas cahaya = " + String(lux) + " lux");

    if (autoBrightness) {
      if (lux >= 0 && lux <= 250) {
        Serial.println("4 Lampu menyala");
        LedON(4);
      }

      else if (lux > 250 && lux <= 500) {
        Serial.println("3 Lampu menyala");
        LedON(3);
      }

      else if (lux > 500 && lux <= 750) {
        Serial.println("2 Lampu menyala");
        LedON(2);
      }

      else if (lux > 750 && lux <= 1000) {
        Serial.println("1 Lampu menyala");
        LedON(1);
      }

      else {
        Serial.println("Tidak ada Lampu menyala");
        LedON(0);
      }
      delay(1000);
    }
  }

  //check if button has been pressed and start writing to EEPROM
  if (buttonPressed) {
    if (autoBrightness) {
      Serial.println("[ISR] Turn off autobrigtness");
      EEPROM.write(0, 0);
    }
    else {
      Serial.println("[ISR] Turn on autobrigtness");
      EEPROM.write(0, 1);
    }
    EEPROM.commit();
    autoBrightness = !autoBrightness;
    buttonPressed = false;
  }

  // Chaeck data coming from PC
  while (Serial.available()) {
    char c = Serial.read();
    dataReceived += c;

    if (c == '\n') { // End of command
      Serial.println("[MAIN] Received data from user: " + dataReceived);
      writeEEPROM(SSIDPASS_ADDRESS, dataReceived.c_str(), dataReceived.length());
      dataReceived = "";
    }
  }
}

int bh1750Read(int address) {
  int i = 0;
  Wire.beginTransmission(address);
  Wire.requestFrom(address, 2);
  while (Wire.available()) {
    buff[i] = Wire.read();
    i++;
  }
  Wire.endTransmission();

  return i;
}

void bh1750Request(int address) {
  Wire.beginTransmission(address);
  Wire.write(0x10);
  Wire.endTransmission();
}

void LedON(int LedNumber) {
  for (int i = 0; i < LedNumber; i++) {
    digitalWrite(Led_Array[i], HIGH);
  }

  for (int i = 0; i < 4 - LedNumber; i++) {
    digitalWrite(Led_Array[3 - i], LOW);
  }
}
 
void readEEPROM(int address, char *data, int size) {
  Serial.println("[EEPROM] Start reading from EEPROM");
  for (int i = 0; i < size; i++) {
    data[i] = EEPROM.read(address + i);
  } 
}


void writeEEPROM(int address, const char *data, int size) {
  Serial.println("[EEPROM] Start writing to EEPROM");
  for (int i = 0; i < size; i++) {
    EEPROM.write(address + 1, data[i]);
  }
  EEPROM.commit();
}


