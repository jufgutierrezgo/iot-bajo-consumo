/*
 * Zephyr Timer Work Queue
 *
 * Copyright (c) 2023 Mark Palmeri (Duke University)
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <inttypes.h>
#include <zephyr/device.h>


// LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

#define MAIN_SLEEP_TIME_MS   1
#define BLINK_TIMER_INTERVAL_MS 500

static const struct gpio_dt_spec led_blink = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);
static struct gpio_callback button_cb_data;

static struct k_timer blink_timer;

void button_pressed(const struct device *dev, struct gpio_callback *cb,
		    uint32_t pins)
{
	static bool state_button = false;

	// gpio_pin_set_dt(&led_blink, 1);
	printk("Button pressed at %" PRIu32 "\n", k_cycle_get_32());

	if(state_button){
		k_timer_start(&blink_timer, K_MSEC(BLINK_TIMER_INTERVAL_MS), K_MSEC(BLINK_TIMER_INTERVAL_MS));    
		state_button = !state_button;
	} else{
		k_timer_stop(&blink_timer);    
		state_button = !state_button;
	}
}

void blink_timer_handler(struct k_timer *blink_timer){
	
	int ret;
	
	// ret = gpio_pin_toggle_dt(&led_blink);
	if (ret < 0) {
		return;
	}
}

void blink_timer_stop(struct k_timer *blink_timer){
	gpio_pin_set_dt(&led_blink, 0);
}

int main(void)
{
	int ret;

	if (!gpio_is_ready_dt(&led_blink)) {
		return -1;
	}

	ret = gpio_pin_configure_dt(&led_blink, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		return -1;
	}	

	
	if (!gpio_is_ready_dt(&button)) {
		printk("Error: button device %s is not ready\n",
		       button.port->name);
		return -1;
	}

	ret = gpio_pin_configure_dt(&button, GPIO_INPUT);
	if (ret != 0) {
		printk("Error %d: failed to configure %s pin %d\n",
		       ret, button.port->name, button.pin);
		return -1;
	}

	ret = gpio_pin_interrupt_configure_dt(&button,
					      GPIO_INT_EDGE_TO_ACTIVE);
	if (ret != 0) {
		printk("Error %d: failed to configure interrupt on %s pin %d\n",
			ret, button.port->name, button.pin);
		return -1;
	}

	
	gpio_init_callback(&button_cb_data, button_pressed, BIT(button.pin));
	gpio_add_callback(button.port, &button_cb_data);

	k_timer_init(&blink_timer, blink_timer_handler, blink_timer_stop);
    
	

	while (1) {
		k_msleep(MAIN_SLEEP_TIME_MS);
	}

	return 0;
}

