#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <DHT.h>
#include <BlueToothSerial.h>

#define LED_PIN 18
#define PWM_CHANNEL 0
#define PWM_FREQ 5000
#define PWM_RESOLUTION 8

#define BH1750_ADDRESS 0x23
#define BH1750_DATALEN 2
#define DHT_PIN 19
#define DHT_TYPE DHT11

void AutoBrightness();

void bh1750Request(int address);
int bh1750GetData(int address);

byte buff[2];
unsigned short lux = 0, dutyCycle = 0;

BluetoothSerial SerialBT;
DHT dht(DHT_PIN, DHT_TYPE);
float temperature = 0, humidity = 0;
int lastTransmit = 0;

void BacaLux();
void BacaSuhuHumidity();

String receivedString;
void updateDhtData();


void setup() {
  // put your setup code here, to run once:
  Wire.begin();
  Serial.begin(9600);
  SerialBT.begin("ESP32-BTClassic");
  pinMode(LED_PIN, OUTPUT);
  ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(LED_PIN, PWM_CHANNEL);
  dht.begin();

}

void loop() {
  // put your main code here, to run repeatedly:
  BacaLux();
  AutoBrightness();
  BacaSuhuHumidity();

}

void BacaLux() {
  bh1750Request(BH1750_ADDRESS);
  delay(100);
  if (bh1750GetData(BH1750_ADDRESS) == BH1750_DATALEN) {
    lux = (((unsigned short)buff[0] << 8) | (unsigned short)buff[1]) / 1.2;
    Serial.println("Nilai intensitas cahaya = " + String(lux) + " lux");
  }

  if (lux >= 0 && lux <=2500) {
    dutyCycle = 250 - (lux/10);
  } else {
    dutyCycle = 0;
  }
  Serial.println("Nilai duty Cycle = " + String(dutyCycle) + " PWM");

  delay(1000);  
}

void AutoBrightness() {
  ledcWrite(PWM_CHANNEL, dutyCycle);
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

void BacaSuhuHumidity() {
  if (SerialBT.available()) {
    char receivedChar = SerialBT.read();
    receivedString += receivedChar;
  //receivedData[dataIndex] = SerialBT.read();
  //dataIndex++;

    if (receivedString == "TEMP") {
    //if (receivedData[dataIndex - 1] == 'TEMP\r\n') {
      if (millis() - lastTransmit > 5000) {
        //updateDhtData();
        temperature = dht.readTemperature();
        String sentence = "data sensor -> Suhu: " + String (temperature) + " C";
        Serial.println(sentence);
        SerialBT.println(sentence);
        lastTransmit = millis();
      }
    }
  }
}

void updateDhtData() {
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();
}
