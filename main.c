/** * Copyright (c) 2014 - 2021, Nordic Semiconductor ASA * * All rights reserved. * * Redistribution and use in source and binary forms, with or without modification, * are permitted provided that the following conditions are met: * * 1. Redistributions of source code must retain the above copyright notice, this * list of conditions and the following disclaimer. * * 2. Redistributions in binary form, except as embedded into a Nordic * Semiconductor ASA integrated circuit in a product or a software update for * such product, must reproduce the above copyright notice, this list of * conditions and the following disclaimer in the documentation and/or other * materials provided with the distribution. * * 3. Neither the name of Nordic Semiconductor ASA nor the names of its * contributors may be used to endorse or promote products derived from this * software without specific prior written permission. * * 4. This software, with or without modification, must only be used with a * Nordic Semiconductor ASA integrated circuit. * * 5. Any software provided in binary form under this license must not be reverse * engineered, decompiled, modified and/or disassembled. * * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. * */ /** @file * * @defgroup blinky_example_main main.c * @{ * @ingroup blinky_example * @brief Blinky Example Application main file. * * This file contains the source code for a sample application to blink LEDs. * */ 
#include <stdbool.h> 
#include <stdint.h> 
#include "nrf_delay.h" 
#include "nrf_gpio.h" 

#define LED_RED_PIN NRF_GPIO_PIN_MAP(0,8) 
#define LED_GREEN_PIN NRF_GPIO_PIN_MAP(1,9)  
#define LED_BLUE_PIN NRF_GPIO_PIN_MAP(0,12)  
#define USER_BUTTON_PIN NRF_GPIO_PIN_MAP(1,6) 

void rgb_init(){ 
    nrf_gpio_cfg_output(LED_RED_PIN); 
    nrf_gpio_cfg_output(LED_GREEN_PIN); 
    nrf_gpio_cfg_output(LED_BLUE_PIN); 

    nrf_gpio_pin_set(LED_RED_PIN); 
    nrf_gpio_pin_set(LED_GREEN_PIN); 
    nrf_gpio_pin_set(LED_BLUE_PIN); 

    nrf_gpio_cfg_input(USER_BUTTON_PIN, NRF_GPIO_PIN_PULLUP); 
} 
/** * @brief Function for application main entry. */ 
int main(void) { 

    rgb_init(); 

    int nums[4] = {6,4,5}; 
    int temp[4] = {6,4,5}; 
    int i = 0; 

    while (true) { 
        if(nrf_gpio_pin_read(USER_BUTTON_PIN) == 0){ 

            if(temp[i] - 1 < 0){ 
                temp[i] = nums[i]; 
                if((i + 1) > 2) 
                    i = 0; 
                else
                    i++;
            } 
            else{
                temp[i]--; 
            } 

            switch (i){ 
                case 0: 
                nrf_gpio_pin_clear(LED_RED_PIN); 
                nrf_delay_ms(500); 
                nrf_gpio_pin_set(LED_RED_PIN); 
                break; 

                case 1: 
                nrf_gpio_pin_clear(LED_GREEN_PIN); 
                nrf_delay_ms(500); 
                nrf_gpio_pin_set(LED_GREEN_PIN); 
                break; 
                
                case 2: 
                nrf_gpio_pin_clear(LED_BLUE_PIN); 
                nrf_delay_ms(500); 
                nrf_gpio_pin_set(LED_BLUE_PIN); 
                break;
            } 
        } 
    } 
} 
/** 

*@} 

**/

