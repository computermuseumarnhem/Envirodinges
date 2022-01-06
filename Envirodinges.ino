#define CODEVERSION "0.95"

#undef ETHERNET           // do not define when insufficient memory

#define MOSFET            10
#define ACTIVE_LED        9
#define ERROR_LED         8
#define CONTAINER_SELECT  5

#include <PrintEx.h>
#include <Wire.h>
#include <Adafruit_AM2315.h>
#include <math.h>

#ifdef ETHERNET
#include <SPI.h>
#include <Ethernet.h>
#include <Adafruit_MQTT.h>
#endif

PrintEx PSerial = Serial;
PrintEx PSerial1 = Serial1;

#define I2C_ROMADDRESS  0x53
#define I2C_TCAADDR     0x70
#define W5500_CS        7

static uint8_t mac[] = { 0x90, 0xA2, 0xDA, 0x42, 0x42, 0x00 };
static char outline[80];

byte containernum;
bool b_sensor_present[4], b_network;
bool b_errled = false;
byte brightness = 128;    // how bright the LED is
byte fadeAmount = 8;    // how many points to fade the LED by
long nextrun = 0;
long nextfade = 0;
long nextfanshow = 0;
long nexterrorled = 0;
long errorledinterval = 65000; // > 1 minute
char * sensorname[4] = { "external", "inlet", "outlet", "spare" };

#define RUNINTERVAL   60000	// 1 minute
#define FADEINTERVAL  64

#define TDEW_DIFF     10.0  // fan off if dewpoint too close to temperature
#define HUMID_MIN     45.0  // fan on if humidity inside exceeded

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
  nextfanshow = millis() + 5000;
}

void outputvalue(uint8_t sensor, char * outstr)
{
  char buf[80];

//  Serial.print(strlen(outstr));
//  Serial.print("  ");
  memset(buf, 0, 80);
  sprintf(buf, "container%d.sensor.%s.%s", containernum, sensorname[sensor-1], outstr);
  if (Serial) Serial.println(buf);
  Serial1.println(buf);
  memset(outstr, 0, 80);
}

void outputstatus(char * outstr)
{
  char buf[80];

//  Serial.print(strlen(outstr));
//  Serial.print("  ");
  memset(buf, 0, 80);
  sprintf(buf, "container%d.status.%s", containernum, outstr);
  if (Serial) Serial.println(buf);
  Serial1.println(buf);
  memset(outstr, 0, 80);
}
#ifdef ETHERNET
void printIPAddress()
{
  IPAddress myIp = Ethernet.localIP();
  sprintf(outline, "IP: %d.%d.%d.%d\0", myIp[0], myIp[1], myIp[2], myIp[3]);
}
#endif

void setup() {
  byte sensor_index;
  b_network = false;

  Serial.begin(9600);
  Serial1.begin(9600);
  delay(5000);

  
  pinMode(MOSFET, OUTPUT);
  pinMode(ACTIVE_LED, OUTPUT);
  pinMode(ERROR_LED, OUTPUT);
  analogWrite(ACTIVE_LED, 128);
  digitalWrite(ERROR_LED, true);
  digitalWrite(MOSFET, true);

  pinMode(CONTAINER_SELECT, INPUT_PULLUP);
  if (digitalRead(CONTAINER_SELECT) == LOW) {
     mac[5] = 0x01;  
     containernum = 1;
  }
  else
  {
     mac[5] = 0x02;  
     containernum = 2;
  }
  sprintf(outline, "init.version %s\0", CODEVERSION); 
  outputstatus(outline);
  sprintf(outline, "init.container %d\0", containernum); 
  outputstatus(outline);

#ifdef ETHERNET
  Ethernet.init(W5500_CS); 
#endif   
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
    // memset(outline, 0, 80);
    if (b_sensor_present[sensor_index]) {
      sprintf(outline, "%s\0", "status OK\0");
    }
    else {
      sprintf(outline, "%s\0", "status missing - check wiring & pullups!"); 
    }
    outputvalue(sensor_index, outline);
    delay(1000);     
  }
  // memset(outline, 0, 80);
  sprintf(outline, "%s\0", "code starting");
  outputstatus(outline);
  
#ifdef ETHERNET  
  // memset(outline, 0, 80);
  sprintf(outline, "MAC: %02x:%02x:%02x:%02x:%02x:%02x\n\0", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  output(statusoutline);
  // start Ethernet and UDP
  auto link = Ethernet.linkStatus();
  // memset(outline, 0, 80);
  switch (link) {
    case Unknown:
      sprintf(outline, "netw0rk.linkstatus %s\0", "Unknown");
      break;
    case LinkON:
      sprintf(outline, "network.linkstatus %s\0", "UP");
      b_network = true;
      break;
    case LinkOFF:
      sprintf(outline, "network.linkstatus %s\0", "DOWN");
      b_network = true;
      break;
  }
  outputstatus(outline);

  if (b_network) {
    if (Ethernet.begin(mac) == 0) {
      b_network = false;
      // memset(outline, 0, 80);
      sprintf(outline, "%s\0", "network.dhcp fail");
      outputstatus(outline);
    }
    else {
      printIPAddress();
      b_network = true;
      digitalWrite(ERROR_LED, false);
    }
  }
#endif
  digitalWrite(ERROR_LED, false);
  nexterrorled = millis();
  fanswitch(false);
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

  byte sensor_index;
  float temps[4], humids[4], tdew[4];
  float tint_min, tint_max, humidint_max, tdew_max, tdew_fan;
  bool b_fanon, b_readok, b_sensorfault;

  char *fanstate[] { "off", "on" };


  if (millis() > nextrun) {
    nextrun += RUNINTERVAL;
    b_sensorfault = false;

    for (sensor_index=1; sensor_index<=sensor_required; sensor_index++) {
        set_i2c_mux((sensor_index + MUX_OFFSET));
        delay(500);
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
        // memset(outline, 0, 80);
        if (b_readok == false) {
          b_sensorfault = true;
          sprintf(outline, "%s\0", "status missing\0");
        }
        else {
          tdew[sensor_index] = dewpoint(temps[sensor_index], humids[sensor_index]);
          sprintf(outline, "temp: %2.1fC\0", temps[sensor_index]);
          outputvalue(sensor_index, outline); 
          sprintf(outline, "hum:  %2.1f%%\0", humids[sensor_index]);
          outputvalue(sensor_index, outline); 
          sprintf(outline, "dewpt: %2.1fC\0", tdew[sensor_index])
        }
        outputvalue(sensor_index, outline); 
    }
    // memset(outline, 0, 80);
    if (b_sensorfault == false) { 
      tint_max = max(temps[2], temps[3]);           // highest temp inside
      tint_min = min(temps[2], temps[3]);           // lowest temp inside
      humidint_max = max(humids[2], humids[3]);     // highest humidity inside
      tdew_max = dewpoint(tint_min, humidint_max);  // highest dewpoint inside
      tdew_fan = dewpoint(tint_min, humids[1]);     // inside temp against outside humidity
      b_fanon = (humidint_max > HUMID_MIN);         // fan on if humid inside
      b_fanon = b_fanon && ((tint_min - TDEW_DIFF) > tdew_fan) ; // ... unless chance of condensation
      sprintf(outline, "fan.min.temp.intern: %2.1fC\0", tint_min);
      outputstatus(outline);
      sprintf(outline, "fan.humid.extern: %2.1f%%\0", humids[1]);
      outputstatus(outline);
      sprintf(outline, "fan.dewpt: %2.1fC\0", tdew_fan);
      outputstatus(outline);
      sprintf(outline, "fan.status %s\0", fanstate[b_fanon]);
    }
    else {
      b_fanon = false;
      sprintf(outline, "fan.status %s\0", "off - sensor read error");
    }
    outputstatus(outline);
    fanswitch(b_fanon);
#ifdef ETHERNET
    if (!b_network) {
      errorledinterval = 10000;
    }
#endif
    
    if (b_sensorfault) { 
      errorledinterval = 2000; 
    }
    else {
      errorledinterval = 65000;
    }
  }

  if (millis() > nextfanshow) {   // show controller activity
    if (millis() > nextfade) {              
      nextfade += FADEINTERVAL;     // wait for FADEINTERVAL milliseconds to see the dimming effect
      analogWrite(ACTIVE_LED, brightness);

      // change the brightness for next time through the loop:
      brightness = brightness + fadeAmount;
      // reverse the direction of the fading at the ends of the fade:
      if (brightness < 8 || brightness >= 248) {
        fadeAmount = -fadeAmount;
      }    
    }
  }
  else {                        // show fan state for 5 seconds
    if (b_fanon == true) {
      brightness = 240;   
    }
    else
    {
      brightness = 8;
    }
    analogWrite(ACTIVE_LED, brightness);
  }

  if (millis() > nexterrorled) {
    nexterrorled += errorledinterval;
    if (errorledinterval < 60000) { // error condition
      if (b_errled != false ) {
        b_errled = false;
      }
      else {
        b_errled = true;    
      }
    }
    else {
      b_errled = false;
    }
    digitalWrite(ERROR_LED, b_errled);
  }
}
