/**
 * Copyright (c) 2014 - 2021, Nordic Semiconductor ASA
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <stdbool.h> 
#include <stdint.h> 
#include <math.h>
#include "nrf_delay.h" 
#include "nrfx_gpiote.h" 
#include "nrfx_systick.h" 
#include "nrfx_pwm.h" 
#include "nrfx_nvmc.h"
#include "nrf_gpio.h" 

#define LED_RED_PIN NRF_GPIO_PIN_MAP(0,8) 
#define LED_GREEN_PIN NRF_GPIO_PIN_MAP(1,9)  
#define LED_BLUE_PIN NRF_GPIO_PIN_MAP(0,12)  
#define LED1 NRF_GPIO_PIN_MAP(0,6)
#define USER_BUTTON_PIN NRF_GPIO_PIN_MAP(1,6) 

#define DOUBLE_CLICK_MS 200000
#define PWM_TOP_VALUE 1000     
#define PWM_STEP_MODE_HUE 5        
#define PWM_STEP_MODE_SATURATION 25
#define HUE_STEP 0.5
#define SAT_STEP 0.5
#define BRI_STEP 0.5
#define MEMORY_ADDR_HUE  0x6e000
#define MEMORY_ADDR_SAT (MEMORY_ADDR_HUE + 4)
#define MEMORY_ADDR_BRI (MEMORY_ADDR_HUE + 8)

#define FADE_DELAY_MS 5     
#define DEBOUNCE_US 10000

volatile int current_duty = 0;
volatile float hue, saturation, brightness = 0;
volatile int current_mode = 0;
volatile bool fade_up = true;
volatile bool fade_up_sat = true;
volatile bool fade_up_bri = true;
volatile bool first_click = false;
nrfx_systick_state_t state;
nrfx_systick_state_t debounce_state;

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
void hsv_to_rgb(float h, float s, float v, int* r_out, int* g_out, int* b_out){
    float H = fmodf(h, 360.0f);
    float S = fminf(100.0f, fmaxf(0.0f, s)) / 100.0f;
    float V = fminf(100.0f, fmaxf(0.0f, v)) / 100.0f;

    float C = V * S;
    float X = C * (1.0f - fabsf(fmodf(H / 60.0f, 2.0f) - 1.0f));
    float m = V - C;

    float r = 0, g = 0, b = 0;

    if (H < 60.0f)        { r = C; g = X; b = 0; }
    else if (H < 120.0f)  { r = X; g = C; b = 0; }
    else if (H < 180.0f)  { r = 0; g = C; b = X; }
    else if (H < 240.0f)  { r = 0; g = X; b = C; }
    else if (H < 300.0f)  { r = X; g = 0; b = C; }
    else                  { r = C; g = 0; b = X; }

    int R = (int)((r + m) * (float)PWM_TOP_VALUE + 0.5f);
    int G = (int)((g + m) * (float)PWM_TOP_VALUE + 0.5f);
    int B = (int)((b + m) * (float)PWM_TOP_VALUE + 0.5f);

    if (R > PWM_TOP_VALUE) R = PWM_TOP_VALUE;
    if (G > PWM_TOP_VALUE) G = PWM_TOP_VALUE;
    if (B > PWM_TOP_VALUE) B = PWM_TOP_VALUE;

    *r_out = R;
    *g_out = G;
    *b_out = B;
}
void set_pwm_duty(int duty)
{
    int step;
    int delta;
    if (current_mode == 1 || current_mode == 2) {
        step = (current_mode == 1) ? PWM_STEP_MODE_HUE : PWM_STEP_MODE_SATURATION;
        delta = fade_up ? step : -step;
        current_duty += delta;
        if (current_duty > PWM_TOP_VALUE) {
            current_duty = PWM_TOP_VALUE;
            fade_up = false;
        } else if (current_duty < 0) {
            current_duty = 0;
            fade_up = true;
        }
    }
    switch(current_mode){
        case 0:
            pwm_values.channel_3 = 0;
            break;
        case 3:
            pwm_values.channel_3 = PWM_TOP_VALUE;
            break;
        default:
            if (duty < 0) duty = 0;
            if (duty > PWM_TOP_VALUE) duty = PWM_TOP_VALUE;
            pwm_values.channel_3 = duty;  
            break;
    }
}
void apply_pwm_rgb(void)
{
    int r, g, b;
    hsv_to_rgb(hue, saturation, brightness, &r, &g, &b);

    pwm_values.channel_0 = r;
    pwm_values.channel_1 = g;
    pwm_values.channel_2 = b;
}
void save_color(void)
{
    union {
        float f;
        uint32_t u;
    } conv;

    conv.f = hue;
    uint32_t hue_val = conv.u;
    conv.f = saturation;
    uint32_t sat_val = conv.u;
    conv.f = brightness;
    uint32_t bri_val = conv.u;

    bool need_erase = false;

    if (!nrfx_nvmc_word_writable_check(MEMORY_ADDR_HUE, hue_val)) {
        need_erase = true;
    }
    if (!nrfx_nvmc_word_writable_check(MEMORY_ADDR_SAT, sat_val)) {
        need_erase = true;
    }
    if (!nrfx_nvmc_word_writable_check(MEMORY_ADDR_BRI, bri_val)) {
        need_erase = true;
    }

    if (need_erase) {
        nrfx_nvmc_page_erase(MEMORY_ADDR_HUE);
    }

    nrfx_nvmc_word_write(MEMORY_ADDR_HUE, hue_val);
    nrfx_nvmc_word_write(MEMORY_ADDR_SAT, sat_val);
    nrfx_nvmc_word_write(MEMORY_ADDR_BRI, bri_val);

    while (!nrfx_nvmc_write_done_check()) {}  // Ждём завершения
}
void button_event_handler(nrfx_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
    if (!nrfx_systick_test(&debounce_state, DEBOUNCE_US))
    {
        return;
    }
    nrfx_systick_get(&debounce_state);

    if (!first_click) {
        nrfx_systick_get(&state);
        first_click = true;
    } 
    else 
    {
        if (!nrfx_systick_test(&state, DOUBLE_CLICK_MS)) {
            first_click = false;
            if(current_mode + 1 > 3)
                current_mode = 0;
            else
                current_mode++;
            if(current_mode == 1)
                save_color();
        } else {
            nrfx_systick_get(&state);
            first_click = false;
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
    nrf_gpio_cfg_output(LED1);

    nrf_gpio_pin_set(LED_RED_PIN); 
    nrf_gpio_pin_set(LED_GREEN_PIN); 
    nrf_gpio_pin_set(LED_BLUE_PIN); 
    nrf_gpio_pin_set(LED1);

    nrfx_pwm_config_t const config =
    {
        .output_pins =
        {
            LED_RED_PIN   | NRFX_PWM_PIN_INVERTED,
            LED_GREEN_PIN | NRFX_PWM_PIN_INVERTED,
            LED_BLUE_PIN  | NRFX_PWM_PIN_INVERTED,
            LED1          | NRFX_PWM_PIN_INVERTED
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
        1,                      
        NRFX_PWM_FLAG_LOOP       
    );
} 
void change_pwm(){
switch (current_mode)
    {
        case 2:
            if(fade_up_sat)
                saturation += SAT_STEP;
            else
                saturation -= SAT_STEP;
            if (saturation > 100){
                saturation = 100;
                fade_up_sat =false;
            }
            else if(saturation < 0){
                saturation = 0;
                fade_up_sat = true;
            }
            break;

        case 3:
            if(fade_up_bri)
                brightness += BRI_STEP;
            else
                brightness -= BRI_STEP;
            if (brightness > 100){
                brightness = 100;
                fade_up_bri = false;
            }
            else if(brightness < 0){
                brightness = 0;
                fade_up_bri = true;
            }
            break;

        default:
            break;
    }
    apply_pwm_rgb();
}
/** * @brief Function for application main entry. */ 
int main(void) { 
    rgb_init(); 
    nrfx_systick_init();
    nrfx_systick_get(&debounce_state);

    uint32_t stored_hue = *((uint32_t *)MEMORY_ADDR_HUE);
    union {
        float f;
        uint32_t u;
    } conv;
    if (stored_hue == 0xFFFFFFFF) {
        hue = 18.0f;
        saturation = 100.0f;
        brightness = 100.0f;
    } else {
        conv.u = stored_hue;
        hue = conv.f;
        conv.u = *((uint32_t *)MEMORY_ADDR_SAT);
        saturation = conv.f;
        conv.u = *((uint32_t *)MEMORY_ADDR_BRI);
        brightness = conv.f;
    }

    apply_pwm_rgb();

    while (true) { 
        set_pwm_duty(current_duty);
        if(nrf_gpio_pin_read(USER_BUTTON_PIN) == 0)
            {change_pwm();}
        nrf_delay_ms(FADE_DELAY_MS);
    }
} 
/** 

*@} 

**/