#include "display_module.h"

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/display.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/spi.h>

// uint8_t my_buffer[4] = {0};
// struct spi_buf my_spi_buffer[1];
// my_spi_buffer[0].buf = my_buffer;
// my_spi_buffer[0].len = 4;
// const struct spi_buf_set rx_buff = { my_spi_buffer, 1 };

// const struct device *gpio = DEVICE_DT_GET(DT_NODELABEL(gpio0));
// const struct spi_dt_spec display_waveshare = SPI_DT_SPEC_GET(DT_NODELABEL(waveshare), SPI_OP, 0);

#define BL_PIN 15
#define RST_PIN 16
#define DC_PIN 17

void display_test() {
    const device* display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
    display_capabilities capabilities;

	if (!device_is_ready(display_dev)) {
		printk("Device %s not found. Aborting.", display_dev->name);
        return;
    }

    display_get_capabilities(display_dev, &capabilities);

    switch (capabilities.current_pixel_format) {
        case PIXEL_FORMAT_RGB_888:
            printk("PIXEL_FORMAT_RGB_888\r\n");
            break;
        case PIXEL_FORMAT_MONO01:
            printk("PIXEL_FORMAT_MONO01\r\n");
    }
}