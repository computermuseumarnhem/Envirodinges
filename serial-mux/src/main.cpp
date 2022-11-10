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
 * Serial2: SoftwareSerial
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
 * Of course, I mounted the device the other way around.
 *
 *      -------------
 *  10  |             |  9  TX   Serial2
 *  16  |             |  8
 *  14  |             |  7
 *  15  |             |  6
 *  18  |             |  5
 *  19  |             |  4  RX   Serial2
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
 *
 */

#define SERIAL1_TX 1
#define SERIAL1_RX 2
#define SERIAL2_TX 9
#define SERIAL2_RX 4

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

    while (!Serial) {
        delay(5000);
    }
    Serial.println("envirodinges.status: started");
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
    if (timer.next()) {
        Serial.println("envirodinges.status: running");
    }
}
