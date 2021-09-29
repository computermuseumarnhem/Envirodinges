#define CODEVERSION "0.4"

#define MOSFET  10
#define ACTIVE_LED  9
#define ERROR_LED   8
#define CONTAINER_SELECT  5

#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <PrintEx.h>
#include <Wire.h>
#include <Adafruit_AM2315.h>
#include <math.h>

PrintEx PSerial = Serial;
PrintEx PSerial1 = Serial1;

#define I2C_ROMADDRESS 0x53
#define I2C_TCAADDR 0x70
#define W5500_CS  7

bool sensor_present[8], b_network, b_serial, b_container;
int led = ACTIVE_LED;  // the PWM pin the LED is attached to
int brightness = 0;    // how bright the LED is
int fadeAmount = 5;    // how many points to fade the LED by
long nextrun = 0;

#define RUNINTERVAL 60000

Adafruit_AM2315 am2315_1;
Adafruit_AM2315 am2315_2;
Adafruit_AM2315 am2315_3;
Adafruit_AM2315 am2315_4;

static uint8_t mac[] = { 0x90, 0xA2, 0xDA, 0x42, 0x42, 0x00 };

int ContainerNum;

void set_i2c_mux(byte i2c_sel) {
  char outstr[40];
  if (i2c_sel > 7) return;
 
  Wire.beginTransmission(I2C_TCAADDR);
  Wire.write(1 << i2c_sel);
  Wire.endTransmission();  
  PSerial.printf("mux: %d - sensor: %d - ", i2c_sel, i2c_sel);
//  printserial(outstr);
  delay(250);
}

void fanswitch(bool state)
{
  digitalWrite(MOSFET, state);
}

//void printserial(char outstr[40])
//{
//  if (b_serial) { Serial.print(outstr); };
//  Serial1.print(outstr);
//}

void printIPAddress()
{
  char outstr[40];
  IPAddress myIp = Ethernet.localIP();
  PSerial.printf("IP address: %d.%d.%d.%d\n", myIp[0], myIp[1], myIp[2], myIp[3]);
  //printserial(outstr);
}

void setup() {
  byte b;
  char outstr[40];
  b_network = false;

  Serial.begin(9600);
  Serial1.begin(9600);
  delay(2000);
  b_serial = (Serial);
  
  pinMode(MOSFET, OUTPUT);
  pinMode(ACTIVE_LED, OUTPUT);
  pinMode(ERROR_LED, OUTPUT);
  digitalWrite(ACTIVE_LED, true);
  digitalWrite(ERROR_LED, true);
  digitalWrite(MOSFET, true);

  pinMode(CONTAINER_SELECT, INPUT_PULLUP);
  if (digitalRead(CONTAINER_SELECT) == HIGH) {
     mac[5] = 0x01;  
     ContainerNum = 1;
  }
  else
  {
     mac[5] = 0x02;  
     ContainerNum = 2;
  }
  PSerial.printf("\nEnvirodinges %s, Container %d\n\n", CODEVERSION, ContainerNum); 
  //printserial(outstr);

  Ethernet.init(W5500_CS);  
  Wire.begin();  
  for (b=0; b<=7; b++) {
    sensor_present[b] = false;
    if (b >= 4) {
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
//        sprintf(outstr, "%s\n\0", "OK");
          PSerial.printf("OK\n");
      }
      else {
//        sprintf(outstr, "%s\n\0", "Sensor not found, check wiring & pullups!");
        PSerial.printf("%s\n", "Sensor not found, check wiring & pullups!");
      }
//      printserial(outstr);
      delay(1000);     
    }
  }
  PSerial.printf("%s\n\n\n", "Envirodinges starting");
//  printserial(outstr); 
  PSerial.printf("Setting MAC: %02x:%02x:%02x:%02x:%02x:%02x\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
//  printserial(outstr);

  // start Ethernet and UDP
  auto link = Ethernet.linkStatus();

  switch (link) {
    case Unknown:
      PSerial.printf("Link status: %s\n", "Unknown");
      break;
    case LinkON:
      PSerial.printf("Link status: %s\n", "UP");
      b_network = true;
      break;
    case LinkOFF:
      PSerial.printf("Link status: %s\n", "DOWN");
      break;
  }
//  printserial(outstr);
  if (b_network) {
    if (Ethernet.begin(mac) == 0) {
      b_network = false;
      PSerial.printf("%s\n", "Failed to configure Ethernet using DHCP");
//      printserial(outstr);
    }
    else {
      printIPAddress();
      b_network = true;
      digitalWrite(ERROR_LED, false);
    }
  }
}

float dewpoint(float t, float h) {
  //  A = Log(RH / 100) / Log(2.718282) + (17.62 * Ta / (243.12 + Ta))
  //  Td = 243.12 * A / (17.62 - A) grdC
  
  float tdew, a;

  a = log(h/100) / (log(2.718282) + (17.62 * t / (243.12 + t)));
  tdew = 243.12 * a / (17.62 - a);
  return tdew;
}

void loop() {
  // put your main code here, to run repeatedly:

  int m;
  float temps[8], humids[8], tdew[8];
  
  if (millis() > nextrun) {
    nextrun = millis() + RUNINTERVAL;

    for (m=0; m<=7; m++) {
      if (sensor_present[m]) {
        set_i2c_mux((m));
        delay(200);
        switch (m) {
          case 4:
            am2315_1.readTemperatureAndHumidity(&temps[4], &humids[4]);
            break;
          case 5:
            am2315_2.readTemperatureAndHumidity(&temps[5], &humids[5]);
            break;
          case 6:
            am2315_3.readTemperatureAndHumidity(&temps[6], &humids[6]);
            break;
          case 7:
            am2315_4.readTemperatureAndHumidity(&temps[7], &humids[7]);
            break;
        }
        tdew[m] = dewpoint(temps[m], humids[m]);
        PSerial.printf("Hum:  %2.1f    Temp: %2.1f   Dewpoint: %2.1f\n", humids[m], temps[m], tdew[m]); 
      }
    }
  }
  analogWrite(led, brightness);

  // change the brightness for next time through the loop:
  brightness = brightness + fadeAmount;

  // reverse the direction of the fading at the ends of the fade:
  if (brightness <= 0 || brightness >= 255) {
    fadeAmount = -fadeAmount;
  }
  // wait for 30 milliseconds to see the dimming effect
  delay(30);

  if (fadeAmount > 0) {
    fanswitch(true);
  }
  else {
    fanswitch(false);
  }
}
