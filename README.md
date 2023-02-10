# soyosource-weblimiter

Die Seite befindet sich noch im Aufbau!

Mit diesem Projekt ist es möglich die Einspeiseleistung einens SoyoSource GTN-1000W / GTN-1200W per Webinterface oder MQTT (z.B iobroker) zu steuern.
Der SoyoSource Einspeisewechselrichter kann die zu wandelnde Energie (DC-Seitig) aus PV-Module oder aus einer Batterie beziehen. Die Einspeiseleistung auf der AC-Seite kann im Einstellmenü als Festwert in Watt oder durch einen auf der Phase angeschlossenen Limiter bereitgestellt werden. Der Limiter wird per RS485 Schnittstelle am SoyoSource angeschlossen und senden dann die auf der Phase anliegende Leistung an den SoyoSource.

Diese Schaltung (Bild 1) ersetzt den Limiter zur Leistungsvorgabe und wird auch an der RS485 Schnittstelle angeschlossen. Damit die Leistungsvorgabe der Schaltung funktioniert muss im Einstellmenü der Limitermode aktiviert werden (Bild 2) 

Bild 1: Schaltung
<img src="https://github.com/matlen67/soyosource-weblimiter/blob/main/images/wiring_nodemcu_rs485.png" width="512">

Bild 2: Einstellmenü
Hier muss Bat AutoLimit Grid auf Y stehen
<img src="https://github.com/matlen67/soyosource-weblimiter/blob/main/images/display_setup.jpg" width="256">
  



<img src="https://github.com/matlen67/soyosource-weblimiter/blob/main/images/Webif_20230209_2015.jpg" width="256">

<img src="https://github.com/matlen67/soyosource-weblimiter/blob/main/images/iobroker_mqtt.png" width="256">

<img src="https://github.com/matlen67/soyosource-weblimiter/blob/main/images/wiring_nodemcu_rs485.png" width="512">
