#define CODEVERSION "0.5"

#define MOSFET            10
#define ACTIVE_LED        9
#define ERROR_LED         8
#define CONTAINER_SELECT  5

#include <SPI.h>
#include <Ethernet.h>
#include <PrintEx.h>
#include <Wire.h>
#include <Adafruit_AM2315.h>
#include <math.h>
#include <Adafruit_MQTT.h>

PrintEx PSerial = Serial;
PrintEx PSerial1 = Serial1;

#define I2C_ROMADDRESS  0x53
#define I2C_TCAADDR     0x70
#define W5500_CS        7

#define SENSORCOUNT_3
#define I2CMUX_L    4
#define I2CMUX_H    6

bool sensor_present[8], b_network, b_serial, b_container;
byte brightness = 0;    // how bright the LED is
byte fadeAmount = 5;    // how many points to fade the LED by
long nextrun = 0;
long nextfade = 0;

#define RUNINTERVAL   60000
#define FADEINTERVAL  30

#define TDEW_DIFF     10.0  // fan off if dewpoint too close to temperature
#define HUMID_MAX     75.0  // fan on if humidity inside exceeded

Adafruit_AM2315 am2315_1;
Adafruit_AM2315 am2315_2;
Adafruit_AM2315 am2315_3;
#ifndef SENSORCOUNT_3
  Adafruit_AM2315 am2315_4;
#endif

static uint8_t mac[] = { 0x90, 0xA2, 0xDA, 0x42, 0x42, 0x00 };

byte ContainerNum;

void set_i2c_mux(byte i2c_sel) {
  if (i2c_sel > I2CMUX_H) return;
 
  Wire.beginTransmission(I2C_TCAADDR);
  Wire.write(1 << i2c_sel);
  Wire.endTransmission();  
  if (Serial) { PSerial.printf("mux: %d - sensor: %d - ", i2c_sel, i2c_sel); };
  PSerial1.printf("mux: %d - sensor: %d - ", i2c_sel, i2c_sel);
  delay(250);
}

void fanswitch(bool state)
{
  digitalWrite(MOSFET, state);
}

void printIPAddress()
{
  IPAddress myIp = Ethernet.localIP();
  if (Serial) { PSerial.printf("IP address: %d.%d.%d.%d\n", myIp[0], myIp[1], myIp[2], myIp[3]); };
  PSerial1.printf("IP address: %d.%d.%d.%d\n", myIp[0], myIp[1], myIp[2], myIp[3]);
}

void setup() {
  byte b;
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
  if (Serial) { PSerial.printf("\nEnvirodinges %s, Container %d\n\n", CODEVERSION, ContainerNum); };
  PSerial1.printf("\nEnvirodinges %s, Container %d\n\n", CODEVERSION, ContainerNum); 

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
#ifndef SENSORCOUNT_3          
        case 7:
          sensor_present[b] = am2315_4.begin();
          break;
#endif          
      }
      if (sensor_present[b]) {
          if (Serial) { PSerial.printf("OK\n"); };
          PSerial1.printf("OK\n");
      }
      else {
        if (Serial) { PSerial.printf("%s\n", "Sensor not found, check wiring & pullups!"); };
        PSerial1.printf("%s\n", "Sensor not found, check wiring & pullups!"); 
      }
      delay(1000);     
    }
  }
  if (Serial) {
    PSerial.printf("%s\n\n\n", "Envirodinges starting");
    PSerial.printf("Setting MAC: %02x:%02x:%02x:%02x:%02x:%02x\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  }
  PSerial1.printf("%s\n\n\n", "Envirodinges starting");
  PSerial1.printf("Setting MAC: %02x:%02x:%02x:%02x:%02x:%02x\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  // start Ethernet and UDP
  auto link = Ethernet.linkStatus();

  switch (link) {
    case Unknown:
      if (Serial) { PSerial.printf("Link status: %s\n", "Unknown"); };
      PSerial1.printf("Link status: %s\n", "Unknown");
      break;
    case LinkON:
      if (Serial) { PSerial.printf("Link status: %s\n", "UP"); };
      PSerial1.printf("Link status: %s\n", "UP");
      b_network = true;
      break;
    case LinkOFF:
      if (Serial) { PSerial.printf("Link status: %s\n", "DOWN"); };
      PSerial1.printf("Link status: %s\n", "DOWN");
      break;
  }

  if (b_network) {
    if (Ethernet.begin(mac) == 0) {
      b_network = false;
      if (Serial) { PSerial.printf("%s\n", "Failed to configure Ethernet using DHCP"); };
      PSerial1.printf("%s\n", "Failed to configure Ethernet using DHCP"); 
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
  float tint_min, tint_max, hint_max, tdew_max, tdew_fan;
  bool fan_on = false;
  
  if (millis() > nextrun) {
    nextrun += RUNINTERVAL;

    for (m=0; m<=7; m++) {
      if (sensor_present[m]) {
        set_i2c_mux((m));
        delay(200);
        switch (m) {
          case 4:   // external sensor
            am2315_1.readTemperatureAndHumidity(&temps[4], &humids[4]);
            break;
          case 5:   // internal sensor, inlet
            am2315_2.readTemperatureAndHumidity(&temps[5], &humids[5]);
            break;
          case 6:   // internal sensor, outlet
            am2315_3.readTemperatureAndHumidity(&temps[6], &humids[6]);
            break;
#ifndef SENSORCOUNT_3            
          case 7:
            am2315_4.readTemperatureAndHumidity(&temps[7], &humids[7]);
            break;
#endif
        }
        tdew[m] = dewpoint(temps[m], humids[m]);
        PSerial.printf("Hum:  %2.1f    Temp: %2.1f   Dewpoint: %2.1f\n", humids[m], temps[m], tdew[m]); 
      }
    }
    tint_max = max(temps[5], temps[6]);       // highest temp inside
    tint_min = min(temps[5], temps[6]);       // lowest temp inside
    hint_max = max(humids[5], humids[6]);     // highest humidity inside
    tdew_max = dewpoint(tint_min, hint_max);  // highest dewpoint inside
    tdew_fan = dewpoint(tint_min, humids[4]); // inside temp against outside humidity
    fan_on = (hint_max > HUMID_MAX);          // fan on if humid inside
    fan_on = (fan_on && ((tint_min - tdew_fan) < TDEW_DIFF)); // ... unless chance of condensation 
    fanswitch(fan_on);
  }
  if (millis() > nextfade) {
    nextfade += 30;   
    analogWrite(ACTIVE_LED, brightness);
  
    // change the brightness for next time through the loop:
    brightness = brightness + fadeAmount;
  
    // reverse the direction of the fading at the ends of the fade:
    if (brightness <= 0 || brightness >= 255) {
      fadeAmount = -fadeAmount;
    }
    // wait for 30 milliseconds to see the dimming effect
    fanswitch((fadeAmount > 0)); // for testing
  }
}
