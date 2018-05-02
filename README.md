RadioHead Packet Radio library for Pi-Gate® board 
=================================================

###Version 1.67

This is a fork of the original RadioHead Packet Radio library for embedded microprocessors. It provides a complete object-oriented library for sending and receiving packetized messages via a variety of common data radios and other transports on a range of embedded microprocessors.

**Please read the full documentation and licensing from the original author [site][1]**

### features added with this fork
=================================

- Added driver for Pi-Gate® board
- Added samples for Pi-Gate® board

Sample code for Raspberry PI is located under [RadioHead/examples/raspi/pi-gate][3] folder.

### Installation on Raspberry PI
================================

Clone repository
```shell
git clone https://github.com/gerrylenz/pi-gate
```

**Connection and pins definition**

Boards pins (Chip Select, IRQ line, Reset and TXE) definition are set in the new [RadioHead/examples/raspi/pi-gate/GateDefinitions.h][4] file. In your code, you need to include the file definition like this
```cpp
#include "GateDefinitions.h"

```

**Create an instance of a driver for 2 modules**
```cpp
for 433 Mhz Gate
RH_SX1276 rf433(RF433_CS_PIN, RF433_IRQ_PIN, RF433_RST_PIN);
for 868Mhz Gate
RH_SX1276 rf868(RF868_CS_PIN, RF868_IRQ_PIN, RF868_RST_PIN, RF868_TXE_PIN);

```

[1]: http://www.airspayce.com/mikem/arduino/RadioHead/
[2]: http://www.airspayce.com/mikem/arduino/RadioHead/RadioHead-1.67.zip
[3]: https://github.com/gerrylenz/pi-gate/blob/master/examples/raspi/pi-gate
[4]: https://github.com/gerrylenz/pi-gate/blob/master/examples/raspi/pi-gate/GateDefinitions.h