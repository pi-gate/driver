<img src="https://ex-store.de/images/pi-gate/lo_pi-gate.png" height="25%" width="25%" alt="Pi-Gate®"><br>
RadioHead Packet Radio library for Pi-Gate® board 
=================================================
<p align="center"><img src="https://ex-store.de/images/pi-gate/image1.png" height="40%" width="40%" alt="Pi-Gate®"></p>

###Version 1.67

This is a fork of the original RadioHead Packet Radio library for embedded microprocessors. It provides a complete object-oriented library for sending and receiving packetized messages via Semtech SX1276 chip on a range of embedded microprocessors.

**Please read the full documentation and licensing from the original author [site][1]**

### features added with this fork
=================================

- Added driver for Pi-Gate® board
- Added samples for Pi-Gate® board

Driver code is located under [/RH_PI-GATE.cpp][5] and [/RH_PI-GATE.h][6].<br>
Sample code for Raspberry PI is located under [/examples/raspi/pi-gate][3] folder.

### Installation on Raspberry PI
================================

You need install bcm2835 library

This library consists of a single non-shared library and header file, which will be installed in the usual places by make install
```shell
wget http://www.airspayce.com/mikem/bcm2835/bcm2835-1.55.tar.gz
tar zxvf bcm2835-1.xx.tar.gz
cd bcm2835-1.xx
./configure
make
sudo make check
```

Clone repository
```shell
sudo apt update
sudo apt install git
git clone https://github.com/gerrylenz/pi-gate
```

### Coding
================================

**Connection and pins definition**

Boards pins (Chip Select, IRQ line, Reset and TXE) definition are set in the [/examples/raspi/pi-gate/GateDefinitions.h][4] file. In your code, you need to include the file definition like this
```cpp
#include "GateDefinitions.h"

```

**Create an instance of a driver for 2 modules**
```cpp
//for 433Mhz Gate
RH_SX1276 rf433(RF433_CS_PIN, RF433_IRQ_PIN, RF433_RST_PIN);
//for 868Mhz Gate
RH_SX1276 rf868(RF868_CS_PIN, RF868_IRQ_PIN, RF868_RST_PIN, RF868_TXE_PIN);

```

**Create samples**
```shell
cd pi-gate/examples/raspi/pi-gate/
make
sudo ./multiserver
```

[1]: http://www.airspayce.com/mikem/arduino/RadioHead/
[2]: http://www.airspayce.com/mikem/arduino/RadioHead/RadioHead-1.67.zip
[3]: https://github.com/gerrylenz/pi-gate/blob/master/examples/raspi/pi-gate
[4]: https://github.com/gerrylenz/pi-gate/blob/master/examples/raspi/pi-gate/GateDefinitions.h
[5]: https://github.com/gerrylenz/pi-gate/blob/master/RH_PI-GATE.cpp
[6]: https://github.com/gerrylenz/pi-gate/blob/master/RH_PI-GATE.h