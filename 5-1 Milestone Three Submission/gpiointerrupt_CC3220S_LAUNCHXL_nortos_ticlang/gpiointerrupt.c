/*
 * Copyright (c) 2015-2020, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

//======== gpiointerrupt.c ========
#include <stdint.h>
#include <stddef.h>
#include <ti/drivers/GPIO.h>// Driver Header files
#include <ti/drivers/Timer.h>// Timer
#include "ti_drivers_config.h"// Driver configuration

// Flash code message states
enum CURRENT_FLASHES {SOS, TEST} CURRENT_FLASH, BUTTON_STATE;
// LED code states
enum LED_STATES {LED_RED, LED_GREEN, LED_IDLE} LED_STATE;
// Flash messages
enum LED_STATES sosFlash[] = {
    // S
    LED_RED, LED_IDLE, // DOT, pause
    LED_RED, LED_IDLE, // DOT, pause
    LED_RED, LED_IDLE, LED_IDLE, LED_IDLE, // DOT, character pause
    // O
    LED_GREEN, LED_GREEN, LED_GREEN, LED_IDLE, // DASH, pause
    LED_GREEN, LED_GREEN, LED_GREEN, LED_IDLE, // DASH, pause
    LED_GREEN, LED_GREEN, LED_GREEN, LED_IDLE, LED_IDLE, LED_IDLE, // DASH, character pause
    // S
    LED_RED, LED_IDLE, // DOT, pause
    LED_RED, LED_IDLE, // DOT, pause
    LED_RED, LED_IDLE, LED_IDLE, LED_IDLE, // DOT, character pause
    LED_IDLE, LED_IDLE, LED_IDLE, LED_IDLE, LED_IDLE, LED_IDLE, LED_IDLE // message flash pause
};

enum LED_STATES hiFlash[] = {
    // H
    LED_RED, LED_IDLE, // DOT, pause
    LED_RED, LED_IDLE, // DOT, pause
    LED_RED, LED_IDLE, // DOT, pause
    LED_RED, LED_IDLE, LED_IDLE, LED_IDLE, LED_IDLE, // DOT, character pause
    // I
    LED_RED, LED_IDLE, // DOT, pause
    LED_RED, LED_IDLE, LED_IDLE, LED_IDLE, LED_IDLE, // DOT, character pause
    LED_IDLE, LED_IDLE, LED_IDLE, LED_IDLE, LED_IDLE, LED_IDLE, LED_IDLE  // message flash pause
};
unsigned int messageTimer = 0;// message counter integer

//======== setLEDs ========
void setLEDS() {// dot, dash, or break.
    switch(LED_STATE) {
        case LED_RED:   // DOT
            GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_ON);
            GPIO_write(CONFIG_GPIO_LED_1, CONFIG_GPIO_LED_OFF);
            break;
        case LED_GREEN: // DASH
            GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_OFF);
            GPIO_write(CONFIG_GPIO_LED_1, CONFIG_GPIO_LED_ON);
            break;
        case LED_IDLE:   // BREAK Pause
            GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_OFF);
            GPIO_write(CONFIG_GPIO_LED_1, CONFIG_GPIO_LED_OFF);
            break;
        default:
            break;
    }
}

//======== timerCallback ========
// function for timer interrupt.

void timerCallback(Timer_Handle myHandle, int_fast16_t status)
{
    switch(CURRENT_FLASH) {
        case SOS:
            LED_STATE = sosFlash[messageTimer];
            setLEDS();
            messageTimer++;
            if(messageTimer == (sizeof(sosFlash)/sizeof(sosFlash[0]))) {
                CURRENT_FLASH = BUTTON_STATE;
                messageTimer = 0;
            }
            break;
        case TEST:
            LED_STATE = hiFlash[messageTimer];
            setLEDS();
            messageTimer++;
            if(messageTimer == (sizeof(hiFlash)/sizeof(hiFlash[0]))) {
                CURRENT_FLASH = BUTTON_STATE;
                messageTimer = 0;
            }
            break;
        default:
            break;
    }
}

// ======== timerInit ========
// Initialization function for the timer interrupt on timer0.
void initTimer(void)
{
    Timer_Handle timer0;
    Timer_Params params;
    Timer_init();
    Timer_Params_init(&params);
    params.period = 500;
    params.periodUnits = Timer_PERIOD_US;
    params.timerMode = Timer_CONTINUOUS_CALLBACK;
    params.timerCallback = timerCallback;
    timer0 = Timer_open(CONFIG_TIMER_0, &params);
    if(timer0 == NULL) {
        while (1) {}/* Failed to initialized timer */
    }

    if(Timer_start(timer0) == Timer_STATUS_ERROR)
    {
        while (1) {}/* Failed to start timer */
    }
}

//======== gpioCallback ========
void gpioCallback(uint_least8_t index)
{
    switch(BUTTON_STATE) {
        case SOS:
            BUTTON_STATE = TEST;
            break;
        case TEST:
            BUTTON_STATE = SOS;
            break;
        default:
            break;
    }
}

//======== mainThread ========
void *mainThread(void *arg0)
{
    // Call driver in it functions for GPIO and timer
    GPIO_init();
    initTimer();
    // Configure the LED and button pins
    GPIO_setConfig(CONFIG_GPIO_LED_0, GPIO_CFG_OUT_STD | GPIO_CFG_OUT_LOW);
    GPIO_setConfig(CONFIG_GPIO_LED_1, GPIO_CFG_OUT_STD | GPIO_CFG_OUT_LOW);
    GPIO_setConfig(CONFIG_GPIO_BUTTON_0, GPIO_CFG_IN_PU | GPIO_CFG_IN_INT_FALLING);
    // Start with LEDs off
    GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_OFF);
    GPIO_write(CONFIG_GPIO_LED_1, CONFIG_GPIO_LED_OFF);
    // Set initial states to SOS
    BUTTON_STATE = SOS;
    CURRENT_FLASH = BUTTON_STATE;
    // Install Button callback
    GPIO_setCallback(CONFIG_GPIO_BUTTON_0, gpioCallback);
    // Enable interrupts
    GPIO_enableInt(CONFIG_GPIO_BUTTON_0);

    if (CONFIG_GPIO_BUTTON_0 != CONFIG_GPIO_BUTTON_1) {
        // Configure BUTTON1 pin
        GPIO_setConfig(CONFIG_GPIO_BUTTON_1, GPIO_CFG_IN_PU | GPIO_CFG_IN_INT_FALLING);
        // Install Button callback
        GPIO_setCallback(CONFIG_GPIO_BUTTON_1, gpioCallback);
        GPIO_enableInt(CONFIG_GPIO_BUTTON_1);
    }

    return (NULL);
}
