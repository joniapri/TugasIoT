#include <Arduino.h>
#include <Wire.h>
#include <EEPROM.h>

#define LED1_HIJAU1 2
#define LED2_MERAH 4
#define LED3_PUTIH 16
#define LED4_HIJAU2 17

#define TOMBOL_AUTO 15

#define BH1750_ADDRESS 0x23
#define BH1750_DATALEN 2

#define EEPROM_SIZE 1

int Led_Array[4] = {LED1_HIJAU1, LED2_MERAH, LED3_PUTIH, LED4_HIJAU2};
unsigned char Status_Auto = LOW;
bool changeStatusAuto = false;
void LedON(int LedNumber);
void AutoBrightness();

portMUX_TYPE gpioIntMux = portMUX_INITIALIZER_UNLOCKED;

void bh1750Request(int address);
int bh1750GetData(int address);

volatile bool statusInterrupt = false;
int totalCounterInterrupt = 0;

hw_timer_t * timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

char readData[EEPROM_SIZE], receivedData[EEPROM_SIZE];
int dataIndex =0;

char Auto;

byte buff[2];
unsigned short lux = 0;
void BacaLux();

void readEEPROM(int address, char * data);
void writeEEPROM(int address, char * data);

void IRAM_ATTR gpioISR() {
  portENTER_CRITICAL(&gpioIntMux);
  changeStatusAuto = LOW;
  portEXIT_CRITICAL(&gpioIntMux);
}

void BacaEEPROM() {
  EEPROM.begin(EEPROM_SIZE);
  delay(100);
  readEEPROM(0, readData);
  Status_Auto = EEPROM.read(0);
  Serial.println("Data EEPROM: ");
  Serial.println(readData);
  Serial.println(Status_Auto);
}

void TulisEEPROM() {
  if (changeStatusAuto == LOW  && digitalRead(TOMBOL_AUTO) == LOW ) {
    portENTER_CRITICAL(&gpioIntMux);
    changeStatusAuto = HIGH;
    portEXIT_CRITICAL(&gpioIntMux);

    Status_Auto = !Status_Auto;
   // Serial.println("Status AutoBrightness = " + String(Status_Auto));
    EEPROM.write(0, Status_Auto);
    EEPROM.commit();
   // writeEEPROM(0, Status_Auto);
   // memset(Status_Auto, 0, EEPROM_SIZE); 
  }
}

void setup() {
  BacaEEPROM();
  Wire.begin();
  Serial.begin(9600);
  for (int i = 0; i < 4; i++) {
    pinMode(Led_Array[i], OUTPUT);
  }
  pinMode(TOMBOL_AUTO, INPUT_PULLUP);
 
  attachInterrupt(TOMBOL_AUTO, &gpioISR, FALLING);
  
}

void loop() {
  TulisEEPROM();
  BacaLux();
 
  if (Status_Auto == LOW) {
    Serial.println("Status Auto Brightness TIDAK AKTIF ");
    LedON(4);
   
    Serial.println(" ");
    delay(1000);
  }

  if (Status_Auto == HIGH) {
    Serial.println("Status Auto Brightness AKTIF ");
    AutoBrightness();
    Serial.println(" ");
    delay(1000);
  }

}

void LedON(int LedNumber) {
  for (int i = 0; i < LedNumber; i++) {
    digitalWrite(Led_Array[i], HIGH);
  }

  for (int i = 0; i < 4 - LedNumber; i++) {
    digitalWrite(Led_Array[3-i], LOW);
  }
}
     
void AutoBrightness() {
  if (lux >= 0 && lux < 250) {
    LedON(4);
    Serial.println("4 Lampu menyala");
  }

  if (lux >= 250 && lux < 500) {
    LedON(3);
    Serial.println("3 Lampu menyala");
  }

  if (lux >= 500 && lux < 750) {
    LedON(2);
    Serial.println("2 Lampu menyala");
  }

  if (lux >= 750 && lux < 1000) {
    LedON(1);
    Serial.println("1 Lampu menyala");
  }

  if (lux >= 1000) {
    LedON(0);
    Serial.println("Tidak ada Lampu menyala");
  }
}

void BacaLux() {
  bh1750Request(BH1750_ADDRESS);
  delay(200);
  if (bh1750GetData(BH1750_ADDRESS) == BH1750_DATALEN) {
    lux = (((unsigned short)buff[0] << 8) | (unsigned short)buff[1]) / 1.2;
    Serial.println("Nilai intensitas cahaya = " + String(lux) + " lux");
  }
  delay(1000);  
}

void bh1750Request(int address) {
  Wire.beginTransmission(address);
  Wire.write(0x10);
  Wire.endTransmission();
}

int bh1750GetData(int address) {
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

void readEEPROM(int address, char * data) {
  for (int i = 0; i < EEPROM_SIZE; i++) {
    data[i] = EEPROM.read(address + 1);
  }
}

void writeEEPROM(int address, char * data) {
  Serial.println("Mulai tulis ke EEPROM");
  for (int i = 0; i < EEPROM_SIZE; i++) {
    EEPROM.write(address + 1, data[i]);
  }
  EEPROM.commit();

}