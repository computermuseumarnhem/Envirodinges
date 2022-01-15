/*
 * Serial multiplexer.
 *
 * Two real serial ports can receive lines of text and
 * transmit these over the virtual serial port (USB) of
 * a Arduino Pro Micro
 * 
 * Connections:
 * 
 * Serial: Virtual serial port over USB (/dev/ttyUSBx on the Pi)
 * Serial1: HardwareSerial port of the Pro Micro
 *    - RXI 
 *    - TXO
 * Serial2: AltSoftSerial (by Paul Stoffregen). Pins are fixed to:
 * 
 *                        USB
 *                       -----
 *                   ---|     |---
 *  Serial1  TX  1  |   |     |   | RAW
 *  Serial1  RX  0  |    -----    | GND
 *              GND |             | RST
 *              GND |             | VCC
 *               2  |             | 21
 *               3  |             | 20
 *  Serial2   RX 4  |             | 19
 *               5  |             | 18
 *               6  |             | 15
 *               7  |             | 14
 *               8  |             | 16
 *  Serial2   TX 9  |             | 10
 *                   -------------
 *   
 *
 * 
 *  
 */


#include <Arduino.h>
#include <AltSoftSerial.h>

AltSoftSerial Serial2;
String Buffer1;
String Buffer2;

void setup() {
    Serial.begin(115200);
    Serial1.begin(9600);
    Serial2.begin(9600);
    Buffer1 = "";
    Buffer2 = "";
}

void ProcessChar(String &buffer, const char c) {
    switch (c) {
        case '\r':
            break;  // ignore
        case '\n':
            Serial.println(buffer);
            buffer = "";
            break;
        default:
            buffer += c;
            break;
    }
}

void loop() {
    if (Serial1.available()) {
        char c = Serial1.read();
        ProcessChar(Buffer1, c);
    }

    if (Serial2.available()) {
        char c = Serial2.read();
        ProcessChar(Buffer2, c);
    }
}
