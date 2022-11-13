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
 *    - RXI (pin 0)
 *    - TXO (pin 1), unused
 * Serial2: SoftwareSerial
 *    - RXI (pin 8)
 *    - TXO (pin 9), unused
 *
 *      -------------
 *  10  |             |  9  TX   Serial2
 *  16  |             |  8  RX   Serial2
 *  14  |             |  7
 *  15  |             |  6
 *  18  |             |  5
 *  19  |             |  4
 *  20  |             |  3
 *  21  |             |  2
 * VCC  |             | GND
 * RST  |             | GND
 * GND  |    -----    |  0  RX   Serial1
 * RAW  |   |     |   |  1  TX   Serial1
 *       ---|     |---
 *           -----
 *                       USB
 *
 */

#define SERIAL1_RX 0
#define SERIAL1_TX 1
#define SERIAL2_RX 8
#define SERIAL2_TX 9

#include <Arduino.h>
#include <SoftwareSerial.h>

SoftwareSerial Serial2(SERIAL2_RX, SERIAL2_TX);
String Buffer1;
String Buffer2;

class Timer {
private:
    bool _rollover;
    unsigned long _timeout;
    unsigned long _next;

public:
    Timer(unsigned long timeout) {
        _timeout = timeout;
        next();
    }

    bool next() {
        unsigned long now = millis();

        if (_rollover) {
            if (now < _next) {  // millis() has also rolled over
                _rollover = false;
            }
            return false;  // still not ready for action
        }

        if (now > _next) {  // ready for action
            _next = now + _timeout;
            if (_next < now) {  // _next rolled over
                _rollover = true;
            }
            return true;
        }

        return false;
    }
};

Timer timer(10000);

void setup() {
    Serial.begin(115200);
    Serial1.begin(9600);
    Serial2.begin(9600);
    Serial2.listen();
    Buffer1 = "";
    Buffer2 = "";

    // Waiting for the USB Serial to be connected
    // is not necessary nor wanted

    delay(5000);

    Serial.println("envirodinges.status: started");
}

void ProcessChar(String &buffer, const char c) {
    switch (c) {
        case '\r':
            break;  // ignore
        case '\n':
            Serial.println("envirodinges." + buffer);
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
    if (timer.next()) {
        Serial.println("envirodinges.status: alive");
    }
}
