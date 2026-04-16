/*
 * Ethercat.h
 *
 *  Created on: Apr 15, 2026
 *      Author: Nikhil Murty
 */

#ifndef ETHERCAT_H
#define ETHERCAT_H

#include "main.h"
#include "slave_1.h" // your custom pdos that were made by the easy configurator tool, replace this with whatever .h file is generated for your project
#include <stdbool.h>

// --- Macro Calculations for Buffer Splitting ---
#define TOT_BYTE_NUM_OUT  CUST_BYTE_NUM_OUT
#define TOT_BYTE_NUM_IN   CUST_BYTE_NUM_IN

#if TOT_BYTE_NUM_OUT > 64
  #define SEC_BYTE_NUM_OUT  (TOT_BYTE_NUM_OUT - 64)
  #if ((SEC_BYTE_NUM_OUT & 0x03) != 0x00)
    #define SEC_BYTE_NUM_ROUND_OUT  ((SEC_BYTE_NUM_OUT | 0x03) + 1)
  #else
    #define SEC_BYTE_NUM_ROUND_OUT  SEC_BYTE_NUM_OUT
  #endif
  #define FST_BYTE_NUM_ROUND_OUT  64
#else
  #if ((TOT_BYTE_NUM_OUT & 0x03) != 0x00)
    #define FST_BYTE_NUM_ROUND_OUT ((TOT_BYTE_NUM_OUT | 0x03) + 1)
  #else
    #define FST_BYTE_NUM_ROUND_OUT  TOT_BYTE_NUM_OUT
  #endif
  #define SEC_BYTE_NUM_OUT  0
  #define SEC_BYTE_NUM_ROUND_OUT  0
#endif

#if TOT_BYTE_NUM_IN > 64
  #define SEC_BYTE_NUM_IN  (TOT_BYTE_NUM_IN - 64)
  #if ((SEC_BYTE_NUM_IN & 0x03) != 0x00)
    #define SEC_BYTE_NUM_ROUND_IN  ((SEC_BYTE_NUM_IN | 0x03) + 1)
  #else
    #define SEC_BYTE_NUM_ROUND_IN  SEC_BYTE_NUM_IN
  #endif
  #define FST_BYTE_NUM_ROUND_IN  64
#else
  #if ((TOT_BYTE_NUM_IN & 0x03) != 0x00)
    #define FST_BYTE_NUM_ROUND_IN ((TOT_BYTE_NUM_IN | 0x03) + 1)
  #else
    #define FST_BYTE_NUM_ROUND_IN  TOT_BYTE_NUM_IN
  #endif
  #define SEC_BYTE_NUM_IN  0
  #define SEC_BYTE_NUM_ROUND_IN  0
#endif

// --- LAN9252 Registers ---
#define ECAT_CSR_DATA           0x0300
#define ECAT_CSR_CMD            0x0304
#define ECAT_PRAM_RD_ADDR_LEN   0x0308
#define ECAT_PRAM_RD_CMD        0x030C
#define ECAT_PRAM_WR_ADDR_LEN   0x0310
#define ECAT_PRAM_WR_CMD        0x0314
#define ECAT_PRAM_RD_DATA       0x0000
#define ECAT_PRAM_WR_DATA       0x0020
#define AL_CONTROL              0x0120
#define AL_STATUS               0x0130
#define AL_STATUS_CODE          0x0134
#define AL_EVENT                0x0220
#define AL_EVENT_MASK           0x0204
#define WDOG_STATUS             0x0440
#define SM0_BASE                0x0800
#define SM1_BASE                0x0808
#define ALIAS                   0x0012

#define HW_CFG                  0x0074
#define BYTE_TEST               0x0064
#define RESET_CTL               0x01F8
#define ID_REV                  0x0050
#define IRQ_CFG                 0x0054
#define INT_EN                  0x005C

// --- Flags ---
#define ECAT_CSR_BUSY     0x80
#define PRAM_ABORT        0x40000000
#define PRAM_BUSY         0x80
#define PRAM_AVAIL        0x01
#define READY             0x08
#define DIGITAL_RST       0x00000001
#define ALEVENT_CONTROL   0x0001
#define ALEVENT_SM        0x0010

#define ESM_INIT          0x01
#define ESM_PREOP         0x02
#define ESM_BOOT          0x03
#define ESM_SAFEOP        0x04
#define ESM_OP            0x08

#define ESC_WRITE 		  0x80
#define ESC_READ 		  0xC0
#define COMM_SPI_READ     0x03
#define COMM_SPI_WRITE    0x02
#define DUMMY_BYTE        0xFF

typedef union {
    uint16_t  Word;
    uint8_t   Byte[2];
} UWORD;

typedef union {
    uint32_t  Long;
    uint16_t  Word[2];
    uint8_t   Byte[4];
} ULONG;

typedef enum {
  ASYNC,
  DC_SYNC,
  SM_SYNC
} SyncMode;

// --- STM32 Hardware Structure ---
typedef struct {
    SPI_HandleTypeDef *hspi;
    GPIO_TypeDef      *CS_Port;
    uint16_t          CS_Pin;
    SyncMode          Sync;
    PROCBUFFER_OUT    BufferOut;
    PROCBUFFER_IN     BufferIn;
} EasyCAT_HandleTypeDef;

// --- Prototypes ---
bool EasyCAT_Init(EasyCAT_HandleTypeDef *hecat);
uint8_t EasyCAT_MainTask(EasyCAT_HandleTypeDef *hecat);
void EasyCAT_WriteAlias(EasyCAT_HandleTypeDef *hecat, uint16_t Alias);

#endif // EASYCAT_H
