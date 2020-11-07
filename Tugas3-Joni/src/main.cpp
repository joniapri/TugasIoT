#include <Wire.h>

#define BHI1750_ADDRESS 0x23
#define BHI1750_DATALEN 2
#define LED1_HIJAU1 2
#define LED2_MERAH 4
#define LED3_PUTIH 16
#define LED4_HIJAU2 17
#define TOMBOL_AUTO 15

bool Auto_Status = false;

int LedArray[4] = {LED1_HIJAU1, LED2_MERAH, LED3_PUTIH, LED4_HIJAU2};

void bh1750Request (int address);
int bh1750GetData (int address);

byte buff[2];
unsigned short lux = 0;
void LedON(int LedNumber);

void setup() {
  Wire.begin();
  Serial.begin(9600);
  for (int i = 0; i < 4; i++) {
    pinMode(LedArray[i], OUTPUT);
  }
  pinMode(TOMBOL_AUTO, INPUT_PULLUP);
  
}

void LedON(int LedNumber) {
  for (int i = 0; i < LedNumber; i++) {
    digitalWrite(LedArray[i], HIGH);
    }
  
  for (int i = 0; i < 4 - LedNumber; i++) {
    digitalWrite(LedArray[3-i], LOW);
    }
  
}

void AutoBrighness() {
  if ((lux) >= 0 && (lux) < 250 ) {
   LedON(4);
  }

  if ((lux) >= 250 && (lux) < 500 ) {
    LedON(3);
  }

  if ((lux) >= 500 && (lux) < 750 ) {
    LedON(2);
  }

  if ((lux) >= 750 && (lux) < 1000 ) {
    LedON(1);
  }

  if ((lux) >= 1000 ) {
   LedON(0);
  }
}

void loop() {
  bh1750Request(BHI1750_ADDRESS);
  delay(200);

  if (digitalRead(TOMBOL_AUTO) == LOW) {
    Auto_Status = !Auto_Status;
  }

  if (bh1750GetData(BHI1750_ADDRESS) == BHI1750_DATALEN) {
    lux = (((unsigned short)buff[0] << 8) | (unsigned short)buff[1]) / 1.2;
    String sentence = "Nilai intensitas cahaya : " + String(lux) + " lux";
    Serial.println(sentence);
  }
  if (Auto_Status == HIGH) {
    AutoBrighness();
  }  
  else {
    LedON(4);
  }
  
  Serial.println("Status Autobrightness = " + String(Auto_Status));
  delay (1000);

}

void bh1750Request(int address) {
  Wire.beginTransmission(address);
  Wire.write(0x10);
  Wire.endTransmission();
}
int bh1750GetData (int address) {
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
