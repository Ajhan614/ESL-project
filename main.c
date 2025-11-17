/** * Copyright (c) 2014 - 2021, Nordic Semiconductor ASA * * All rights reserved. * * Redistribution and use in source and binary forms, with or without modification, * are permitted provided that the following conditions are met: * * 1. Redistributions of source code must retain the above copyright notice, this * list of conditions and the following disclaimer. * * 2. Redistributions in binary form, except as embedded into a Nordic * Semiconductor ASA integrated circuit in a product or a software update for * such product, must reproduce the above copyright notice, this list of * conditions and the following disclaimer in the documentation and/or other * materials provided with the distribution. * * 3. Neither the name of Nordic Semiconductor ASA nor the names of its * contributors may be used to endorse or promote products derived from this * software without specific prior written permission. * * 4. This software, with or without modification, must only be used with a * Nordic Semiconductor ASA integrated circuit. * * 5. Any software provided in binary form under this license must not be reverse * engineered, decompiled, modified and/or disassembled. * * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. * */ /** @file * * @defgroup blinky_example_main main.c * @{ * @ingroup blinky_example * @brief Blinky Example Application main file. * * This file contains the source code for a sample application to blink LEDs. * */ 
#include <stdbool.h> 
#include <stdint.h> 
#include "nrf_delay.h" 
#include "nrfx_gpiote.h" 
#include "nrfx_systick.h" 
#include "nrfx_pwm.h" 
#include "nrf_gpio.h" 

#define LED_RED_PIN NRF_GPIO_PIN_MAP(0,8) 
#define LED_GREEN_PIN NRF_GPIO_PIN_MAP(1,9)  
#define LED_BLUE_PIN NRF_GPIO_PIN_MAP(0,12)  
#define USER_BUTTON_PIN NRF_GPIO_PIN_MAP(1,6) 

#define DOUBLE_CLICK_MS 1000000
#define PWM_TOP_VALUE 1000     
#define PWM_STEP      20        
#define FADE_DELAY_MS 5     
#define DEBOUNCE_US 5000

volatile int current_color = 0;
volatile int current_duty = 0;
volatile int current_cycle = 0;
volatile bool fade_up = true;
volatile bool animating = false;
volatile bool first_click = false;
nrfx_systick_state_t state;
nrfx_systick_state_t debounce_state;

const int nums[3] = {6,4,5};

static nrfx_pwm_t pwm_instance = NRFX_PWM_INSTANCE(0);
static nrf_pwm_values_individual_t pwm_values = {
    .channel_0 = 0,     
    .channel_1 = 0,    
    .channel_2 = 0,     
    .channel_3 = 0      
};
static nrf_pwm_sequence_t pwm_seq =
{
    .values.p_individual = &pwm_values,
    .length = 4,           
    .repeats = 0,
    .end_delay = 0
};
void set_pwm_duty(int color, int duty)
{
    if (duty < 0) duty = 0;
    if (duty > PWM_TOP_VALUE) duty = PWM_TOP_VALUE;

    if (color == 0)
        pwm_values.channel_0 = duty;  
    else if (color == 1)
        pwm_values.channel_1 = duty;  
    else
        pwm_values.channel_2 = duty;  
}
void on_double_click(){
    animating = !animating;
}
void button_event_handler(nrfx_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
    if (!nrfx_systick_test(&debounce_state, DEBOUNCE_US))
    {
        return;
    }
    nrfx_systick_get(&debounce_state);

    if (!first_click)
    {
        nrfx_systick_get(&state);
        first_click = true;
    }
    else
    {
        if(!nrfx_systick_test(&state, DOUBLE_CLICK_MS))
        {
            first_click = false;
            on_double_click();
        }
        else
        {
            nrfx_systick_get(&state);
        }
    }
}
void rgb_init(){ 
    nrfx_gpiote_init();
    nrfx_gpiote_in_config_t btn_config = NRFX_GPIOTE_CONFIG_IN_SENSE_HITOLO(true);
    btn_config.pull = NRF_GPIO_PIN_PULLUP;

    nrfx_gpiote_in_init(USER_BUTTON_PIN, &btn_config, button_event_handler);

    nrfx_gpiote_in_event_enable(USER_BUTTON_PIN, true);

    nrf_gpio_cfg_output(LED_RED_PIN); 
    nrf_gpio_cfg_output(LED_GREEN_PIN); 
    nrf_gpio_cfg_output(LED_BLUE_PIN); 

    nrf_gpio_pin_set(LED_RED_PIN); 
    nrf_gpio_pin_set(LED_GREEN_PIN); 
    nrf_gpio_pin_set(LED_BLUE_PIN); 
    nrfx_pwm_config_t const config =
    {
        .output_pins =
        {
            LED_RED_PIN   | NRFX_PWM_PIN_INVERTED,
            LED_GREEN_PIN | NRFX_PWM_PIN_INVERTED,
            LED_BLUE_PIN  | NRFX_PWM_PIN_INVERTED,
            NRFX_PWM_PIN_NOT_USED,
        },

        .irq_priority = NRFX_PWM_DEFAULT_CONFIG_IRQ_PRIORITY,
        .base_clock   = NRF_PWM_CLK_1MHz,
        .count_mode   = NRF_PWM_MODE_UP,
        .top_value    = PWM_TOP_VALUE,
        .load_mode    = NRF_PWM_LOAD_INDIVIDUAL,
        .step_mode    = NRF_PWM_STEP_AUTO
    };

    nrfx_pwm_init(&pwm_instance, &config, NULL);

    nrfx_pwm_simple_playback(
        &pwm_instance, 
        &pwm_seq,
        1,                       // 1 цикл
        NRFX_PWM_FLAG_LOOP       // бесконечный повтор (анимация)
    );
} 
/** * @brief Function for application main entry. */ 
int main(void) { 
    rgb_init(); 
    nrfx_systick_init();
    nrfx_systick_get(&debounce_state);

    while (true) { 
        if (animating)
        {
            set_pwm_duty(current_color, current_duty);

            if (fade_up)
            {
                current_duty += PWM_STEP;
                if (current_duty > PWM_TOP_VALUE)
                {
                    current_duty = PWM_TOP_VALUE;
                    fade_up = false;
                }
            }
            else
            {
                current_duty -= PWM_STEP;
                if (current_duty < 0)
                {
                    current_duty = 0;
                    fade_up = true;
                    current_cycle++;
                    if (current_cycle >= nums[current_color])
                    {
                        current_cycle = 0;
                        if(current_color + 1 > 2)
                            current_color = 0;
                        else
                            current_color = current_color + 1;
                    }
                }
            }

            nrf_delay_ms(FADE_DELAY_MS);
        }
    } 
} 
/** 

*@} 

**/

