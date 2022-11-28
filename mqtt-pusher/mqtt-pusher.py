#!/usr/bin/env python3

from config import Config
import sys
import serial
import signal
import paho.mqtt.client as mqtt

def sigterm_handler(signum, frame):
    sys.exit(0)

signal.signal(signal.SIGTERM, sigterm_handler)

def main(argv):

    mqtt_client = mqtt.Client(Config.mqtt_clientname)
    mqtt_client.will_set('envirodinges/mqtt-pusher', 'unplanned disconnect')
    mqtt_client.connect(Config.mqtt_broker, )

    mqtt_client.publish('envirodinges/mqtt-pusher', 'started')

    exit_reason = 'Unknown'

    with serial.Serial(Config.port, Config.speed) as port:
        try:
            while True:
                rawline = port.readline()
                try:
                    line = str(rawline, encoding='ascii').strip()
                except UnicodeDecodeError:
                    line = f'envirodinges/error: decoding {repr(rawline)}'
                    print(line, file=sys.stderr)
                    
                if 'status.fan.status' in line:
                    topic, value = line.split(' ', 1)
                elif ':' in line:
                    topic, value = line.split(':', 1)
                else:
                    print(line, file=sys.stderr)
                    continue

                # cleanup value:
                value = value.strip()
                if value.endswith('%'):
                    value = float(value.strip('%'))
                elif value.endswith('C'):
                    value = float(value.strip('C'))

                topic = topic.replace('.','/')
                mqtt_client.publish(topic, value)
        
        except KeyboardInterrupt:
            exit_reason = 'Ctrl-C'
            pass
        except SystemExit:
            exit_reason = 'SIGTERM'
            pass

    mqtt_client.publish('envirodinges/mqtt-pusher', f'stopped {exit_reason}')
    mqtt_client.disconnect()

    return exit_reason

if __name__ == '__main__':

    print('mqtt-pusher started', file=sys.stderr)
    reason = main(sys.argv[1:])
    print(f'mqtt-pusher stopped ({reason})', file=sys.stderr)
