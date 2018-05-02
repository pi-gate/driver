// Pi-Gate board
//==============
#define RF433_CS_PIN  RPI_V2_GPIO_P1_26 // Slave Select on CE0 so P1 connector pin #24
#define RF433_IRQ_PIN RPI_V2_GPIO_P1_15 // IRQ on GPIO25 so P1 connector pin #22
#define RF433_RST_PIN RPI_V2_GPIO_P1_16 // Reset on GPIO5 so P1 connector pin #29
#define RF433_TXE_PIN NOT_A_PIN

#define RF868_CS_PIN  RPI_V2_GPIO_P1_24 // Slave Select on CE0 so P1 connector pin #24
#define RF868_IRQ_PIN RPI_V2_GPIO_P1_18 // IRQ on GPIO25 so P1 connector pin #22
#define RF868_RST_PIN RPI_V2_GPIO_P1_22 // Reset on GPIO5 so P1 connector pin #29
#define RF868_TXE_PIN RPI_V2_GPIO_P1_13 // Reset on GPIO5 so P1 connector pin #29
