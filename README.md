# Envirodinges
Veredelde fan controller voor het ventileren van de opslagcontainers. 

Fan aan/uit wordt gestuurd op basis van AM2315 thermo/hygro sensors (1 buiten, 2 binnen) zodat er geventileerd wordt als de dauwpunttemperatuur buiten voldoende laag is dat er binnen geen kans is op condens van de aangezogen lucht.

Status output 1x/minuut via RS232.

MQTT status wanneer online via LAN mogelijk, maar nog niet geimplementeerd.

Omdat de AM2315 geen instelbaar I2C adres heeft worden ze via een multiplexer (TCA9548) aangesloten.
