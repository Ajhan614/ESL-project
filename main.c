#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <stddef.h>

#include "nrf.h"
#include "nrf_drv_clock.h"
#include "nrf_gpio.h"
#include "nrf_delay.h"
#include "nrfx_pwm.h"

#include "app_timer.h"
#include "app_error.h"
#include "app_util.h"

#include "nrf_cli.h"
#include "nrf_cli_rtt.h"
#include "nrf_cli_types.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#include "nrf_cli_cdc_acm.h"
#include "nrf_drv_usbd.h"
#include "app_usbd_core.h"
#include "app_usbd.h"
#include "app_usbd_string_desc.h"
#include "app_usbd_cdc_acm.h"

#define LED_RED_PIN NRF_GPIO_PIN_MAP(0,8) 
#define LED_GREEN_PIN NRF_GPIO_PIN_MAP(1,9)  
#define LED_BLUE_PIN NRF_GPIO_PIN_MAP(0,12)  
#define LED1 NRF_GPIO_PIN_MAP(0,6)
#define PWM_TOP_VALUE 1000

static void usbd_user_ev_handler(app_usbd_event_type_t event)
{
    switch (event)
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
        pwm_values.channel_0 = (r * PWM_TOP_VALUE) / 255 ;
        pwm_values.channel_1 = (g * PWM_TOP_VALUE) / 255;
        pwm_values.channel_2 = (b * PWM_TOP_VALUE) / 255;

        nrf_cli_fprintf(p_cli, NRF_CLI_NORMAL, "RGB set successfully: R=%d, G=%d, B=%d\n", r,g,b);
    }
    else {
        nrf_cli_error(p_cli,"RGB values out of range\n");
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

    if(h >= 0 && h <= 360 && s >= 0 && s <= 100 && v >= 0 && v <= 100){
        pwm_values.channel_0 = (r_out * PWM_TOP_VALUE) / 255;
        pwm_values.channel_1 = (g_out * PWM_TOP_VALUE) / 255;
        pwm_values.channel_2 = (b_out * PWM_TOP_VALUE) / 255;

        nrf_cli_fprintf(p_cli, NRF_CLI_NORMAL, "HSV set successfully: H=%d, S=%d, V=%d\n", h,s,v);
    }
    else {
        nrf_cli_error(p_cli,"HSV values out of range\n");
    }
}
void process_help(nrf_cli_t const * p_cli, size_t argc, char ** argv){
    nrf_cli_fprintf(p_cli, NRF_CLI_NORMAL, "  Supported commands:\n\n");
    nrf_cli_fprintf(p_cli, NRF_CLI_NORMAL, "  RGB <r> <g> <b>   - the device sets current color to specified one.(0-255)\n");
    nrf_cli_fprintf(p_cli, NRF_CLI_NORMAL, "  HSV <h> <s> <v>   - the same with RGB, but color is specified in HSV.(H:0-360, S:0-100, V:0-100)\n");
    nrf_cli_fprintf(p_cli, NRF_CLI_NORMAL, "  help              - print information about supported commands.\n");
}

NRF_CLI_CMD_REGISTER(RGB, NULL, NULL, process_rgb);
NRF_CLI_CMD_REGISTER(HSV, NULL, NULL, process_hsv);
NRF_CLI_CMD_REGISTER(help, NULL, NULL, process_help);

static void cli_init(void)
{
    nrfx_pwm_config_t const config = {
        .output_pins = {
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
    nrfx_pwm_simple_playback(&pwm_instance, &pwm_seq, 1, NRFX_PWM_FLAG_LOOP);

    ret_code_t ret;

    ret = nrf_cli_init(&m_cli_cdc_acm, NULL, true, true, NRF_LOG_SEVERITY_INFO);
    APP_ERROR_CHECK(ret);

    static const app_usbd_config_t usbd_config = {
        .ev_handler = app_usbd_event_execute,
        .ev_state_proc = usbd_user_ev_handler
    };
    ret = app_usbd_init(&usbd_config);
    APP_ERROR_CHECK(ret);

    app_usbd_class_inst_t const * class_cdc_acm =
            app_usbd_cdc_acm_class_inst_get(&nrf_cli_cdc_acm);
    ret = app_usbd_class_append(class_cdc_acm);
    APP_ERROR_CHECK(ret);

    ret = app_usbd_power_events_enable();
    APP_ERROR_CHECK(ret);
    
    /* Give some time for the host to enumerate and connect to the USB CDC port */
    nrf_delay_ms(1000);

    ret = nrf_cli_start(&m_cli_cdc_acm);
    APP_ERROR_CHECK(ret);
}
static void cli_process(void)
{
    nrf_cli_process(&m_cli_cdc_acm);
}
int main(void)
{
    ret_code_t ret;

    ret = nrf_drv_clock_init();
    APP_ERROR_CHECK(ret);
    nrf_drv_clock_lfclk_request(NULL);

    cli_init();

    while (true)
    {
        cli_process();

        if (NRF_LOG_PROCESS() == false)
        {
            __WFE();
        }
    }
}

/** @} */
