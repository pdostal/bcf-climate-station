#include <application.h>
#include <bcl.h>
#include <bc_usb_cdc.h>

bc_led_strip_t led_strip;
bc_tick_t last_public = 0;
char tx_buffer[1024];
float illuminance = 5000;

static void get_heat_map_color(float value, float *red, float *green, float *blue);
static void usb_publish(const char *topic, const char *key, float *value, const char *unit);

void application_init(void)
{
    bc_rtc_init();

    bc_usb_cdc_init();

    bc_led_strip_init(&led_strip, bc_module_power_get_led_strip_driver(), &bc_module_power_led_strip_buffer_rgbw_144);

    bc_module_climate_init();
    bc_module_climate_set_update_interval_thermometer(50);
    bc_module_climate_set_update_interval_hygrometer(1000);
    bc_module_climate_set_update_interval_lux_meter(1000);
    bc_module_climate_set_update_interval_barometer(1000);
    bc_module_climate_set_event_handler(climate_event_event_handler, NULL);

}

void climate_event_event_handler(bc_module_climate_event_t event, void *event_param)
{
    (void) event_param;
    float value;

    switch (event)
    {
        case BC_MODULE_CLIMATE_EVENT_UPDATE_THERMOMETER:
            if (bc_module_climate_get_temperature_celsius(&value))
            {
                bc_tick_t now = bc_tick_get();
                if (last_public + 1000 < now)
                {
                    usb_publish("meteostanice/thermometer/i2c0-48", "temperature", &value, "\\u2103");
                    last_public = now;
                }

                value -= -20;

                int max_i = 142 * value / (50+20);

                if (max_i > 142)
                {
                    max_i = 142;
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
                    get_heat_map_color((float)i / 142, &red, &green, &blue);

                    bc_led_strip_set_pixel_rgbw(&led_strip, i, brightness * red, brightness * green, brightness * blue, 0);
                }
                for (int i = max_i; i < 142; i++)
                {
                    bc_led_strip_set_pixel_rgbw(&led_strip, i, 0, 0, 0, white);
                }
                bc_led_strip_write(&led_strip);

            }
            break;

        case BC_MODULE_CLIMATE_EVENT_UPDATE_HYGROMETER:
            if (bc_module_climate_get_humidity_percentage(&value))
            {
                usb_publish("meteostanice/humidity-sensor/i2c0-40", "relative-humidity", &value, "%");
            }
            break;

        case BC_MODULE_CLIMATE_EVENT_UPDATE_LUX_METER:
            if (bc_module_climate_get_luminosity_lux(&value))
            {
                usb_publish("meteostanice/lux-meter/i2c0-44", "illuminance", &value, "lux");
                illuminance = value;
            }
            break;

        case BC_MODULE_CLIMATE_EVENT_UPDATE_BAROMETER:
            if (bc_module_climate_get_pressure_pascal(&value))
            {
                usb_publish("meteostanice/barometer/i2c0-60", "pressure", &value, "Pa");
            }
            break;

        default:
            break;
    }
}

static void usb_publish(const char *topic, const char *key, float *value, const char *unit)
{
    snprintf(tx_buffer, sizeof(tx_buffer),
                         "[\"%s\", {\"%s\": [%0.2f, \"%s\"]}]\n", topic, key, *value, unit);

        bc_usb_cdc_write((const char *) tx_buffer, strlen(tx_buffer));
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
