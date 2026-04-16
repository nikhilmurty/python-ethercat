#include "Ethercat.h"

// --- SPI Hardware Abstraction ---
static inline uint8_t SPI_TransferRx(EasyCAT_HandleTypeDef *ecat, uint8_t data) {
    uint8_t rxData;
    HAL_SPI_TransmitReceive(ecat->hspi, &data, &rxData, 1, 10);
    return rxData;
}

static inline void SPI_TransferTx(EasyCAT_HandleTypeDef *ecat, uint8_t data) {
    HAL_SPI_Transmit(ecat->hspi, &data, 1, 10);
}

static inline void SCS_Low(EasyCAT_HandleTypeDef *ecat) {
    HAL_GPIO_WritePin(ecat->CS_Port, ecat->CS_Pin, GPIO_PIN_RESET);
}

static inline void SCS_High(EasyCAT_HandleTypeDef *ecat) {
    HAL_GPIO_WritePin(ecat->CS_Port, ecat->CS_Pin, GPIO_PIN_SET);
}

// --- Internal Prototypes ---
static uint32_t SPIReadRegisterDirect(EasyCAT_HandleTypeDef *ecat, uint16_t Address, uint8_t Len);
static void SPIWriteRegisterDirect(EasyCAT_HandleTypeDef *ecat, uint16_t Address, uint32_t DataOut);
static uint32_t SPIReadRegisterIndirect(EasyCAT_HandleTypeDef *ecat, uint16_t Address, uint8_t Len);
static void SPIWriteRegisterIndirect(EasyCAT_HandleTypeDef *ecat, uint32_t DataOut, uint16_t Address, uint8_t Len);
static void SPIReadProcRamFifo(EasyCAT_HandleTypeDef *ecat);
static void SPIWriteProcRamFifo(EasyCAT_HandleTypeDef *ecat);

// --- Initialization ---
bool EasyCAT_Init(EasyCAT_HandleTypeDef *ecat) {
    ULONG TempLong;
    uint32_t tickstart;

    SCS_High(ecat);
    HAL_Delay(100);

    // 1. LAN9252 Reset
    SPIWriteRegisterDirect(ecat, RESET_CTL, DIGITAL_RST);
    tickstart = HAL_GetTick();
    do {
        TempLong.Long = SPIReadRegisterDirect(ecat, RESET_CTL, 4);
        if ((HAL_GetTick() - tickstart) > 1000) return false; // Timeout
    } while ((TempLong.Byte[0] & 0x01) != 0x00);

    // 2. Byte Order Test
    tickstart = HAL_GetTick();
    do {
        TempLong.Long = SPIReadRegisterDirect(ecat, BYTE_TEST, 4);
        if ((HAL_GetTick() - tickstart) > 1000) return false;
    } while (TempLong.Long != 0x87654321);

    // 3. Hardware Ready Check
    tickstart = HAL_GetTick();
    do {
        TempLong.Long = SPIReadRegisterDirect(ecat, HW_CFG, 4);
        if ((HAL_GetTick() - tickstart) > 1000) return false;
    } while ((TempLong.Byte[3] & READY) == 0);

    // 4. Sync Configuration
    if ((ecat->Sync == DC_SYNC) || (ecat->Sync == SM_SYNC)) {
        if (ecat->Sync == DC_SYNC) {
            SPIWriteRegisterIndirect(ecat, 0x00000004, AL_EVENT_MASK, 4);
        } else {
            SPIWriteRegisterIndirect(ecat, 0x00000100, AL_EVENT_MASK, 4);
        }
        SPIWriteRegisterDirect(ecat, IRQ_CFG, 0x00000111);
        SPIWriteRegisterDirect(ecat, INT_EN, 0x00000001);
    }

    return true;
}

// --- Main Task ---
uint8_t EasyCAT_MainTask(EasyCAT_HandleTypeDef *ecat) {
    bool WatchDog = 0;
    bool Operational = 0;
    ULONG TempLong;
    uint8_t Status;

    TempLong.Long = SPIReadRegisterIndirect(ecat, WDOG_STATUS, 1);
    if ((TempLong.Byte[0] & 0x01) == 0x01) WatchDog = 0;
    else WatchDog = 1;

    TempLong.Long = SPIReadRegisterIndirect(ecat, AL_STATUS, 1);
    Status = TempLong.Byte[0] & 0x0F;
    if (Status == ESM_OP) Operational = 1;
    else Operational = 0;

    // Process data transfer
    if (WatchDog || !Operational) {
        for (uint8_t i = 0; i < TOT_BYTE_NUM_ROUND_OUT; i++) {
            ecat->BufferOut.Byte[i] = 0;
        }
    } else {
        SPIReadProcRamFifo(ecat);
    }

    SPIWriteProcRamFifo(ecat);

    if (WatchDog) Status |= 0x80;
    return Status;
}

void EasyCAT_WriteAlias(EasyCAT_HandleTypeDef *ecat, uint16_t Alias) {
    SPIWriteRegisterIndirect(ecat, Alias, ALIAS, 2);
}

// --- Register Access Functions ---
static uint32_t SPIReadRegisterDirect(EasyCAT_HandleTypeDef *ecat, uint16_t Address, uint8_t Len) {
    ULONG Result;
    Result.Long = 0;
    UWORD Addr;
    Addr.Word = Address;

    SCS_Low(ecat);
    SPI_TransferTx(ecat, COMM_SPI_READ);
    SPI_TransferTx(ecat, Addr.Byte[1]);
    SPI_TransferTx(ecat, Addr.Byte[0]);

    for (uint8_t i = 0; i < Len; i++) {
        Result.Byte[i] = SPI_TransferRx(ecat, DUMMY_BYTE);
    }
    SCS_High(ecat);
    return Result.Long;
}

static void SPIWriteRegisterDirect(EasyCAT_HandleTypeDef *ecat, uint16_t Address, uint32_t DataOut) {
    ULONG Data;
    UWORD Addr;
    Addr.Word = Address;
    Data.Long = DataOut;

    SCS_Low(ecat);
    SPI_TransferTx(ecat, COMM_SPI_WRITE);
    SPI_TransferTx(ecat, Addr.Byte[1]);
    SPI_TransferTx(ecat, Addr.Byte[0]);

    SPI_TransferTx(ecat, Data.Byte[0]);
    SPI_TransferTx(ecat, Data.Byte[1]);
    SPI_TransferTx(ecat, Data.Byte[2]);
    SPI_TransferTx(ecat, Data.Byte[3]);
    SCS_High(ecat);
}

static uint32_t SPIReadRegisterIndirect(EasyCAT_HandleTypeDef *ecat, uint16_t Address, uint8_t Len) {
    ULONG TempLong;
    UWORD Addr;
    Addr.Word = Address;

    TempLong.Byte[0] = Addr.Byte[0];
    TempLong.Byte[1] = Addr.Byte[1];
    TempLong.Byte[2] = Len;
    TempLong.Byte[3] = ESC_READ;

    SPIWriteRegisterDirect(ecat, ECAT_CSR_CMD, TempLong.Long);

    do {
        TempLong.Long = SPIReadRegisterDirect(ecat, ECAT_CSR_CMD, 4);
    } while(TempLong.Byte[3] & ECAT_CSR_BUSY);

    TempLong.Long = SPIReadRegisterDirect(ecat, ECAT_CSR_DATA, Len);
    return TempLong.Long;
}

static void SPIWriteRegisterIndirect(EasyCAT_HandleTypeDef *ecat, uint32_t DataOut, uint16_t Address, uint8_t Len) {
    ULONG TempLong;
    UWORD Addr;
    Addr.Word = Address;

    SPIWriteRegisterDirect(ecat, ECAT_CSR_DATA, DataOut);

    TempLong.Byte[0] = Addr.Byte[0];
    TempLong.Byte[1] = Addr.Byte[1];
    TempLong.Byte[2] = Len;
    TempLong.Byte[3] = ESC_WRITE;

    SPIWriteRegisterDirect(ecat, ECAT_CSR_CMD, TempLong.Long);

    do {
        TempLong.Long = SPIReadRegisterDirect(ecat, ECAT_CSR_CMD, 4);
    } while (TempLong.Byte[3] & ECAT_CSR_BUSY);
}

// --- Process RAM FIFOs ---
static void SPIReadProcRamFifo(EasyCAT_HandleTypeDef *ecat) {
    ULONG TempLong;

#if TOT_BYTE_NUM_OUT > 0
    SPIWriteRegisterDirect(ecat, ECAT_PRAM_RD_CMD, PRAM_ABORT);
    SPIWriteRegisterDirect(ecat, ECAT_PRAM_RD_ADDR_LEN, (0x00001000 | (((uint32_t)TOT_BYTE_NUM_OUT) << 16)));
    SPIWriteRegisterDirect(ecat, ECAT_PRAM_RD_CMD, 0x80000000);

    do {
        TempLong.Long = SPIReadRegisterDirect(ecat, ECAT_PRAM_RD_CMD, 2);
    } while (TempLong.Byte[1] != (FST_BYTE_NUM_ROUND_OUT/4));

    SCS_Low(ecat);
    SPI_TransferTx(ecat, COMM_SPI_READ);
    SPI_TransferTx(ecat, 0x00);
    SPI_TransferTx(ecat, 0x00);

    for (uint8_t i=0; i< FST_BYTE_NUM_ROUND_OUT; i++) {
        ecat->BufferOut.Byte[i] = SPI_TransferRx(ecat, DUMMY_BYTE);
    }
    SCS_High(ecat);
#endif

#if SEC_BYTE_NUM_OUT > 0
    do {
        TempLong.Long = SPIReadRegisterDirect(ecat, ECAT_PRAM_RD_CMD, 2);
    } while (TempLong.Byte[1] != SEC_BYTE_NUM_ROUND_OUT/4);

    SCS_Low(ecat);
    SPI_TransferTx(ecat, COMM_SPI_READ);
    SPI_TransferTx(ecat, 0x00);
    SPI_TransferTx(ecat, 0x00);

    for (uint8_t i=0; i< (SEC_BYTE_NUM_ROUND_OUT); i++) {
        ecat->BufferOut.Byte[i+64] = SPI_TransferRx(ecat, DUMMY_BYTE);
    }
    SCS_High(ecat);
#endif
}

static void SPIWriteProcRamFifo(EasyCAT_HandleTypeDef *ecat) {
    ULONG TempLong;

#if TOT_BYTE_NUM_IN > 0
    SPIWriteRegisterDirect(ecat, ECAT_PRAM_WR_CMD, PRAM_ABORT);
    SPIWriteRegisterDirect(ecat, ECAT_PRAM_WR_ADDR_LEN, (0x00001200 | (((uint32_t)TOT_BYTE_NUM_IN) << 16)));
    SPIWriteRegisterDirect(ecat, ECAT_PRAM_WR_CMD, 0x80000000);

    do {
        TempLong.Long = SPIReadRegisterDirect(ecat, ECAT_PRAM_WR_CMD, 2);
    } while (TempLong.Byte[1] < (FST_BYTE_NUM_ROUND_IN/4));

    SCS_Low(ecat);
    SPI_TransferTx(ecat, COMM_SPI_WRITE);
    SPI_TransferTx(ecat, 0x00);
    SPI_TransferTx(ecat, 0x20);

    for (uint8_t i=0; i< FST_BYTE_NUM_ROUND_IN; i++) {
        SPI_TransferTx(ecat, ecat->BufferIn.Byte[i]);
    }
    SCS_High(ecat);
#endif

#if SEC_BYTE_NUM_IN > 0
    do {
        TempLong.Long = SPIReadRegisterDirect(ecat, ECAT_PRAM_WR_CMD, 2);
    } while (TempLong.Byte[1] < (SEC_BYTE_NUM_ROUND_IN/4));

    SCS_Low(ecat);
    SPI_TransferTx(ecat, COMM_SPI_WRITE);
    SPI_TransferTx(ecat, 0x00);
    SPI_TransferTx(ecat, 0x20);

    for (uint8_t i=0; i< SEC_BYTE_NUM_ROUND_IN; i++) {
        SPI_TransferTx(ecat, ecat->BufferIn.Byte[i+64]);
    }
    SCS_High(ecat);
#endif
}
