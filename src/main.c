#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

#include "modules/server_module/nrf9160_setup.h"

#define LED0_NODE DT_ALIAS(led0)

#define SLEEP_TIME_MS 2000

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

int main() {
	printk("Welcome to the OreCart Hardware Project!\r\n");
    int ret;

    if (!gpio_is_ready_dt(&led)) {
        return 0;
    }

    ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		return 0;
	}

#if defined(CONFIG_NRF_MODEM_LIB)
	init_nrf9160_modem();
#endif

	while (1) {
		k_sleep(K_SECONDS(1));
		// printk("Beep\r\n");
		gpio_pin_toggle_dt(&led);
	}

	return 0;
}
