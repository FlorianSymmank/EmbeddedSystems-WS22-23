## Embedded Systemes Semsterpojekt WS22/23

Daten eines DHT22 Temperatur und Feuchtigkeitsensors werden auf mehreren 7 Segment Displays ausgegebe.  
Als OS verwende ich Zephyr.

___
Data Pin = Port A Pin 0  
Store Pin =	Port A Pin 1  
Refresh Pin = Port A Pin 4  
DHT22 Pin = Port B Pin 0  

VCC [+]  
OE [-] output enable (low active)  
SCRLR [+] storage clear (low active)  

SER serial input (data line)  
RCLK register clock (output clock) (bei 0 -> 1 werden die werte im register auf die ausgänge übertragen) 
SRCLK storage clock (input clock) (bei 0 -> 1 wird der wert von SER in das register geschoben, vorherige werte im register werden einen weiter geschoben, der ausgang verändert sich nicht)  

8-Bit shift register  
https://pdf1.alldatasheet.com/datasheet-pdf/view/797421/TI1/SN74HC595N.html

7-seg-display  
https://pdf1.alldatasheet.com/datasheet-pdf/view/1134416/ETC2/5611AH.html  
https://zxcongducxz.000webhostapp.com/arduino/SevenSegmentLEDDisplay.html  

GND 3 und 8 (eine Verbindung reicht)

Verbindungen 8-Bit shift register -> 7-seg-display  
qA -> A (Pin 7)  
qB -> B (Pin 6)  
qC -> C (Pin 4)  
qD -> D (Pin 2)  
qE -> E (Pin 1)  
qF -> F (Pin 9)  
qG -> G (Pin 10)  
qH -> H (Pin 5)  

qH' -> ser (nächstes register)