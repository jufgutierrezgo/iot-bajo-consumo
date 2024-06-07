/*
 * Zephyr Timer Work Queue
 *
 * Copyright (c) 2023 Mark Palmeri (Duke University)
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

#define MAIN_SLEEP_TIME_MS   100000
#define BLINK_TIMER_INTERVAL_MS 5000
#define WORK_QUEUE_NAP_TIME_MS 10
#define ONESHOT_DURATION_MS 10000

static const struct gpio_dt_spec led_blink = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
// static const struct gpio_dt_spec led_oneshot = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);
static const struct gpio_dt_spec led_oneshot = GPIO_DT_SPEC_GET(DT_ALIAS(mycusgpio), gpios);

void blink_timer_handler(struct k_timer *blink_timer);
void blink_timer_stop(struct k_timer *blink_timer);
void oneshot_timer_handler(struct k_timer *blink_timer);
void blink_timer_work_handler(struct k_work *timer_work);

K_TIMER_DEFINE(blink_timer, blink_timer_handler, blink_timer_stop);
K_TIMER_DEFINE(oneshot_timer, oneshot_timer_handler, NULL);
K_WORK_DEFINE(blink_timer_work, blink_timer_work_handler);

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
	ret = gpio_pin_configure_dt(&led_oneshot, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		return -1;
	}

    k_timer_start(&blink_timer, K_MSEC(BLINK_TIMER_INTERVAL_MS), K_MSEC(BLINK_TIMER_INTERVAL_MS));
    k_timer_start(&oneshot_timer, K_MSEC(ONESHOT_DURATION_MS), K_NO_WAIT);

	while (1) {
		k_msleep(MAIN_SLEEP_TIME_MS);
	}

	return 0;
}

void blink_timer_handler(struct k_timer *blink_timer){
    k_work_submit(&blink_timer_work);
    LOG_INF("Submitted blinking work to the queue! (%lld)", k_uptime_get());
}

void blink_timer_stop(struct k_timer *blink_timer){
    LOG_INF("Stopping the blinking LED.");
    gpio_pin_set_dt(&led_blink, 0);
}

void blink_timer_work_handler(struct k_work *timer_work) {
    LOG_INF("Doing blink work! (%lld)", k_uptime_get());
    gpio_pin_toggle_dt(&led_blink);
    k_msleep(WORK_QUEUE_NAP_TIME_MS);
    LOG_INF("Took a nap... just woke up. (%lld)", k_uptime_get());
}

void oneshot_timer_handler(struct k_timer *oneshot_timer) {
    LOG_INF("Turn oneshot LED off (%lld)", k_uptime_get());
    gpio_pin_set_dt(&led_oneshot, 0);
}