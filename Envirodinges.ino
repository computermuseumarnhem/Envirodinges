
#include <SPI.h>
#include <Ethernet2.h>
#include <EthernetUdp2.h>
#include <PrintEx.h>
#include <Wire.h>
#include <Adafruit_AM2315.h>

PrintEx PSerial = Serial;

#define I2C_ROMADDRESS 0x53
#define I2C_TCAADDR 0x70

bool sensor_present[8];

Adafruit_AM2315 am2315_1;
Adafruit_AM2315 am2315_2;
Adafruit_AM2315 am2315_3;
Adafruit_AM2315 am2315_4;


void set_i2c_mux(byte i2c_sel) {
  if (i2c_sel > 7) return;
 
  Wire.beginTransmission(I2C_TCAADDR);
  Wire.write(1 << i2c_sel);
  Wire.endTransmission();  
  PSerial.printf("mux: %d - sensor: %d - ", i2c_sel, i2c_sel);
  delay(250);
}
void setup() {
  byte b;
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }

  Wire.begin();  
  for (b=0; b<=7; b++) {
    sensor_present[b] = false;
    set_i2c_mux(b);
    delay(500);
     switch (b) {
      case 4:
        sensor_present[b] = am2315_1.begin();
        break;
      case 5:
        sensor_present[b] = am2315_2.begin();
        break;
      case 6:
        sensor_present[b] = am2315_3.begin();
        break;
      case 7:
        sensor_present[b] = am2315_4.begin();
        break;
    }
    if (sensor_present[b]) {
      Serial.println("OK");
    }
    else {
      Serial.println("Sensor not found, check wiring & pullups!");
    }
    delay(1000);
  }
}

void loop() {
  // put your main code here, to run repeatedly:

}
