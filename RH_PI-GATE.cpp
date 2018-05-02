// pi-gate.cpp
//
// Original
// Author: Mike McCauley (mikem@airspayce.com)
// Copyright (C) 2014 Mike McCauley
// $Id: RH_RF95.cpp,v 1.14 2017/01/13 01:29:36 mikem Exp mikem $
//
// Changed
// Author: Gernot Lenz (contact@pi-gate.net)
// Copyright (C) 2018 Gernot Lenz
// $Id: pi-gate.cpp,v 1.0 2018/05/01 $

#include <RH_PI-GATE.h>

// These are indexed by the values of ModemConfigChoice
// Stored in flash (program) memory to save SRAM
PROGMEM static const RH_SX1276::ModemConfig MODEM_CONFIG_TABLE[] = {
//  1d,     1e,      26
		{ 0x72, 0x74, 0x00 }, // Bw125Cr45Sf128 (the chip default)
		{ 0x92, 0x74, 0x00 }, // Bw500Cr45Sf128
		{ 0x48, 0x94, 0x00 }, // Bw31_25Cr48Sf512
		{ 0x78, 0xc4, 0x00 }, // Bw125Cr48Sf4096

		};

RH_SX1276::RH_SX1276(uint8_t slaveSelectPin, uint8_t interruptPin, uint8_t rstPin, uint8_t txePin, RHGenericSPI& spi) :
		RHSPIDriver(slaveSelectPin, spi), _rxBufValid(0) {
	_slaveSelectPin = slaveSelectPin;
	_interruptPin = interruptPin;
	_resetPin = rstPin;
	_txePin = txePin;
}

bool RH_SX1276::init() {
#ifdef DEBUG
	printf("\nss: %d, rst: %d, txe: %d, irq: %d\n", _slaveSelectPin, _resetPin, _txePin, _interruptPin);
#endif

	if (!RHSPIDriver::init()) {
#ifdef DEBUG
		printf("RHSPIDriver::init error\n");
#endif
		return false;
	}

	if (_txePin < 0xff) {
		pinMode(_txePin, OUTPUT);   // Pi-Gate
		digitalWrite(_txePin, LOW);
	}

	// IRQ Pin input/pull down
	pinMode(_interruptPin, INPUT);
	bcm2835_gpio_set_pud(_interruptPin, BCM2835_GPIO_PUD_DOWN);

	// Pulse a reset on module
	pinMode(_resetPin, OUTPUT);
	digitalWrite(_resetPin, LOW);
	delay(150);
	digitalWrite(_resetPin, HIGH);
	delay(100);

	byte version = spiRead(RH_SX1276_REG_42_VERSION);
	if (version != 0x12) { // sx1272
		digitalWrite(_resetPin, LOW);
		delay(150);
		digitalWrite(_resetPin, HIGH);
		delay(100);
		version = spiRead(RH_SX1276_REG_42_VERSION);
		if (version != 0x22) { // sx1276
#ifdef DEBUG
				printf("Kein 0x12 oder 0x22 device gefunden.\n");
#endif
			return false;;
		}
	}

#ifdef DEBUG
	printf("Modul Version=%d\n", version);
#endif

	// Set sleep mode, so we can also set LORA mode:
	spiWrite(RH_SX1276_REG_01_OP_MODE, RH_SX1276_MODE_SLEEP | RH_SX1276_LONG_RANGE_MODE);
	delay(10); // Wait for sleep mode to take over from say, CAD
	// Check we are in sleep mode, with LORA set
	if (spiRead(RH_SX1276_REG_01_OP_MODE) != (RH_SX1276_MODE_SLEEP | RH_SX1276_LONG_RANGE_MODE)) {
#ifdef DEBUG
		printf("Error=%x\n", spiRead(RH_SX1276_REG_01_OP_MODE));
#endif
		return false; // No device present?
	}

	// Set up FIFO
	// We configure so that we can use the entire 256 byte FIFO for either receive
	// or transmit, but not both at the same time
	spiWrite(RH_SX1276_REG_0E_FIFO_TX_BASE_ADDR, 0);
	spiWrite(RH_SX1276_REG_0F_FIFO_RX_BASE_ADDR, 0);

	// Packet format is preamble + explicit-header + payload + crc
	// Explicit Header Mode
	// payload is TO + FROM + ID + FLAGS + message data
	// RX mode is implmented with RXCONTINUOUS
	// max message data length is 255 - 4 = 251 octets

	setModeIdle();

	// Set up default configuration
	// No Sync Words in LORA mode.
	setModemConfig(Bw125Cr45Sf128); // Radio default
//    setModemConfig(Bw125Cr48Sf4096); // slow and reliable?
	setPreambleLength(8); // Default is 8

	// An innocuous ISM frequency, same as RF22's
	//setFrequency(868.0);

	// Lowish power
	setTxPower(13);

	return true;
}

// Check whether the latest received message is complete and uncorrupted
void RH_SX1276::validateRxBuf() {
	if (_bufLen < 4)
		return; // Too short to be a real message
	// Extract the 4 headers
	_rxHeaderTo = _buf[0];
	_rxHeaderFrom = _buf[1];
	_rxHeaderId = _buf[2];
	_rxHeaderFlags = _buf[3];
	if (_promiscuous || _rxHeaderTo == _thisAddress || _rxHeaderTo == RH_BROADCAST_ADDRESS) {
		_rxGood++;
		_rxBufValid = true;
	}
}

bool RH_SX1276::available() {
#ifdef RH_SX1276_IRQLESS
	// Read the interrupt register
	uint8_t irq_flags = spiRead(RH_SX1276_REG_12_IRQ_FLAGS);
	if (_mode == RHModeRx && irq_flags & RH_SX1276_RX_DONE) {
		// Have received a packet
		uint8_t len = spiRead(RH_SX1276_REG_13_RX_NB_BYTES);

		// Reset the fifo read ptr to the beginning of the packet
		spiWrite(RH_SX1276_REG_0D_FIFO_ADDR_PTR, spiRead(RH_SX1276_REG_10_FIFO_RX_CURRENT_ADDR));
		spiBurstRead(RH_SX1276_REG_00_FIFO, _buf, len);
		_bufLen = len;
		spiWrite(RH_SX1276_REG_12_IRQ_FLAGS, 0xff); // Clear all IRQ flags

		// Remember the RSSI of this packet
		// this is according to the doc, but is it really correct?
		// weakest receiveable signals are reported RSSI at about -66
		_lastRssi = spiRead(RH_SX1276_REG_1A_PKT_RSSI_VALUE) - 137;

		// We have received a message.
		validateRxBuf();
		if (_rxBufValid)
			setModeIdle(); // Got one
	} else if (_mode == RHModeCad && irq_flags & RH_SX1276_CAD_DONE) {
		_cad = irq_flags & RH_SX1276_CAD_DETECTED;
		setModeIdle();
	}

	spiWrite(RH_SX1276_REG_12_IRQ_FLAGS, 0xff); // Clear all IRQ flags

#endif // defined RH_SX1276_IRQLESS

	if (_mode == RHModeTx)
		return false;
	setModeRx();
	return _rxBufValid; // Will be set by the interrupt handler when a good message is received
}

void RH_SX1276::clearRxBuf() {
	ATOMIC_BLOCK_START;
	_rxBufValid = false;
	_bufLen = 0;
	ATOMIC_BLOCK_END;
}

bool RH_SX1276::recv(uint8_t* buf, uint8_t* len) {
	if (!available())
		return false;
	if (buf && len) {
		ATOMIC_BLOCK_START;
		// Skip the 4 headers that are at the beginning of the rxBuf
		if (*len > _bufLen - RH_SX1276_HEADER_LEN)
			*len = _bufLen - RH_SX1276_HEADER_LEN;
		memcpy(buf, _buf + RH_SX1276_HEADER_LEN, *len);
		ATOMIC_BLOCK_END;
	}
	clearRxBuf(); // This message accepted and cleared
	return true;
}

bool RH_SX1276::send(const uint8_t* data, uint8_t len) {
	if (len > RH_SX1276_MAX_MESSAGE_LEN)
		return false;

	waitPacketSent(); // Make sure we dont interrupt an outgoing message
	setModeIdle();

	if (!waitCAD())
		return false;  // Check channel activity

	// Position at the beginning of the FIFO
	spiWrite(RH_SX1276_REG_0D_FIFO_ADDR_PTR, 0);
	// The headers
	spiWrite(RH_SX1276_REG_00_FIFO, _txHeaderTo);
	spiWrite(RH_SX1276_REG_00_FIFO, _txHeaderFrom);
	spiWrite(RH_SX1276_REG_00_FIFO, _txHeaderId);
	spiWrite(RH_SX1276_REG_00_FIFO, _txHeaderFlags);
	// The message data
	spiBurstWrite(RH_SX1276_REG_00_FIFO, data, len);
	spiWrite(RH_SX1276_REG_22_PAYLOAD_LENGTH, len + RH_SX1276_HEADER_LEN);

	setModeTx(); // Start the transmitter
	// when Tx is done, interruptHandler will fire and radio mode will return to STANDBY
	return true;
}

#ifdef RH_SX1276_IRQLESS
// Since we have no interrupts, we need to implement our own 
// waitPacketSent for the driver by reading RF69 internal register
bool RH_SX1276::waitPacketSent() {
	// If we are not currently in transmit mode, there is no packet to wait for
	if (_mode != RHModeTx)
		return false;

	while (!(spiRead(RH_SX1276_REG_12_IRQ_FLAGS) & RH_SX1276_TX_DONE)) {
		YIELD;
	}

	// A transmitter message has been fully sent
	_txGood++;
	setModeIdle(); // Clears FIFO
	return true;
}
#endif // defined RH_SX1276_IRQLESS

bool RH_SX1276::printRegisters() {
	uint8_t registers[] = { 0x01, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x014, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20,
			0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27 };

	uint8_t i;
	for (i = 0; i < sizeof(registers); i++) {
		printf("%x:%x\n", registers[i], spiRead(registers[i]));
	}
	return true;
}

uint8_t RH_SX1276::maxMessageLength() {
	return RH_SX1276_MAX_MESSAGE_LEN;
}

bool RH_SX1276::setFrequency(float centre) {
	// Frf = FRF / FSTEP
	uint32_t frf = (centre * 1000000.0) / RH_SX1276_FSTEP;
	spiWrite(RH_SX1276_REG_06_FRF_MSB, (frf >> 16) & 0xff);
	spiWrite(RH_SX1276_REG_07_FRF_MID, (frf >> 8) & 0xff);
	spiWrite(RH_SX1276_REG_08_FRF_LSB, frf & 0xff);

	return true;
}

void RH_SX1276::setModeIdle() {
	if (_mode != RHModeIdle) {
		spiWrite(RH_SX1276_REG_01_OP_MODE, RH_SX1276_MODE_STDBY);
		_mode = RHModeIdle;
	}
}

bool RH_SX1276::sleep() {
	if (_mode != RHModeSleep) {
		spiWrite(RH_SX1276_REG_01_OP_MODE, RH_SX1276_MODE_SLEEP);
		_mode = RHModeSleep;
	}
	return true;
}

void RH_SX1276::setModeRx() {
	if (_mode != RHModeRx) {
		if (_txePin < 0xff) {
#ifdef DEBUG
			printf("setModeRx, pin GPIO%d to LOW\n", _txePin);
#endif
			digitalWrite(_txePin, LOW);
		}

		spiWrite(RH_SX1276_REG_01_OP_MODE, RH_SX1276_MODE_RXCONTINUOUS);
		spiWrite(RH_SX1276_REG_40_DIO_MAPPING1, 0x00); // Interrupt on RxDone
		_mode = RHModeRx;
	}
}

void RH_SX1276::setModeTx() {
	if (_mode != RHModeTx) {
		if (_txePin < 0xff) {
#ifdef DEBUG
			printf("setModeRx, pin GPIO%d to HIGH\n", _txePin);
#endif
			digitalWrite(_txePin, HIGH);
		}

		spiWrite(RH_SX1276_REG_01_OP_MODE, RH_SX1276_MODE_TX);
		spiWrite(RH_SX1276_REG_40_DIO_MAPPING1, 0x40); // Interrupt on TxDone
		_mode = RHModeTx;
	}
}

void RH_SX1276::setTxPower(int8_t power, bool useRFO) {
	// Sigh, different behaviours depending on whther the module use PA_BOOST or the RFO pin
	// for the transmitter output
	if (useRFO) {
		if (power > 14)
			power = 14;
		if (power < -1)
			power = -1;
		spiWrite(RH_SX1276_REG_09_PA_CONFIG, RH_SX1276_MAX_POWER | (power + 1));
	} else {
		if (power > 23)
			power = 23;
		if (power < 5)
			power = 5;

		// For RH_SX1276_PA_DAC_ENABLE, manual says '+20dBm on PA_BOOST when OutputPower=0xf'
		// RH_SX1276_PA_DAC_ENABLE actually adds about 3dBm to all power levels. We will us it
		// for 21, 22 and 23dBm
		if (power > 20) {
			spiWrite(RH_SX1276_REG_4D_PA_DAC, RH_SX1276_PA_DAC_ENABLE);
			power -= 3;
		} else {
			spiWrite(RH_SX1276_REG_4D_PA_DAC, RH_SX1276_PA_DAC_DISABLE);
		}

		// RFM95/96/97/98 does not have RFO pins connected to anything. Only PA_BOOST
		// pin is connected, so must use PA_BOOST
		// Pout = 2 + OutputPower.
		// The documentation is pretty confusing on this topic: PaSelect says the max power is 20dBm,
		// but OutputPower claims it would be 17dBm.
		// My measurements show 20dBm is correct
		spiWrite(RH_SX1276_REG_09_PA_CONFIG, RH_SX1276_PA_SELECT | (power - 5));
	}
}

// Sets registers from a canned modem configuration structure
void RH_SX1276::setModemRegisters(const ModemConfig* config) {
	spiWrite(RH_SX1276_REG_1D_MODEM_CONFIG1, config->reg_1d);
	spiWrite(RH_SX1276_REG_1E_MODEM_CONFIG2, config->reg_1e);
	spiWrite(RH_SX1276_REG_26_MODEM_CONFIG3, config->reg_26);
}

// Set one of the canned FSK Modem configs
// Returns true if its a valid choice
bool RH_SX1276::setModemConfig(ModemConfigChoice index) {
	if (index > (signed int) (sizeof(MODEM_CONFIG_TABLE) / sizeof(ModemConfig)))
		return false;

	ModemConfig cfg;
	memcpy_P(&cfg, &MODEM_CONFIG_TABLE[index], sizeof(RH_SX1276::ModemConfig));
	setModemRegisters(&cfg);

	return true;
}

// Return the  Modem configs
bool RH_SX1276::getModemConfig(ModemConfigChoice index, ModemConfig* config) {
	if (index > (signed int) (sizeof(MODEM_CONFIG_TABLE) / sizeof(ModemConfig)))
		return false;

	memcpy_P(config, &MODEM_CONFIG_TABLE[index], sizeof(RH_SX1276::ModemConfig));

	return true;
}

void RH_SX1276::setPreambleLength(uint16_t bytes) {
	spiWrite(RH_SX1276_REG_20_PREAMBLE_MSB, bytes >> 8);
	spiWrite(RH_SX1276_REG_21_PREAMBLE_LSB, bytes & 0xff);
}

bool RH_SX1276::isChannelActive() {
	// Set mode RHModeCad
	if (_mode != RHModeCad) {
		spiWrite(RH_SX1276_REG_01_OP_MODE, RH_SX1276_MODE_CAD);
		spiWrite(RH_SX1276_REG_40_DIO_MAPPING1, 0x80); // Interrupt on CadDone
		_mode = RHModeCad;
	}

	while (_mode == RHModeCad)
		YIELD;

	return _cad;
}

void RH_SX1276::enableTCXO() {
	while ((spiRead(RH_SX1276_REG_4B_TCXO) & RH_SX1276_TCXO_TCXO_INPUT_ON) != RH_SX1276_TCXO_TCXO_INPUT_ON) {
		sleep();
		spiWrite(RH_SX1276_REG_4B_TCXO, (spiRead(RH_SX1276_REG_4B_TCXO) | RH_SX1276_TCXO_TCXO_INPUT_ON));
	}
}

