# T5_V213_GAS
Report last 5 days of GAS usage on
LilyGO TTGO T5 V2.3 ESP32 - 2.13 inch E-paper

This program consults an InfluxDB in which GAS usage is reported each minute.

setup()
- initialise display
- connect to WiFi
- check if InfluxDB is alive with a http ping command<br>
  http.begin(client, server, 8086, "/ping");
- clean display

loop()
- report total number of GAS usage records stored in InfluxDB<br>
  /query?db=energydb&q=SELECT+count(gas)+from+log
- get the GAS usage for the last 5 days (aggregate records per 24 hours)<br>
  /query?db=energydb&q=SELECT+max(gas)+from+log+WHERE+time+>=+now()+-5d+GROUP+BY+time(24h)
- draw graph with the GAS usage per day during last 5 days
- poweroff display & wait 5 minutes before to continue

 Display Size:
 - Width : 122 pixels
 - Height : 250 pixels


### Arduino Setup:
- add Arduino -> Preferences -> Additional Board Manager URL:<br>
  https://dl.espressif.com/dl/package_esp32_index.json <br>
- Board Manager:<br>
  search for esp32 Espressif Systems<br>
  -> Install
- Select the correct board<br>
  -> Tools -> Boards -> ESP32 Boards -> ESP32 Dev Module
   
### LIBs & Code References:
- https://github.com/lewisxhe/GxEPD via Arduino -> Sketch -> Include Library -> Add .ZIP Library<br>
  Do _NOT_ install GxEPD via Arduino Library Manager !!
- Adafruit GFX Library : install via Arduino Library Manager<br>
  https://github.com/adafruit/Adafruit-GFX-Library
- ArduinoJson : install via Arduino Library Manager<br>
  https://github.com/bblanchon/ArduinoJson
- draw graph is based on routine used in:<br>
  https://github.com/markbirss/LilyGo-EPD-4-7-OWM-Weather-Display
  

### Shopping list:
Part|Price|Qtd.|Url
---|---|---|---
LilyGO TTGO T5 V2.3 ESP32 |€ 17.5|1|https://www.tinytronics.nl/shop/nl/development-boards/microcontroller-boards/met-wi-fi/lilygo-ttgo-t5-v2.3-esp32-met-2.13-inch-e-paper-e-ink

### Reference Pictures:
<img src="jpg/T5_V213_GAS.jpg" width="32%"/>

