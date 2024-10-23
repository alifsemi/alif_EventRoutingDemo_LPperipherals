/* Copyright (C) 2024 Alif Semiconductor - All Rights Reserved.
 * Use, distribution and modification of this code is permitted under the
 * terms stated in the Alif Semiconductor Software License Agreement
 *
 * You should have received a copy of the Alif Semiconductor Software
 * License Agreement with this file. If not, please write to:
 * contact@alifsemi.com, or visit: https://alifsemi.com/license
 *
 */
#include <stdint.h>
#include <stdio.h>

#include <RTE_Components.h>
#include CMSIS_device_header
#include <Driver_SPI.h>
#include <pinconf.h>

#ifndef HW_REG32
#define HW_REG32(u,v)   *((volatile uint32_t *)(u + v))
#endif

#define SPI_INSTANCE            LP
#define SPI_TRANSFER_COUNT      8
#define SPI_TRANSFER_MULTIPLE   8
#define SPI_EARLY_RETURN        1

extern ARM_DRIVER_SPI ARM_Driver_SPI_(SPI_INSTANCE);
static ARM_DRIVER_SPI *ptrLPSPI = &ARM_Driver_SPI_(SPI_INSTANCE);

uint8_t user_spi_rx_mcode[] = {
        0xBC, 0x01, 0x04, 0x50, 0x01, 0x04,
        0xBC, 0x00, 0x60, 0x00, 0x00, 0x43,
        0xBC, 0x02, 0x00, 0x00, 0x00, 0x00,
        0x22, 0x07, 0x20, 0x07, 0x35, 0x60,
        0x30, 0x60, 0x25, 0x60, 0x09, 0x13,
        0x38, 0x08, 0x3C, 0x0C, 0x34, 0x00,
        0x5E, 0x00, 0xFF, 0x2C, 0x15
};

uint8_t user_spi_tx_mcode[] = {
        0xBC, 0x01, 0x05, 0x10, 0x01, 0x04,
        0xBC, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xBC, 0x02, 0x60, 0x00, 0x00, 0x43,
        0x20, 0x07, 0x35, 0xC0, 0x30, 0xC0,
        0x25, 0xC0, 0x09, 0x13, 0x38, 0x08,
        0x5C, 0xE0, 0xFF, 0x2C, 0x0F
};

volatile uint32_t lpspi_cb_status = 0;

static void configure_lpspi_pinmux()
{
    uint32_t padconf = PADCTRL_READ_ENABLE |
            PADCTRL_OUTPUT_DRIVE_STRENGTH_2MA;

    /* LPSPI on Port 7 */
    // pinconf_set(PORT_7, PIN_4, PINMUX_ALTERNATE_FUNCTION_4, padconf);   /* P7_4 is LPSPI_MISO */
    // pinconf_set(PORT_7, PIN_5, PINMUX_ALTERNATE_FUNCTION_5, padconf);   /* P7_5 is LPSPI_MOSI */
    // pinconf_set(PORT_7, PIN_6, PINMUX_ALTERNATE_FUNCTION_5, padconf);   /* P7_6 is LPSPI_SCLK */
    // pinconf_set(PORT_7, PIN_7, PINMUX_ALTERNATE_FUNCTION_5, padconf);   /* P7_7 is LPSPI_SS0 */

    /* LPSPI on Port 11 */
    pinconf_set(PORT_11, PIN_4, PINMUX_ALTERNATE_FUNCTION_4, padconf);   /* P11_4 is LPSPI_MISO */
    pinconf_set(PORT_11, PIN_5, PINMUX_ALTERNATE_FUNCTION_4, padconf);   /* P11_5 is LPSPI_MOSI */
    pinconf_set(PORT_11, PIN_6, PINMUX_ALTERNATE_FUNCTION_4, padconf);   /* P11_6 is LPSPI_SCLK */
    pinconf_set(PORT_11, PIN_7, PINMUX_ALTERNATE_FUNCTION_4, padconf);   /* P11_7 is LPSPI_SS0 */

    /* P15_2 is LPSPI DMA event trigger */
    HW_REG32(LPGPIO_CTRL_BASE, ((LPSPI_DMA_TX_PERIPH_REQ - 24) * 0x04)) = 1U;
}

static void LPSPI_cb_func(uint32_t event)
{
    lpspi_cb_status |= event;
}

void LPSPI_config()
{
    int32_t ret;
    uint32_t static lpspi_tx_buff[SPI_TRANSFER_COUNT] = {0x111111, 0x222222, 0x333333, 0x444444, 0x555555, 0x666666, 0x777777, 0x888888};
    uint32_t static lpspi_rx_buff[SPI_TRANSFER_COUNT * SPI_TRANSFER_MULTIPLE] = {0};
    uint32_t lpspi_freq   = 4800000;
    uint32_t lpspi_config = ARM_SPI_MODE_MASTER |
            ARM_SPI_SS_MASTER_HW_OUTPUT |
            ARM_SPI_CPOL0_CPHA0 |
            ARM_SPI_DATA_BITS(24);

    configure_lpspi_pinmux();

    ret = ptrLPSPI->Initialize(LPSPI_cb_func);
    while(ret != ARM_DRIVER_OK);

    ret = ptrLPSPI->PowerControl(ARM_POWER_FULL);
    while(ret != ARM_DRIVER_OK);

    ret = ptrLPSPI->Control(lpspi_config, lpspi_freq);
    while(ret != ARM_DRIVER_OK);

    ret = ptrLPSPI->Control(ARM_SPI_CONTROL_SS, ARM_SPI_SS_ACTIVE);
    while(ret != ARM_DRIVER_OK);

    /* modify the user_rx_mcode with spi_rx_buff address */
    uint32_t spi_rx_buff_global = LocalToGlobal(lpspi_rx_buff);
    uint32_t spi_rx_dma_increment = 65536 - (SPI_TRANSFER_COUNT * SPI_TRANSFER_MULTIPLE * sizeof(uint32_t));
    user_spi_rx_mcode[14] = (uint8_t) ((spi_rx_buff_global >>  0) & 0xFF);
    user_spi_rx_mcode[15] = (uint8_t) ((spi_rx_buff_global >>  8) & 0xFF);
    user_spi_rx_mcode[16] = (uint8_t) ((spi_rx_buff_global >> 16) & 0xFF);
    user_spi_rx_mcode[17] = (uint8_t) ((spi_rx_buff_global >> 24) & 0xFF);
    user_spi_rx_mcode[19] = (uint8_t) (SPI_TRANSFER_MULTIPLE - 1);
    user_spi_rx_mcode[37] = (uint8_t) (spi_rx_dma_increment >> 0) & 0xFF;
    user_spi_rx_mcode[38] = (uint8_t) (spi_rx_dma_increment >> 8) & 0xFF;

    /* modify the user_tx_mcode with lpspi_tx_buff address */
    uint32_t spi_tx_buff_global = LocalToGlobal(lpspi_tx_buff);
    user_spi_tx_mcode[8]  = (uint8_t) ((spi_tx_buff_global >>  0) & 0xFF);
    user_spi_tx_mcode[9]  = (uint8_t) ((spi_tx_buff_global >>  8) & 0xFF);
    user_spi_tx_mcode[10] = (uint8_t) ((spi_tx_buff_global >> 16) & 0xFF);
    user_spi_tx_mcode[11] = (uint8_t) ((spi_tx_buff_global >> 24) & 0xFF);
    user_spi_tx_mcode[21] = (uint8_t) (LPSPI_DMA_TX_PERIPH_REQ << 3);
    user_spi_tx_mcode[23] = (uint8_t) (LPSPI_DMA_TX_PERIPH_REQ << 3);
    user_spi_tx_mcode[25] = (uint8_t) (LPSPI_DMA_TX_PERIPH_REQ << 3);

    lpspi_cb_status = 0;

    do {
        ret = ptrLPSPI->Transfer(lpspi_tx_buff, lpspi_rx_buff, SPI_TRANSFER_COUNT * SPI_TRANSFER_MULTIPLE);
    }
    while(ret == ARM_DRIVER_ERROR_BUSY);
    while(ret != ARM_DRIVER_OK);

#if SPI_EARLY_RETURN
    return;
#endif

    do {
        while(lpspi_cb_status == 0) __WFI();
        lpspi_cb_status = 0;
    } while(1);

    ret = ptrLPSPI->PowerControl(ARM_POWER_OFF);
    while(ret != ARM_DRIVER_OK);

    ret = ptrLPSPI->Uninitialize();
    while(ret != ARM_DRIVER_OK);
}
