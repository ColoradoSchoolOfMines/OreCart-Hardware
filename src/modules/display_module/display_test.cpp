#include "display_module.h"

#include <zephyr/kernel.h>
#include <zephyr/drivers/display.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>

#include <zephyr/fs/fs.h>

#include <zephyr/display/cfb.h>

#include "../../common/tools.h"

#define LEFT_BTN_NODE	DT_ALIAS(leftbutton)
#define RIGHT_BTN_NODE  DT_ALIAS(rightbutton)
#define SELECT_BTN_NODE  DT_ALIAS(selectbutton)

static const struct gpio_dt_spec left_btn = GPIO_DT_SPEC_GET_OR(LEFT_BTN_NODE, gpios, {0});
static const struct gpio_dt_spec right_btn = GPIO_DT_SPEC_GET_OR(RIGHT_BTN_NODE, gpios, {0});
static const struct gpio_dt_spec select_btn = GPIO_DT_SPEC_GET_OR(SELECT_BTN_NODE, gpios, {0});

#define BL_PIN 15
#define RST_PIN 16
#define DC_PIN 17

const device* display;

// AT some point, we shouldn't hard-code the routes and should instead fetch those from the server.
// LOL, this is terrible.
const char* ROUTES[NUM_ROUTES] = {
    "  GOLD ROUTE  ",
    " SILVER ROUTE ",
    "TUNGSTEN ROUTE",
    "  IRON ROUTE  "
};

static struct gpio_callback left_button_cb_data;
static struct gpio_callback right_button_cb_data;
static struct gpio_callback select_button_cb_data;

// This is terrible, need to fix ASAP
enum ButtonMode {
    CHANGE_ROUTE,
    CHANGE_RIDERSHIP_COUNT
} button_mode;

uint8_t currently_selected_route = 0;
uint8_t current_ridership = 0;

static int _config_btn(const gpio_dt_spec* btn);

void button_left_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
    switch (button_mode) {
        case CHANGE_ROUTE:
            currently_selected_route = (currently_selected_route - 1) % NUM_ROUTES;
            draw_route_selection_screen();
            break;
        case CHANGE_RIDERSHIP_COUNT:
            if (current_ridership > 0) current_ridership--;
            draw_ridership_screen();
            break;
        default:
            break; // Do nothing
    }
}

void button_right_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
    switch (button_mode) {
        case CHANGE_ROUTE:
            currently_selected_route = (currently_selected_route + 1) % NUM_ROUTES;
            draw_route_selection_screen();
            break;
        case CHANGE_RIDERSHIP_COUNT:
            if (current_ridership < 255) current_ridership++;
            draw_ridership_screen();
            break;
        default:
            break; // Do nothing
    }
}

void button_select_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
    if (button_mode == CHANGE_ROUTE) {
        draw_ridership_screen();
    }
}

void display_test() {
    _config_btn(&left_btn);
    gpio_init_callback(&left_button_cb_data, button_left_pressed, BIT(left_btn.pin));
	gpio_add_callback(left_btn.port, &left_button_cb_data);

    _config_btn(&right_btn);
    gpio_init_callback(&right_button_cb_data, button_right_pressed, BIT(right_btn.pin));
	gpio_add_callback(right_btn.port, &right_button_cb_data);

    _config_btn(&select_btn);
    gpio_init_callback(&select_button_cb_data, button_select_pressed, BIT(select_btn.pin));
	gpio_add_callback(select_btn.port, &select_button_cb_data);
    
    display = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
    display_capabilities capabilities;

	if (!device_is_ready(display)) {
		printk("Device %s not found. Aborting.", display->name);
        return;
    }

    display_get_capabilities(display, &capabilities);

	if (display_set_pixel_format(display, PIXEL_FORMAT_MONO10) != 0) {
		if (display_set_pixel_format(display, PIXEL_FORMAT_MONO01) != 0) {
			printk("Failed to set required pixel format");
			return;
		}
	}

	if (display_blanking_off(display) != 0) {
		printk("Failed to turn off display blanking\n");
		return;
	}

	int err = cfb_framebuffer_init(display);
	if (err) {
		printk("Could not initialize framebuffer (err %d)\n", err);
	}
}

void draw_splash() {
    int err = cfb_framebuffer_clear(display, true);
	if (err) {
		printk("Could not clear framebuffer (err %d)\n", err);
	}

	err = cfb_print(display, WELCOME_MSG, 0, 0);
	if (err) {
		printk("Could not display font (err %d)\n", err);
	}

	err = cfb_framebuffer_finalize(display);
	if (err) {
		printk("Could not finalize framebuffer (err %d)\n", err);
	}
}

void draw_route_selection_screen() {
    int err = cfb_framebuffer_clear(display, true);
	if (err) {
		printk("Could not clear framebuffer (err %d)\n", err);
	}

	err = cfb_print(display, ROUTE_SELECTION_MSG, 15, 0);
    err &= cfb_print(display, string_format("< %s >", ROUTES[currently_selected_route]).c_str(), 5, 10);

	if (err) {
		printk("Could not display font (err %d)\n", err);
	}

	err = cfb_framebuffer_finalize(display);
	if (err) {
		printk("Could not finalize framebuffer (err %d)\n", err);
	}
}

void draw_ridership_screen() {
    int err = cfb_framebuffer_clear(display, true);
	if (err) {
		printk("Could not clear framebuffer (err %d)\n", err);
	}

	err = cfb_print(display, RIDERSHIP_MSG, 0, 0);
    err &= cfb_print(display, string_format("< %i >", current_ridership).c_str(), 5, 10);

	if (err) {
		printk("Could not display font (err %d)\n", err);
	}

	err = cfb_framebuffer_finalize(display);
	if (err) {
		printk("Could not finalize framebuffer (err %d)\n", err);
	}
}

static int _config_btn(const gpio_dt_spec* btn) {
    int ret = gpio_pin_configure_dt(btn, GPIO_INPUT);
    if (ret) return ret;
	ret &= gpio_pin_interrupt_configure_dt(btn, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret) return ret;
    return 0;
}