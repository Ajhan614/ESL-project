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

#include "nrfx_pwm.h" 
#include "nrfx_nvmc.h"
#include "nrf_gpio.h" 

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrf_log_backend_usb.h"

#include "app_usbd.h"
#include "app_usbd_cdc_acm.h"

#include "nrf_cli.h"
#include "nrf_cli_cdc_acm.h"

#define LED_RED_PIN NRF_GPIO_PIN_MAP(0,8) 
#define LED_GREEN_PIN NRF_GPIO_PIN_MAP(1,9)  
#define LED_BLUE_PIN NRF_GPIO_PIN_MAP(0,12)  
#define LED1 NRF_GPIO_PIN_MAP(0,6)
//#define USER_BUTTON_PIN NRF_GPIO_PIN_MAP(1,6) 

#define PWM_TOP_VALUE 1000

NRF_CLI_CDC_ACM_DEF(m_cli_cdc_acm_transport);
NRF_CLI_DEF(m_cli_cdc_acm,
            "usb_cli:~$ ",
            &m_cli_cdc_acm_transport.transport,
            '\r',
            4);

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
void hsv_to_rgb(float h, float s, float v, uint32_t* r_out, uint32_t* g_out, uint32_t* b_out){
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
void process_rgb(nrf_cli_t const * p_cli, size_t argc, char ** argv){
    if(argc != 4){
        nrf_cli_error(p_cli, "Too few arguments : %d", argc);
        return;
    }

    uint32_t r = atoi(argv[1]);
    uint32_t g = atoi(argv[2]);
    uint32_t b = atoi(argv[3]);

    if(r >= 0 && r <= 255 && g >= 0 && g <= 255 && b >= 0 && b <= 255){
        pwm_values.channel_0 = r; 
        pwm_values.channel_1 = g; 
        pwm_values.channel_2 = b;

        nrf_cli_fprintf(p_cli, NRF_CLI_NORMAL, "RGB set successfully: R=%d, G=%d, B=%d\r\n", r,g,b);
    }
    else {
        nrf_cli_error(p_cli,"RGB values out of range\r\n");
    }
}
void process_hsv(nrf_cli_t const * p_cli, size_t argc, char ** argv){
    if(argc != 4){
        nrf_cli_error(p_cli, "Too few arguments : %d", argc);
        return;
    }
    uint32_t h = atoi(argv[1]);
    uint32_t s = atoi(argv[2]);
    uint32_t v = atoi(argv[3]);
    uint32_t r_out, g_out, b_out;
    hsv_to_rgb(h,s,v,&r_out, &g_out, &b_out);

    if(r_out >= 0 && r_out <= 255 && g_out >= 0 && g_out <= 255 && b_out >= 0 && b_out <= 255){
        pwm_values.channel_0 = r_out; 
        pwm_values.channel_1 = g_out; 
        pwm_values.channel_2 = b_out;

        nrf_cli_fprintf(p_cli, NRF_CLI_NORMAL, "HSV set successfully: H=%d, S=%d, V=%d\r\n", h,s,v);
    }
    else {
        nrf_cli_error(p_cli,"HSV values out of range\r\n");
    }
}
void process_help(nrf_cli_t const * p_cli, size_t argc, char ** argv){
    nrf_cli_fprintf(p_cli, NRF_CLI_NORMAL, "Supported commands:\r\n");
    nrf_cli_fprintf(p_cli, NRF_CLI_NORMAL, "  RGB <r> <g> <b>   - Set color using RGB values (0-255)\n");
    nrf_cli_fprintf(p_cli, NRF_CLI_NORMAL, "  HSV <h> <s> <v>   - Set color using HSV values (H:0-360, S:0-100, V:0-100)\n");
    nrf_cli_fprintf(p_cli, NRF_CLI_NORMAL, "  help              - Print information about supported commands\n");
}

NRF_CLI_CMD_REGISTER(RGB, NULL, NULL, process_rgb);
NRF_CLI_CMD_REGISTER(HSV, NULL, NULL, process_hsv);
NRF_CLI_CMD_REGISTER(help, NULL, NULL, process_help);

static void usb_ev_handler(app_usbd_internal_evt_t const * const p_event)
{
    switch (p_event->type)
    {
        case APP_USBD_EVT_STOPPED:
            app_usbd_disable();
            break;
        case APP_USBD_EVT_POWER_DETECTED:
            if (!nrf_drv_usbd_is_enabled())
            {
                app_usbd_enable();
            }
            break;
        case APP_USBD_EVT_POWER_REMOVED:
            app_usbd_stop();
            break;
        case APP_USBD_EVT_POWER_READY:
            app_usbd_start();
            break;
        default:
            break;
    }
}
void logs_init(){ 
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

    ret_code_t ret = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(ret);

    NRF_LOG_DEFAULT_BACKENDS_INIT();

    NRF_LOG_INFO("Starting up the project with USB logging");

    ret = nrf_cli_init(&m_cli_cdc_acm, NULL, true, true, NRF_LOG_SEVERITY_INFO);
    APP_ERROR_CHECK(ret);
    ret = nrf_cli_start(&m_cli_cdc_acm);
    APP_ERROR_CHECK(ret);

    static const app_usbd_config_t usbd_config = {
        .ev_handler = usb_ev_handler,   // исправлено
    };
    ret = app_usbd_init(&usbd_config);
    APP_ERROR_CHECK(ret);

    app_usbd_class_inst_t const * class_cdc_acm = app_usbd_cdc_acm_class_inst_get(&nrf_cli_cdc_acm);
    ret = app_usbd_class_append(class_cdc_acm);
    APP_ERROR_CHECK(ret);

    ret = app_usbd_power_events_enable();
    APP_ERROR_CHECK(ret);
} 

void my_cli_process(){
    nrf_cli_process(&m_cli_cdc_acm);
}
/** * @brief Function for application main entry. */ 
int main(void) { 
    logs_init(); 

    while (true) {
        nrf_cli_process(&m_cli_cdc_acm);
    }
} 
/** 

*@} 

**/