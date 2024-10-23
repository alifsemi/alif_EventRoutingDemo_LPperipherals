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
#include <Driver_LPTIMER.h>
#include <pinconf.h>

#ifndef HW_REG32
#define HW_REG32(u,v)   *((volatile uint32_t *)(u + v))
#endif

#define LPTIMER_CHANNEL         0
#define LPTIMER_INT_DISABLE     1
#define LPTIMER_EARLY_RETURN    1

extern ARM_DRIVER_LPTIMER DRIVER_LPTIMER0;
static ARM_DRIVER_LPTIMER *ptrLPTIMER = &DRIVER_LPTIMER0;

volatile uint32_t lptimer_cb_status = 0;

static void LPTIMER_cb_func(uint8_t event)
{
    if (event == ARM_LPTIMER_EVENT_UNDERFLOW)
    {
        lptimer_cb_status |= event;
    }
}

void LPTIMER_config()
{
    int32_t ret;
    uint32_t count = 31;    /* 32768 Hz / (31+1) = 1024 Hz */
    //uint32_t count = 15;    /* 32768 Hz / (15+1) = 2048 Hz */
    //uint32_t count = 7;     /* 32768 Hz / ( 7+1) = 4096 Hz */
    uint8_t channel = LPTIMER_CHANNEL;

    /* make LPTIMER output available through LPGPIO pin */
    pinconf_set(PORT_15, channel, 0, 0);
    HW_REG32(LPGPIO_BASE, 0x08) = 1U << channel;

    /* Initialize LPTIMER */
    ret = ptrLPTIMER->Initialize(channel, LPTIMER_cb_func);
    while(ret != ARM_DRIVER_OK);

    ret = ptrLPTIMER->PowerControl(channel, ARM_POWER_FULL);
    while(ret != ARM_DRIVER_OK);

    ret = ptrLPTIMER->Control(channel, ARM_LPTIMER_SET_COUNT1, &count);
    while(ret != ARM_DRIVER_OK);

    lptimer_cb_status = 0;

    ret = ptrLPTIMER->Start(channel);
    while(ret != ARM_DRIVER_OK);

#if LPTIMER_INT_DISABLE
    NVIC_DisableIRQ(LPTIMER0_IRQ_IRQn + channel);
#endif

#if LPTIMER_EARLY_RETURN
    return;
#endif

    do {
        while (lptimer_cb_status == 0) __WFI();
        lptimer_cb_status = 0;
    } while(1);

    ret = ptrLPTIMER->Stop(channel);

    /* Uninitialize LPTIMER */
    ret = ptrLPTIMER->PowerControl(channel, ARM_POWER_OFF);
    while(ret != ARM_DRIVER_OK);

    ret = ptrLPTIMER->Uninitialize(channel);
    while(ret != ARM_DRIVER_OK);
}
