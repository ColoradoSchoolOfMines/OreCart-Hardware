#include <zephyr/kernel.h>

#include "modules/server_module/nrf9160_setup.h"
#include "modules/server_module/server_module.h"

#include "common/van_info.h"
#include "common/location.h"

#include "modules/server_module/net_tools.h"

#include "mbedtls/debug.h"

// #define POSIX_MODE

#ifndef POSIX_MODE
#include <zephyr/drivers/gpio.h>
#define LED0_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
#endif

#define SLEEP_TIME_MS 2000


VanInfo van_info;
mbedtls_ssl_config _ssl_conf;

static void my_debug(void *ctx, int level, const char *file, int line, const char *str) {
	const char *p, *basename;
	(void) ctx;

	/* Extract basename from file */
	for(p = basename = file; *p != '\0'; p++) {
		if(*p == '/' || *p == '\\') {
			basename = p + 1;
		}
	}

	printk("MBEDTLS ---  %s:%04d: |%d| %s\r\n", basename, line, level, str);
}

int main() {
	printk("Welcome to the OreCart Hardware Project!\r\n");
    int ret;

	// mbedtls_ssl_config_init(&_ssl_conf);
	// mbedtls_ssl_config_init(&_ssl_conf);
	// mbedtls_ssl_config_defaults(&_ssl_conf,
    //                                        MBEDTLS_SSL_IS_CLIENT,
    //                                        1,
    //                                        MBEDTLS_SSL_PRESET_DEFAULT);

	// mbedtls_ssl_conf_dbg(&_ssl_conf, my_debug, stdout);
	// mbedtls_debug_set_threshold(4);

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
		// server_send_van_location(&van_info, (struct Location){.lat = 213.12, .lon=20.123}, 100);
		k_sleep(K_SECONDS(1));
	}

	while (1) {
		k_sleep(K_SECONDS(15));
	}

	return 0;
}
