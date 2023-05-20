# soyosource-weblimiter

Die Seite befindet sich noch im Aufbau!

Mit diesem Projekt ist es möglich die Einspeiseleistung einens SoyoSource GTN-1000W / GTN-1200W per Webinterface oder MQTT (z.B iobroker) zu steuern.
Der SoyoSource Einspeisewechselrichter kann die zu wandelnde Energie (DC-Seitig) aus PV-Module oder aus einer Batterie beziehen. Die Einspeiseleistung auf der AC-Seite kann im Einstellmenü als Festwert in Watt oder durch einen auf der Phase angeschlossenen Limiter bereitgestellt werden. Der Limiter wird per RS485 Schnittstelle am SoyoSource angeschlossen und senden dann die auf der Phase anliegende Leistung an den SoyoSource.

Diese Schaltung (Bild 1) ersetzt den Limiter zur Leistungsvorgabe und wird an der RS485 Schnittstelle des SoyoSouce angeschlossen. Damit die Leistungsvorgabe der Schaltung funktioniert muss im Einstellmenü der Limitermode aktiviert werden (Bild 2).

Gleich als Hinweis. In den aktuellen Versionen der Geräte gibt der Soyosource keine Daten über die RS485 aus, somit ist ein Auslesen von verschiedenen Informationen aktuell nicht möglich. Leider liegen mir aktuell keine Informationen vor ob der Sendevorgang per Software/Hardware deaktiviert wurde oder es nur neue Parameter bedarf um den Soyo zum Sprechen zu bewegen kann. Aktuell sende ich per Schleife alle möglichen Byte-Reihenfolgen zum SoyoSoure um zu testen ob er irgendwann einmal antwortet.   
## Arduino IDE 2.0.4
#Library Github
 - ESPAsync_WiFiManager (https://github.com/khoih-prog/ESPAsync_WiFiManager)
 - ESPAsyncWebServer    (https://github.com/me-no-dev/ESPAsyncWebServer)
 - ESPAsyncTCP          (https://github.com/me-no-dev/ESPAsyncTCP)


## Schaltung
### Bauteile
- NodeMCU mit ESP8266 (ESP-12F) (4MB Flash)
- RS485 Entwicklungsboard TTL zu RS485, MAX485

Hinweis: Das RS485 Etwicklungsboard verwenden einen MAX485 Pegelwandler der für eine Versorgungsspannung von 5V ausgelegt ist. Da die GPIO's des ESP8266 dauerhaft nur 3.3V vertragen wird die Spannung Vcc vom RS485 Entwicklungsboard am 3.3V Ausgang des NodeMCU abgegriffen. Das RS485 Etwicklungsboard arbeitet auch zuverlässig mit 3.3V. Die 5V Spannungsversorgung des NodeMCU kann entweder über USB oder den Anschlus-Pin VIN erfolgen.

### Bild 1
<img src="https://github.com/matlen67/soyosource-weblimiter/blob/main/images/wiring_nodemcu_rs485.png" width="512">

### Bild 2: Einstellmenü
Hier muss 'Bat AutoLimit Grid' auf Y stehen

<img src="https://github.com/matlen67/soyosource-weblimiter/blob/main/images/display_setup.jpg" width="256">
  

## Webif
<img src="https://github.com/matten67/soyosource-weblimiter/blob/main/images/webif_20230520_0935.png" width="256">

<img src="https://github.com/matlen67/soyosource-weblimiter/blob/main/images/iobroker_mqtt.png" width="256">

