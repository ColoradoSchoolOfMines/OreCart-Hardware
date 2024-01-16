#include <zephyr/kernel.h>

#include "common/van_info.h"
#include "common/location.h"

#include "modules/net_module/net_module.h"

// #define POSIX_MODE

#ifndef POSIX_MODE
#include <zephyr/drivers/gpio.h>
#define LED0_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
#endif

#define SLEEP_TIME_MS 2000

VanInfo van_info;

int main() {
	OC_LOG_INFO("Welcome to the OreCart Hardware Project!");
    int ret;

	net_module_init();

    if (!gpio_is_ready_dt(&led)) {
        return 0;
    }

    ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		return 0;
	}

	struct VanInfo van_info = {
		.van_id = 1
	};

	struct Location location = {.lat=213.12, .lon=20.123};

	while (1) {
		forward_van_location(van_info, location, 100);
		k_sleep(K_SECONDS(20));
	}

	while (1) {
		k_sleep(K_SECONDS(15));
	}

	return 0;
}
