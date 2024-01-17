#include "display_module.h"

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/spi.h>

uint8_t my_buffer[4] = {0};
struct spi_buf my_spi_buffer[1];
my_spi_buffer[0].buf = my_buffer;
my_spi_buffer[0].len = 4;
const struct spi_buf_set rx_buff = { my_spi_buffer, 1 };

const struct device *gpio = DEVICE_DT_GET(DT_NODELABEL(gpio0));
const struct spi_dt_spec display_waveshare = SPI_DT_SPEC_GET(DT_NODELABEL(waveshare), SPI_OP, 0);

#define BL_PIN 15
#define RST_PIN 16
#define DC_PIN 17

void display_test() {
    /* Gpio pin */
    if (!device_is_ready(gpio))
        printk("Failed to get the gpio0 device");

    gpio_pin_configure(gpio, BL_PIN, GPIO_OUTPUT_HIGH);
    gpio_pin_configure(gpio, RST_PIN, GPIO_OUTPUT_LOW);
    gpio_pin_configure(gpio, DC_PIN, GPIO_OUTPUT_LOW);

    // while (true) {
    //     gpio_pin_toggle(gpio, BL_PIN);
    //     k_sleep(K_SECONDS(2));
    // }


    spi_write_dt(&display_waveshare);
}