/*
 * Copyright (c) 2020 Friedt Professional Engineering Services, Inc
 * Copyright (c) 2020 Jason Kridner
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/led.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/adc.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/socket.h>
#include <zephyr/sys/util.h>
#include <zephyr/random/rand32.h>
#include <zephyr/devicetree.h>
#include <zephyr/devicetree/io-channels.h>
#include <errno.h>
#include <zephyr/linker/sections.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_mgmt.h>

#include <math.h>

#define LOG_LEVEL LOG_LEVEL_INF
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(sensortest);

#define MCAST_IP6ADDR "ff84::2"

#define BLINK_MS 500

#define MAX_STR_LEN 256
static char outstr[MAX_STR_LEN];

enum api {
	BUTTON_API,
	SENSOR_API,
};

enum edev {
	BUTTON,
	LIGHT,
	ACCEL,
	HUMIDITY,
	NUM_DEVICES,
};

struct led_work {
	uint8_t active_led;
	struct k_work_delayable dwork;
};

static void sensor_work_handler(struct k_work *work);

static const char *device_labels[NUM_DEVICES] = {
	[BUTTON] = DT_PROP(DT_ALIAS(sw0), label),
	[LIGHT] = "LIGHT",
	[ACCEL] = "ACCEL",
	[HUMIDITY] = "HUMIDITY",
};

static const char *device_names[NUM_DEVICES] = {
	[BUTTON] = DEVICE_DT_NAME(DT_GPIO_CTLR(DT_ALIAS(sw0), gpios)),
	[LIGHT] = "OPT3001-LIGHT",
	[ACCEL] = "LIS2DE12-ACCEL",
	[HUMIDITY] = "HDC2010-HUMIDITY",
};

static const uint8_t device_pins[NUM_DEVICES] = {
	[BUTTON] = DT_GPIO_PIN(DT_ALIAS(sw0), gpios),
};

static const enum api apis[NUM_DEVICES] = {
	BUTTON_API,
	SENSOR_API, /* LIGHT */
	SENSOR_API, /* ACCEL */
	SENSOR_API, /* HUMIDITY */
};

static struct device *devices[NUM_DEVICES];

static int fd = -1;

/* Set TIMED_SENSOR_READ to 0 to disable */
#define TIMED_SENSOR_READ 60
static int sensor_read_count = TIMED_SENSOR_READ;


static void print_sensor_value(size_t idx, const char *chan,
			       struct sensor_value *val)
{
	LOG_INF("%s: %s%d,%d", device_labels[idx], chan, val->val1, val->val2);

	sprintf(outstr+strlen(outstr), "%d%c:", idx, chan[0]);
	sprintf(outstr+strlen(outstr), "%d", val->val1);
	if (val->val2 != 0) {
		sprintf(outstr+strlen(outstr), ".%02d;", abs(val->val2) / 10000);
	} else {
		sprintf(outstr+strlen(outstr), ";");
	}
}


static void read_sensors(void)
{
	struct sensor_value val;

	outstr[0] = '\0';

	for (size_t i = 0; i < NUM_DEVICES; ++i) {
		if (apis[i] != SENSOR_API) {
			continue;
		}

		if (devices[i] == NULL) {
			continue;
		}

		sensor_sample_fetch(devices[i]);

		if (i == LIGHT) {
			sensor_channel_get(devices[i], SENSOR_CHAN_LIGHT, &val);
			print_sensor_value(i, "l: ", &val);
			
			continue;
		}

		if (i == ACCEL) {
			sensor_channel_get(devices[i], SENSOR_CHAN_ACCEL_X,
					   &val);
			print_sensor_value(i, "x: ", &val);
			sensor_channel_get(devices[i], SENSOR_CHAN_ACCEL_Y,
					   &val);
			print_sensor_value(i, "y: ", &val);
			sensor_channel_get(devices[i], SENSOR_CHAN_ACCEL_Z,
					   &val);
			print_sensor_value(i, "z: ", &val);
			
			continue;
		}

		if (i == HUMIDITY) {
			sensor_channel_get(devices[i], SENSOR_CHAN_HUMIDITY,
					   &val);
			print_sensor_value(i, "h: ", &val);
			sensor_channel_get(devices[i], SENSOR_CHAN_AMBIENT_TEMP,
					   &val);
			print_sensor_value(i, "t: ", &val);
			
			continue;
		}
	}
}


void main(void)
{
	int r;
	
	for (size_t i = 0; i < NUM_DEVICES; ++i) {
		LOG_INF("opening device %s", device_labels[i]);
		devices[i] =
			(struct device *)device_get_binding(device_names[i]);
		if (devices[i] == NULL) {
			LOG_ERR("failed to open device %s", device_labels[i]);
			continue;
		}

		/* per-device setup */
		switch (apis[i]) {
		/*
		case LED_API:
			r = gpio_pin_configure(devices[i], device_pins[i],
					       GPIO_OUTPUT_LOW);
			__ASSERT(r == 0,
				 "gpio_pin_configure(%s, %u, %x) failed: %d",
				 device_labels[i], device_pins[i],
				 GPIO_OUTPUT_LOW, r);
			break;
		*/
		case BUTTON_API:
			r = gpio_pin_configure(
				devices[i], device_pins[i],
				(GPIO_INPUT |
				 DT_GPIO_FLAGS(DT_ALIAS(sw0), gpios)));
			__ASSERT(r == 0,
				 "gpio_pin_configure(%s, %u, %x) failed: %d",
				 device_labels[i], device_pins[i],
				 (GPIO_INPUT |
				  DT_GPIO_FLAGS(DT_ALIAS(sw0), gpios)),
				 r);
			r = gpio_pin_interrupt_configure(
				devices[i], device_pins[i],
				GPIO_INT_EDGE_TO_ACTIVE);
			__ASSERT(
				r == 0,
				"gpio_pin_interrupt_configure(%s, %u, %x) failed: %d",
				device_labels[i], device_pins[i],
				GPIO_INT_EDGE_TO_ACTIVE, r);
			break;

		case SENSOR_API:
			break;

		default:
			break;
		}
	}

	/* setup timer-driven LED event */
	// k_work_init_delayable(&led_work.dwork, led_work_handler);
	//led_work.active_led = LED_SUBG;
	// r = k_work_schedule(&led_work.dwork, K_MSEC(BLINK_MS));
	// __ASSERT(r == 0, "k_work_schedule() failed for LED %u work: %d",
	// 	 LED_SUBG, r);

	/* setup input-driven button event */
	// gpio_init_callback(
	// 	&button_callback, (gpio_callback_handler_t)button_handler,
	// 	BIT(device_pins[BUTTON]));
	// r = gpio_add_callback(devices[BUTTON], &button_callback);
	// __ASSERT(r == 0, "gpio_add_callback() failed: %d", r);

	for (;;) {
		read_sensors();
		k_sleep(K_MSEC(1000));
	}
}
