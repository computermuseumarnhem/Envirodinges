# Envirodinges
Veredelde fan controller voor het ventileren van de opslagcontainers. 

Fan aan/uit wordt gestuurd door een Arduino Pro Micro op basis van AM2315 thermo/hygro sensors (1 buiten, 2 binnen) zodat er geventileerd wordt als de luchttemperatuur >10C boven de dauwpunttemperatuur ligt waardoor er binnen geen kans is op condens van de aangezogen lucht.

Status output 1x/minuut via RS232, zie Envirodinges.log.

LAN hardware met W5500 TCP/IP chip aanwezig, maar niet gebruikt door gebrek aan programmaruimte.

Omdat de AM2315 geen instelbaar I2C adres heeft worden ze via een multiplexer (TCA9548) aangesloten.
