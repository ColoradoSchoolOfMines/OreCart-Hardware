#include <zephyr/kernel.h>

#include "modules/server_module/nrf9160_setup.h"
#include "modules/server_module/server_module.h"

#include "common/van_info.h"
#include "common/location.h"

#include "modules/server_module/net_tools.h"

#define POSIX_MODE

#ifndef POSIX_MODE
#include <zephyr/drivers/gpio.h>
#define LED0_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
#endif

#define SLEEP_TIME_MS 2000


VanInfo van_info;

int main() {
	printk("Welcome to the OreCart Hardware Project!\r\n");
    int ret;

	server_module_init();

#ifndef POSIX_MODE
    if (!gpio_is_ready_dt(&led)) {
        return 0;
    }

    ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		return 0;
	}
#endif

#if defined(CONFIG_NRF_MODEM_LIB)
	init_nrf9160_modem();
#endif

	struct VanInfo van_info = {
		.van_id = 1
	};

	while (1) {
		// printk("Beep\r\n");
	#ifndef POSIX_MODE
		gpio_pin_toggle_dt(&led);
	#endif
		server_send_van_location(&van_info, (struct Location){.lat = 213.12, .lon=20.123}, 100);
		k_sleep(K_SECONDS(50));
	}

	while (1) {
		k_sleep(K_SECONDS(15));
	}

	return 0;
}
