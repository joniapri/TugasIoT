#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <DHT.h>
#include <BlueToothSerial.h>
#include <EEPROM.h>


#define LED_PIN 18
#define PWM_CHANNEL 0
#define PWM_FREQ 5000
#define PWM_RESOLUTION 8
#define TOMBOL_AUTO 15

#define BH1750_ADDRESS 0x23
#define BH1750_DATALEN 2
#define DHT_PIN 19
#define DHT_TYPE DHT11

#define EEPROM_SIZE 96
#define SSIDPASS_ADDRESS 32
#define SSIDPASS_SIZE 64

bool statusAuto = false, buttonPressed = false;

void AutoBrightness();

void bh1750Request(int address);
int bh1750GetData(int address);

byte buff[2];
unsigned short lux = 0, dutyCycle = 0;

char Otomatis[EEPROM_SIZE], DataSsidPass[EEPROM_SIZE];
String DataReceived, Ssid, Password;

BluetoothSerial SerialBT;
DHT dht(DHT_PIN, DHT_TYPE);
float temperature = 0, bTemperature = 0, humidity = 0, bhumidity = 0;
int lastTransmit = 0;

void BacaLux();
void BacaSuhuHumidityLux();
void BacaEEPROM();
void CekStatusAuto();
void readEEPROM(int address, char *data, int size);
void BacaPerubahanTombol(); 
void BacaStatusSerial(); 
void writeEEPROM(int address, const char *data, int size);
void BacaSsidPassword();

String dataBluetooth, receivedString;
void updateDhtData();

void IRAM_ATTR gpioISR() {
  buttonPressed = true;
}


void setup() {
  // put your setup code here, to run once:
  Wire.begin();
  Serial.begin(9600);
  SerialBT.begin("ESP32-BTClassic");
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUILTIN_LED, OUTPUT);
  attachInterrupt(TOMBOL_AUTO, &gpioISR, FALLING);
  
  ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(LED_PIN, PWM_CHANNEL);
  dht.begin();
  
  CekStatusAuto();
  BacaSsidPassword();

}

void loop() {
  // put your main code here, to run repeatedly:
  BacaLux();
  BacaSuhuHumidityLux();
  BacaPerubahanTombol();
  AutoBrightness();
  BacaStatusSerial();
  updateDhtData();  

}

void BacaLux() { //OK
  bh1750Request(BH1750_ADDRESS);
  delay(100);
  if (bh1750GetData(BH1750_ADDRESS) == BH1750_DATALEN) {
    lux = (((unsigned short)buff[0] << 8) | (unsigned short)buff[1]) / 1.2;
    Serial.print("Nilai intensitas cahaya = " + String(lux) + " lux");
  }

  if (lux >= 0 && lux <=2500) {
    dutyCycle = 250 - (lux/10);
  } else {
    dutyCycle = 0;
  }
  Serial.println("  Nilai duty Cycle = " + String(dutyCycle) + " PWM");

  delay(100);  
}

void AutoBrightness() { //OK
  if (statusAuto) {
   ledcWrite(PWM_CHANNEL, dutyCycle);  
   Serial.println("Otomatis = ON");
  }
  else {
    ledcWrite(PWM_CHANNEL, 0);
    Serial.println("Otomatis = OFF");
  }

}

void bh1750Request(int address) { //OK
  Wire.beginTransmission(address);
  Wire.write(0x10);
  Wire.endTransmission();
}

int bh1750GetData(int address) {  //OK
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

void BacaSuhuHumidityLux() {
  String sentence;
  if (SerialBT.available()) {
    char receivedChar = SerialBT.read();
    receivedString += receivedChar;
    if (receivedChar == '\r') {
      if (receivedString == "TEMP\r") {
        if (millis() - lastTransmit > 5000) {
          temperature = dht.readTemperature();
          String sentence = "data suhu -> Suhu: " + String (temperature) + " C";
        //  Serial.println(sentence);
          SerialBT.println(sentence);
          lastTransmit = millis();
        }   
      }
          
      else if (receivedString == "HUMID\r") {
        if (millis() - lastTransmit > 5000) {
          humidity = dht.readHumidity();
          String sentence = "data humidity -> humidity: " + String (humidity) + " %";
        //  Serial.println(sentence);
          SerialBT.println(sentence);
          lastTransmit = millis();
        }      
      }

      else if (receivedString == "LUX\r") {
        if (millis() - lastTransmit > 5000) {
          String sentence = "data lux -> lux: " + String (lux);
        //  Serial.println(sentence);
          SerialBT.println(sentence);
          lastTransmit = millis();
        }      
      }

      else if (receivedString == "AUTOBRIGHT,ON\r") {
        if (millis() - lastTransmit > 5000) {
          statusAuto = true;
          String sentence = "Autobgright = ON";
          SerialBT.println(sentence);
          lastTransmit = millis();
        }      
      }
      
      else if (receivedString == "AUTOBRIGHT,OFF\r") {
        if (millis() - lastTransmit > 5000) {
          statusAuto = false;
          String sentence = "Autobgright = OFF";
          SerialBT.println(sentence);
          lastTransmit = millis();
        }      
      }
      
      else {
        if (millis() - lastTransmit > 5000) {   
          String sentence = "Teks Tidak sesuai Format";
        //  Serial.println(sentence);
          SerialBT.println(sentence);
          lastTransmit = millis();
        }      
      }

      receivedString = "";
      SerialBT.flush();
    }
  }
}

void updateDhtData() { 
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();
  Serial.print("Suhu : " + String(temperature) + " C");
  Serial.println("  Humidity : " + String(humidity) + " %");
  Serial.println("");
  delay(500);
}

void BacaSsidPassword() {
  EEPROM.begin(EEPROM_SIZE);
  readEEPROM(SSIDPASS_ADDRESS, DataSsidPass, SSIDPASS_SIZE);
  String DataStringSsidPass = String(DataSsidPass);
  int petikPertama = 0, petikKedua = 0;
  
  petikPertama = DataStringSsidPass.indexOf(";");
  Ssid = DataStringSsidPass.substring(0, petikPertama);
  Serial.println("SSID = " + Ssid);

  petikKedua = DataStringSsidPass.indexOf(";", petikPertama + 1);
  Password = DataStringSsidPass.substring(petikPertama + 1, petikKedua);
  Serial.println("Password = " + Password);
<<<<<<< HEAD
=======
}

void readEEPROM(int address, char *Data, int size) {
  Serial.println("[EEPROM] mulai membaca dari EEPROM");
  for(int i = 0; i < size; i++) {
    Data[i] = EEPROM.read(address + i);
  }
}

void CekStatusAuto() {   //OK
  EEPROM.begin(128);
  delay(100);
  char Otomatis = EEPROM.read(0);
  Serial.println("Read Otomatis : " + char(Otomatis));
  if (Otomatis == '1') { 
    Serial.println("Otomatis = ON");
    statusAuto = 1;
    digitalWrite(BUILTIN_LED, HIGH);  
  }
  else {
    Serial.println("Otomatis = OFF");
    statusAuto = 0;
    digitalWrite(BUILTIN_LED, LOW);  
  }
}

void BacaPerubahanTombol() {  //OK
  if (buttonPressed) {
    if (statusAuto) {
      Serial.println("[ISR] mematikan auto");
      EEPROM.write(0, '0');
    }
    else {
      Serial.println("[ISR] menghidupkan auto");
      EEPROM.write(0, '1');
    }
    EEPROM.commit();
    statusAuto = !statusAuto;
    buttonPressed = false;
  }
}

void BacaStatusSerial() {
  while (Serial.available()) {
    char DataSerial = Serial.read();
    DataReceived += DataSerial;
    
    if (DataSerial == '\n') {
      Serial.println("Data diterima dari user: " + DataReceived);
      writeEEPROM(SSIDPASS_ADDRESS, DataReceived.c_str(), DataReceived.length());
      DataReceived ="";
    }
  }
}

void writeEEPROM(int address, const char *data, int size) {
  Serial.println("[EEPROM] mulai menulis ke EEPROM");
  for (int i = 0; i < size; i++) {
    EEPROM.write(address + i, data[i]);
  }
  EEPROM.commit();
>>>>>>> 8d4a54ab02e09ad41b50b82bbbdfba7e6382fd3c
}

void readEEPROM(int address, char *Data, int size) {
  Serial.println("[EEPROM] mulai membaca dari EEPROM");
  for(int i = 0; i < size; i++) {
    Data[i] = EEPROM.read(address + i);
  }
}

void CekStatusAuto() {   //OK
  EEPROM.begin(128);
  delay(100);
  char Otomatis = EEPROM.read(0);
  Serial.println("Read Otomatis : " + char(Otomatis));
  if (Otomatis == '1') { 
    Serial.println("Otomatis = ON");
    statusAuto = 1;
    digitalWrite(BUILTIN_LED, HIGH);  
  }
  else {
    Serial.println("Otomatis = OFF");
    statusAuto = 0;
    digitalWrite(BUILTIN_LED, LOW);  
  }
}

void BacaPerubahanTombol() {  //OK
  if (buttonPressed) {
    if (statusAuto) {
      Serial.println("[ISR] mematikan auto");
      EEPROM.write(0, '0');
    }
    else {
      Serial.println("[ISR] menghidupkan auto");
      EEPROM.write(0, '1');
    }
    EEPROM.commit();
    statusAuto = !statusAuto;
    buttonPressed = false;
  }
}

void BacaStatusSerial() {
  while (Serial.available()) {
    char DataSerial = Serial.read();
    DataReceived += DataSerial;
    
    if (DataSerial == '\n') {
      Serial.println("Data diterima dari user: " + DataReceived);
      writeEEPROM(SSIDPASS_ADDRESS, DataReceived.c_str(), DataReceived.length());
      DataReceived ="";
    }
  }
}

void writeEEPROM(int address, const char *data, int size) {
  Serial.println("[EEPROM] mulai menulis ke EEPROM");
  for (int i = 0; i < size; i++) {
    EEPROM.write(address + i, data[i]);
  }
  EEPROM.commit();
}