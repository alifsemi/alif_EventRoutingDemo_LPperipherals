#include <stdint.h>
#include <stdio.h>

#include <RTE_Components.h>
#include CMSIS_device_header
#include <Driver_USART.h>
#include <pinconf.h>

#define UART_INSTANCE           LP
#define UART_EARLY_RETURN       1

extern ARM_DRIVER_USART ARM_Driver_USART_(UART_INSTANCE);
static ARM_DRIVER_USART *USARTdrv = &ARM_Driver_USART_(UART_INSTANCE);

uint8_t user_uart_tx_mcode[] = {
        0xBC, 0x01, 0x71, 0x10, 0x1C, 0x04,
        0xBC, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xBC, 0x02, 0x00, 0x80, 0x00, 0x43,
        0x35, 0x00, 0x32, 0x00, 0x07, 0x2B, 0x00,
        0x13, 0x5C, 0xF8, 0xFF, 0x2C, 0x0B
};

volatile uint32_t lpuart_cb_status;

static void configure_lpuart_pinmux()
{
    /* P7_7 is LPUART_TX */
    // pinconf_set(PORT_7, PIN_7, PINMUX_ALTERNATE_FUNCTION_2, 0);

    /* P9_2 is LPUART_TX */
    pinconf_set(PORT_9, PIN_2, PINMUX_ALTERNATE_FUNCTION_1, 0);

    /* P15_1 is LPUART DMA event trigger */
    pinconf_set(PORT_15, LPUART_DMA_TX_PERIPH_REQ - 24, PINMUX_ALTERNATE_FUNCTION_0, PADCTRL_READ_ENABLE);
}

void LPUART_cb_func(uint32_t event)
{
    if (event & ARM_USART_EVENT_SEND_COMPLETE)
    {
        /* Send Success */
        lpuart_cb_status |= event;
    }

    if (event & ARM_USART_EVENT_RECEIVE_COMPLETE)
    {
        /* Receive Success */
        lpuart_cb_status |= event;
    }

    if (event & ARM_USART_EVENT_RX_TIMEOUT)
    {
        /* Receive Success with rx timeout */
        lpuart_cb_status |= event;
    }
}

void LPUART_config()
{
    int32_t ret;
    uint8_t static lpuart_tx_buff[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    uint32_t lpuart_baud = 1000000;

    configure_lpuart_pinmux();

    ret = USARTdrv->Initialize(LPUART_cb_func);
    while(ret != ARM_DRIVER_OK);

    ret = USARTdrv->PowerControl(ARM_POWER_FULL);
    while(ret != ARM_DRIVER_OK);

    ret =  USARTdrv->Control(ARM_USART_MODE_ASYNCHRONOUS |
                             ARM_USART_DATA_BITS_8       |
                             ARM_USART_PARITY_NONE       |
                             ARM_USART_STOP_BITS_1       |
                             ARM_USART_FLOW_CONTROL_NONE, lpuart_baud);
    while(ret != ARM_DRIVER_OK);

    ret =  USARTdrv->Control(ARM_USART_CONTROL_TX, 1);
    while(ret != ARM_DRIVER_OK);

    /* modify the user_tx_mcode with lpspi_tx_buff address */
    uint32_t lpuart_tx_buff_global = LocalToGlobal(lpuart_tx_buff);
    user_uart_tx_mcode[8]  = (uint8_t) ((lpuart_tx_buff_global >>  0) & 0xFF);
    user_uart_tx_mcode[9]  = (uint8_t) ((lpuart_tx_buff_global >>  8) & 0xFF);
    user_uart_tx_mcode[10] = (uint8_t) ((lpuart_tx_buff_global >> 16) & 0xFF);
    user_uart_tx_mcode[11] = (uint8_t) ((lpuart_tx_buff_global >> 24) & 0xFF);
    user_uart_tx_mcode[19] = (uint8_t) (LPUART_DMA_TX_PERIPH_REQ << 3);
    user_uart_tx_mcode[21] = (uint8_t) (LPUART_DMA_TX_PERIPH_REQ << 3);
    user_uart_tx_mcode[24] = (uint8_t) (LPUART_DMA_TX_PERIPH_REQ << 3);

    lpuart_cb_status = 0;

    ret = USARTdrv->Send(lpuart_tx_buff, 8);
    while(ret != ARM_DRIVER_OK);

#if UART_EARLY_RETURN
    return;
#endif

    do {
        while(lpuart_cb_status == 0) __WFI();
        lpuart_cb_status = 0;
    } while(1);

    ret = USARTdrv->PowerControl(ARM_POWER_OFF);
    while(ret != ARM_DRIVER_OK);

    ret = USARTdrv->Uninitialize();
    while(ret != ARM_DRIVER_OK);
}
