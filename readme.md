# Über ...
ledDimmmerESP8266_R1A ist meine erste "Interpretation" eines LED Dimmers auf ESP8266 Basis.
ledDimmerESP8266 ist ein 4-kanaliger Dimmer auf Basis des ESP8266 12E, wie der Name hoffentlcih
bereits vermuten lässt.

## Versionsgeschichte
2020-03-31 R1A

## Was kann der Dimmer
Nach dem ersten Download des Sketches wird erst einmal nur ein Drahtloses
Netzwerk zu Verfügung gestellt über das der Dimmer dann konfiguriert werden kann.<br/>
Wenn der Dimmer keine gültigen Daten für die Anmeldung ain einem WLAN zur Verfügung hat
stellt er sein eigenes WLAN mit dem Name "ESPap" zur Verfügung.
Mittels des Passwortes "changeme" kann über WLAN eine Verbindung zu dem Dimmer
aufgebaut werden. Danach sollte der Dimmer unter der url http://192.168.4.1/ konfiguriert
werden können. Die Parameter sollten soweit selbsterklärend sein.
Nachdem der Dimmer korrkt konfiguriert wurde wird er sich beim nächsten Power-Up an dem
konfigurierten Netzwerk anmelden, und - soweit auch der MQTT Server richtig konfiguriert wurde -
ebenfalls bei diesem und für die konfigurierten Tokens eine Subskription anmelden.

## Wo finde ich mehr Infos ?
Ich werde schnellstmöglich eine umfangreichere Beschreibung auf meinem eigenen Blog hinterlassen.
Bitte einfach mal dort nachschauen.

Ich freue mich über Fragen und Anregungen unter contact@khwelter.de.