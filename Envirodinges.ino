#define CODEVERSION "0.7"

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

enum errorlvl { none, no_net, no_sor23, no_sensor1 } ;

bool b_sensor_present[4], b_network, b_container, b_sensorcount;
byte sensor_detected = 0;
byte brightness = 0;    // how bright the LED is
byte fadeAmount = 5;    // how many points to fade the LED by
long nextrun = 0;
long nextfade = 0;
long nexterrorled = 0;

#define RUNINTERVAL   60000
#define FADEINTERVAL  30

#define TDEW_DIFF     10.0  // fan off if dewpoint too close to temperature
#define HUMID_MAX     75.0  // fan on if humidity inside exceeded

// #define SENSORCOUNT_4  // only if 4 AM2315 sensors
#define MUX_OFFSET  3
#define I2CMUX_L    4

Adafruit_AM2315 am2315_1;
Adafruit_AM2315 am2315_2;
Adafruit_AM2315 am2315_3;
#ifdef SENSORCOUNT_4
  const byte sensor_required = 4;  
  Adafruit_AM2315 am2315_4;
  #define I2CMUX_H    7
#else
  const byte sensor_required = 3; 
  #define I2CMUX_H    6 
#endif

static uint8_t mac[] = { 0x90, 0xA2, 0xDA, 0x42, 0x42, 0x00 };
static char outline[80];

byte ContainerNum;

void set_i2c_mux(byte i2c_sel) {
  if ((i2c_sel < I2CMUX_L) || (i2c_sel > I2CMUX_H)) return;
 
  Wire.beginTransmission(I2C_TCAADDR);
  Wire.write(1 << i2c_sel);
  Wire.endTransmission();  
  delay(250);
}

void fanswitch(bool state)
{
  digitalWrite(MOSFET, state);
}

//void output(byte len)
//{
//  char buf[80];
//  memset(buf, 0, 80);
//  strncpy(buf, outline, len);
//  if (Serial) Serial.println(buf);
//  Serial1.println(buf);
//  memset(outline, 0, 80);
//}

void printIPAddress()
{
  IPAddress myIp = Ethernet.localIP();
  if (Serial) { PSerial.printf("IP address: %d.%d.%d.%d\n", myIp[0], myIp[1], myIp[2], myIp[3]); };
  PSerial1.printf("IP address: %d.%d.%d.%d\n", myIp[0], myIp[1], myIp[2], myIp[3]);
}

void setup() {
  byte sensor_index;
  b_network = false;

  Serial.begin(9600);
  Serial1.begin(9600);
  delay(2000);
  
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
//  sprintf(outline, "\nEnvirodinges %s, Container %d\n\n", CODEVERSION, ContainerNum); 
//  output(strlen(outline));
//  
  Ethernet.init(W5500_CS);  
  Wire.begin();  
  for (sensor_index=1; sensor_index<=sensor_required; sensor_index++) {
    b_sensor_present[sensor_index] = false;
    set_i2c_mux(sensor_index + MUX_OFFSET);
    delay(500);
    switch (sensor_index) {
      case 1:
        b_sensor_present[sensor_index] = am2315_1.begin();
        break;
      case 2:
        b_sensor_present[sensor_index] = am2315_2.begin();
        break;
      case 3:
        b_sensor_present[sensor_index] = am2315_3.begin();
        break;
#ifdef SENSORCOUNT_4          
      case 4:
        b_sensor_present[sensor_index] = am2315_4.begin();
        break;
#endif          
    }
    if (b_sensor_present[sensor_index]) {
      sensor_detected += 1;
      if (Serial) { PSerial.printf("sensor %d - ", sensor_index); };
      PSerial1.printf("sensor %d - ",sensor_index);
      if (Serial) { PSerial.printf("OK\n"); };
      PSerial1.printf("OK\n");
//          sprintf(outline, "sensor %sensor_index - OK", b);
//          output(strlen(outline));
    }
    else {
      if (Serial) { PSerial.printf("sensor %d - not found, check wiring & pullups!\n", sensor_index); };
      PSerial1.printf("sensor %d - not found, check wiring & pullups!\n", sensor_index); 
//        sprintf(outline, "sensor %d - not found, check wiring & pullups!", b); 
//        output(strlen(outline));
    }
    delay(1000);     
  }
  if (Serial) {
    PSerial.printf("%s\n\n\n", "Envirodinges starting");
    PSerial.printf("Setting MAC: %02x:%02x:%02x:%02x:%02x:%02x\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  }
  PSerial1.printf("%s\n\n\n", "Envirodinges starting");
  PSerial1.printf("Setting MAC: %02x:%02x:%02x:%02x:%02x:%02x\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  
//  sprintf(outline, "%s\n\n\n", "Envirodinges starting");
//  output(strlen(outline));
//  sprintf(outline, "Setting MAC: %02x:%02x:%02x:%02x:%02x:%02x\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
//  output(strlen(outline));
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

  int sensor_index;
  float temps[4], humids[4], tdew[4];
  float tint_min, tint_max, humidint_max, tdew_max, tdew_fan;
  bool b_fanon = false;
  bool b_errled = false;
  bool b_readok;
  bool b_sensorfault;
  char c_fanstate[4];
  
  if (millis() > nextrun) {
    nextrun += RUNINTERVAL;
    b_sensorfault = false;

    for (sensor_index=1; sensor_index<=sensor_required; sensor_index++) {
//      if (b_sensor_present[sensor_index]) {
        set_i2c_mux((sensor_index + MUX_OFFSET));
        delay(200);
        switch (sensor_index) {
          case 1:   // external sensor
            b_readok = am2315_1.readTemperatureAndHumidity(&temps[1], &humids[1]);
            break;
          case 2:   // internal sensor, inlet
            b_readok = am2315_2.readTemperatureAndHumidity(&temps[2], &humids[2]);
            break;
          case 3:   // internal sensor, outlet
            b_readok = am2315_3.readTemperatureAndHumidity(&temps[3], &humids[3]);
            break;
#ifdef SENSORCOUNT_4           
          case 4:
            b_readok = am2315_4.readTemperatureAndHumidity(&temps[4], &humids[4]);
            break;
#endif
        };
        if (b_readok == false) {
          b_sensorfault = true;
          if (Serial) PSerial.printf("C:%d - S%d - missing\n", ContainerNum, sensor_index);
          PSerial1.printf("C:%d - S%d - missing\n", ContainerNum, sensor_index);        
        }
        else {
          tdew[sensor_index] = dewpoint(temps[sensor_index], humids[sensor_index]);
          if (Serial) PSerial.printf("C:%d - S%d - Hum:  %2.1f  Temp: %2.1f  Dewpt: %2.1f\n", ContainerNum, sensor_index, humids[sensor_index], temps[sensor_index], tdew[sensor_index]); 
          PSerial1.printf("C:%d - S%d - Hum:  %2.1f  Temp: %2.1f  Dewpt: %2.1f\n", ContainerNum, sensor_index, humids[sensor_index], temps[sensor_index], tdew[sensor_index]); 
        }
    }
    if (b_sensorfault == false) { 
      tint_max = max(temps[5], temps[6]);           // highest temp inside
      tint_min = min(temps[5], temps[6]);           // lowest temp inside
      humidint_max = max(humids[5], humids[6]);     // highest humidity inside
      tdew_max = dewpoint(tint_min, humidint_max);  // highest dewpoint inside
      tdew_fan = dewpoint(tint_min, humids[4]);     // inside temp against outside humidity
      b_fanon = (humidint_max > HUMID_MAX);         // fan on if humid inside
      b_fanon = b_fanon && ((tint_min - tdew_fan) < TDEW_DIFF) ; // ... unless chance of condensation
      strncpy(c_fanstate, "on", 3);
    }
    else {
      b_fanon = false;
      strncpy(c_fanstate, "off", 4);
    }
    if (Serial) PSerial.printf("Min temp intern: %2.1f, humid extern: %2.1f, -> dewpt: %2.1f, fan %s\n", tint_min, humidint_max, tdew_fan, c_fanstate);
    PSerial1.printf("Min temp intern: %2.1f, humid extern: %2.1f, -> dewpt: %2.1f, fan %s\n", tint_min, humidint_max, tdew_fan, c_fanstate);

    fanswitch(b_fanon);
  }
  if (!b_network) {
    nexterrorled = millis() + 20000;
  }
  else {
    nexterrorled = millis() + 60000; // > 1 minute
  }
  if ((b_sensorfault) || (!b_sensorcount)) { nexterrorled = millis() + 5000; };
  if ( millis() > nexterrorled ) {
    b_errled = !b_errled;
    digitalWrite(ERROR_LED, b_errled);
  }

  if (millis() > nextfade) {
    nextfade += 30;                                 // wait for 30 milliseconds to see the dimming effect
    analogWrite(ACTIVE_LED, brightness);
  
    // change the brightness for next time through the loop:
    brightness = brightness + fadeAmount;
  
    // reverse the direction of the fading at the ends of the fade:
    if (brightness <= 0 || brightness >= 255) {
      fadeAmount = -fadeAmount;
    }

    // fanswitch((fadeAmount > 0)); // for testing
  }
  if ((millis() > nexterrorled) && (b_errled = false)) {
    b_errled = true;
    digitalWrite(ERROR_LED, true);
    nexterrorled += 1000;
  }
  else {
    b_errled = false;
    digitalWrite(ERROR_LED, false);
    nexterrorled += 60000;
  }

}
