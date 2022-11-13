# Serial Multiplexer.

## Envirodinges

De twee Envirodingesen sturen regels over twee RS232-lijnen naar iets wat ze zou kunnen loggen.

## Multiplexer

Een Pro Micro met een hardware serial en een software serial leest
deze RS232 lijnen en (omdat de containers zelf de prefix met 'container1' of 'container2' doet) buffert
ze per regel en stuurt ze door via de USB-serial.

De Pro Micro is gemonteerd op een board met schroefaansluitingen, die is eigenlijk bedoeld voor de Arduino Nano, maar
de Pro Micro past ook.

### Seriele poorten

Omdat de SoftwareSerial op de Pro Micro (ATmega32U4) alleen werkt op poorten waar een pin change interrupt actief kan zijn, worden pin 8 en 9 voor respectievelijk RX en TX gebruikt. De vorige versies op respectievelijk pin 4 en 5, daarna 2 en 3 bleken niet te werken totdat ik de handleiding beter had gelezen.

## Wat te doen met de data

Een IOTStack-gebaseerde Pi (Mosquitto, NodeRed, InfluxDB, Grafana) is ingericht om alles te kunnen netwerken.

- NodeRed luistert naar de USB-serial, vertaalt de puntjes naar slashes en publiceert de key/value pairs als topic/payload naar de Mosquitto MQTT broker.
- NoderRed stuurt de key/value pairs ook naar InfluxDB
- Grafana leest InfluxDB en maakt er shiny plaatjes van.

Bovenstaande kan natuurlijk allemaal gescrapt worden als gebruik kan worden gemaakt van een al bestaande machine die USB-serial kan lezen, en de gegevens naar een reeds bestaande MQTT te sturen. Et Cetera.


