#include <application.h>
#include <bcl.h>

#ifndef PIXEL_COUNT
#define PIXEL_COUNT 72
#endif
#define PREFIX_TALK "climate-station"

static uint32_t _dma_buffer[PIXEL_COUNT * 4 * 2];

static bc_led_strip_buffer_t led_strip_buffer =
{
    .type = BC_LED_STRIP_TYPE_RGBW,
    .count = PIXEL_COUNT,
    .buffer = _dma_buffer
};

static bc_led_strip_t led_strip;
static bc_tick_t last_public_0_48 = 0;
static bc_tick_t last_public_0_49 = 0;
static float illuminance = 5000;
static bool is_module_climate = false;
static bc_led_t led;
static bool led_state = false;

static void temperature_tag_event_handler(bc_tag_temperature_t *self, bc_tag_temperature_event_t event, void *event_param);
static void thermometer(float temperature);
static void get_heat_map_color(float value, float *red, float *green, float *blue);
static void relay_state_set(usb_talk_payload_t *payload);
static void relay_state_get(usb_talk_payload_t *payload);
static void led_state_set(usb_talk_payload_t *payload);
static void led_state_get(usb_talk_payload_t *payload);

void application_init(void)
{
    bc_led_init(&led, BC_GPIO_LED, false, false);
    bc_led_set_mode(&led, BC_LED_MODE_OFF);

    bc_module_power_init();

    bc_led_strip_init(&led_strip, bc_module_power_get_led_strip_driver(), &led_strip_buffer);

    //temperature on core
    static bc_tag_temperature_t temperature_tag_0_49;
    bc_tag_temperature_init(&temperature_tag_0_49, BC_I2C_I2C0, BC_TAG_TEMPERATURE_I2C_ADDRESS_ALTERNATE);
    bc_tag_temperature_set_update_interval(&temperature_tag_0_49, 50);
    bc_tag_temperature_set_event_handler(&temperature_tag_0_49, temperature_tag_event_handler, NULL);

    bc_module_climate_init();
    bc_module_climate_set_update_interval_thermometer(50);
    bc_module_climate_set_update_interval_hygrometer(1000);
    bc_module_climate_set_update_interval_lux_meter(1000);
    bc_module_climate_set_update_interval_barometer(1000);
    bc_module_climate_set_event_handler(climate_event_event_handler, NULL);

    usb_talk_init();

    usb_talk_sub(PREFIX_TALK "/relay/-/state/set", relay_state_set);
    usb_talk_sub(PREFIX_TALK "/relay/-/state/get", relay_state_get);
    usb_talk_sub(PREFIX_TALK "/led/-/state/set", led_state_set);
    usb_talk_sub(PREFIX_TALK "/led/-/state/get", led_state_get);
}

void climate_event_event_handler(bc_module_climate_event_t event, void *event_param)
{
    (void) event_param;
    float value;
    float altitude;

    static uint8_t i2c_thermometer = 0x48;
    static uint8_t i2c_humidity = 0x40;
    static uint8_t i2c_lux_meter = 0x44;
    static uint8_t i2c_barometer = 0x60;

    switch (event)
    {
        case BC_MODULE_CLIMATE_EVENT_UPDATE_THERMOMETER:
            if (bc_module_climate_get_temperature_celsius(&value))
            {
                is_module_climate = true;

                thermometer(value);

                bc_tick_t now = bc_tick_get();
                if (last_public_0_48 + 1000 < now)
                {
                    usb_talk_publish_thermometer(PREFIX_TALK, &i2c_thermometer, &value);
                    last_public_0_48 = now;
                }
            }
            break;

        case BC_MODULE_CLIMATE_EVENT_ERROR_THERMOMETER:
            is_module_climate = false;
            break;

        case BC_MODULE_CLIMATE_EVENT_UPDATE_HYGROMETER:
            if (bc_module_climate_get_humidity_percentage(&value))
            {
                usb_talk_publish_humidity_sensor(PREFIX_TALK, &i2c_humidity, &value);
            }
            break;

        case BC_MODULE_CLIMATE_EVENT_UPDATE_LUX_METER:
            if (bc_module_climate_get_luminosity_lux(&value))
            {
                usb_talk_publish_lux_meter(PREFIX_TALK, &i2c_lux_meter, &value);
                illuminance = value;
            }
            break;

        case BC_MODULE_CLIMATE_EVENT_UPDATE_BAROMETER:


            if (!bc_module_climate_get_pressure_pascal(&value))
            {
                return;
            }

            if (!bc_module_climate_get_altitude_meter(&altitude))
            {
                return;
            }

            usb_talk_publish_barometer(PREFIX_TALK, &i2c_barometer, &value, &altitude);
            break;
        case BC_MODULE_CLIMATE_EVENT_ERROR_HYGROMETER:
        case BC_MODULE_CLIMATE_EVENT_ERROR_LUX_METER:
        case BC_MODULE_CLIMATE_EVENT_ERROR_BAROMETER:
        default:
            break;
    }
}

static void temperature_tag_event_handler(bc_tag_temperature_t *self, bc_tag_temperature_event_t event, void *event_param)
{
    (void) event_param;
    float value;
    static uint8_t i2c_thermometer = 0x49;

    if (event != BC_TAG_TEMPERATURE_EVENT_UPDATE)
    {
        return;
    }

    if (bc_tag_temperature_get_temperature_celsius(self, &value))
    {
        if(!is_module_climate)
        {
            thermometer(value);
        }

        bc_tick_t now = bc_tick_get();
        if (last_public_0_49 + 1000 < now)
        {
            usb_talk_publish_thermometer(PREFIX_TALK, &i2c_thermometer, &value);
            last_public_0_49 = now;
        }
    }
}

static void thermometer(float temperature)
{
    temperature -= -20;

    int max_i = PIXEL_COUNT * temperature / (50 + 20);

    if (max_i > PIXEL_COUNT)
    {
        max_i = PIXEL_COUNT;
    }

    if (max_i < 0)
    {
        max_i = 0;
    }

    float red;
    float green;
    float blue;
    uint8_t brightness = 255;
    uint8_t white = illuminance < 50 ? 1 : 0;

    if (illuminance < 1000)
    {
        brightness = (illuminance / 992.f) * 255 + 8;
    }

    for (int i = 0; i < max_i; i++)
    {
        get_heat_map_color((float)i / PIXEL_COUNT, &red, &green, &blue);

        bc_led_strip_set_pixel_rgbw(&led_strip, i, brightness * red, brightness * green, brightness * blue, 0);
    }
    for (int i = max_i; i < PIXEL_COUNT; i++)
    {
        bc_led_strip_set_pixel_rgbw(&led_strip, i, 0, 0, 0, white);
    }

    bc_led_strip_write(&led_strip);
}

static void get_heat_map_color(float value, float *red, float *green, float *blue)
{
    const int NUM_COLORS = 4;
    const float color[4][3] = { {0,0,1}, {0,1,0}, {1,1,0}, {1,0,0} };

    int idx1;        // Our desired color will be between these two indexes in "color".
    int idx2;
    float fractBetween = 0; // Fraction between "idx1" and "idx2" where our value is.

    if(value <= 0)
    {
        idx1 = idx2 = 0;
    }
    else if(value >= 1)
    {
        idx1 = idx2 = NUM_COLORS - 1;
    }
    else
    {
        value = value * (NUM_COLORS - 1);        // Will multiply value by 3.
        idx1  = floor(value);                  // Our desired color will be after this index.
        idx2  = idx1 + 1;                      // ... and before this index (inclusive).
        fractBetween = value - (float)idx1;    // Distance between the two indexes (0-1).
    }

    *red   = (color[idx2][0] - color[idx1][0]) * fractBetween + color[idx1][0];
    *green = (color[idx2][1] - color[idx1][1]) * fractBetween + color[idx1][1];
    *blue  = (color[idx2][2] - color[idx1][2]) * fractBetween + color[idx1][2];
}

static void relay_state_set(usb_talk_payload_t *payload)
{
    bool state;

    if (!usb_talk_payload_get_bool(payload, &state))
    {
        return;
    }

    bc_module_power_relay_set_state(state);

    usb_talk_publish_relay(PREFIX_TALK, &state);
}

static void relay_state_get(usb_talk_payload_t *payload)
{
    (void) payload;

    bool state = bc_module_power_relay_get_state();

    usb_talk_publish_relay(PREFIX_TALK, &state);
}

static void led_state_set(usb_talk_payload_t *payload)
{
    if (!usb_talk_payload_get_bool(payload, &led_state))
    {
        return;
    }

    bc_led_set_mode(&led, led_state ? BC_LED_MODE_ON : BC_LED_MODE_OFF);

    usb_talk_publish_led(PREFIX_TALK, &led_state);
}

static void led_state_get(usb_talk_payload_t *payload)
{
    (void) payload;

    usb_talk_publish_led(PREFIX_TALK, &led_state);
}
