# Serial Multiplexer.

## Envirodinges

De twee Envirodingesen sturen regels over twee RS232-lijnen naar iets wat ze zou kunnen loggen.

## Multiplexer

Een Pro Micro met een hardware serial en een software serial (AltSoftSerial, slightly modified) leest
deze RS232 lijnen en (omdat de containers zelf de prefix met 'container1' of 'container2' doet) buffert
ze per regel en stuurt ze door via de USB-serial.

## Wat te doen met de data

Een IOTStack-gebaseerde Pi (Mosquitto, NodeRed, InfluxDB, Grafana) is ingericht om alles te kunnen netwerken.

- NodeRed luistert naar de USB-serial, knoopt er 'envirodinges' voor, vertaalt de puntjes naar slashes en publiceert de key/value pairs als topic/payload naar de Mosquitto MQTT broker.
- NoderRed stuurt de key/value pairs ook naar InfluxDB
- Grafana leest InfluxDB en maakt er shiny plaatjes van.

Bovenstaande kan natuurlijk allemaal gescrapt worden als gebruik kan worden gemaakt van een al bestaande machine die USB-serial kan lezen, en de gegevens naar een reeds bestaande MQTT te sturen. Et Cetera.


