# Makefile
# Sample for RH_RF95 (client and server) on Raspberry Pi
# Caution: requires bcm2835 library to be already installed
# http://www.airspayce.com/mikem/bcm2835/

CC            = g++
CFLAGS        = -DRASPBERRY_PI -DBCM2835_NO_DELAY_COMPATIBILITY -D__BASEFILE__=\"$*\"
LIBS          = -lbcm2835
RADIOHEADBASE = ../../..
INCLUDE       = -I$(RADIOHEADBASE)

all: sx1276_client sx1276_server multiserver multiclient

RasPi.o: $(RADIOHEADBASE)/RHutil/RasPi.cpp
				$(CC) $(CFLAGS) -c $(RADIOHEADBASE)/RHutil/RasPi.cpp $(INCLUDE)

sx1276_client.o: sx1276_client.cpp
				$(CC) $(CFLAGS) -c $(INCLUDE) $<

sx1276_server.o: sx1276_server.cpp
				$(CC) $(CFLAGS) -c $(INCLUDE) $<

multiserver.o: multiserver.cpp
				$(CC) $(CFLAGS) -c $(INCLUDE) $<

multiclient.o: multiclient.cpp
				$(CC) $(CFLAGS) -c $(INCLUDE) $<

RH_PI-GATE.o: $(RADIOHEADBASE)/RH_PI-GATE.cpp
				$(CC) $(CFLAGS) -c $(INCLUDE) $<

RHDatagram.o: $(RADIOHEADBASE)/RHDatagram.cpp
				$(CC) $(CFLAGS) -c $(INCLUDE) $<

RHHardwareSPI.o: $(RADIOHEADBASE)/RHHardwareSPI.cpp
				$(CC) $(CFLAGS) -c $(INCLUDE) $<

RHSPIDriver.o: $(RADIOHEADBASE)/RHSPIDriver.cpp
				$(CC) $(CFLAGS) -c $(INCLUDE) $<

RHGenericDriver.o: $(RADIOHEADBASE)/RHGenericDriver.cpp
				$(CC) $(CFLAGS) -c $(INCLUDE) $<

RHGenericSPI.o: $(RADIOHEADBASE)/RHGenericSPI.cpp
				$(CC) $(CFLAGS) -c $(INCLUDE) $<

sx1276_client: sx1276_client.o RH_PI-GATE.o RasPi.o RHHardwareSPI.o RHGenericDriver.o RHGenericSPI.o RHSPIDriver.o
				$(CC) $^ $(LIBS) -o sx1276_client

sx1276_server: sx1276_server.o RH_PI-GATE.o RasPi.o RHHardwareSPI.o RHGenericDriver.o RHGenericSPI.o RHSPIDriver.o
				$(CC) $^ $(LIBS) -o sx1276_server

multiserver: multiserver.o RH_PI-GATE.o RasPi.o RHHardwareSPI.o RHGenericDriver.o RHGenericSPI.o RHSPIDriver.o
				$(CC) $^ $(LIBS) -o multiserver

multiclient: multiclient.o RH_PI-GATE.o RasPi.o RHHardwareSPI.o RHGenericDriver.o RHGenericSPI.o RHSPIDriver.o
				$(CC) $^ $(LIBS) -o multiclient

clean:
				rm -rf *.o sx1276_client sx1276_server multiserver multiclient
