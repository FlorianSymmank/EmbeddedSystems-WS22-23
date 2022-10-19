#include <stdio.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>

#define LED0_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

#define DATA	0
#define STORE	1
#define REFRESH 4

#define SLEEP_TIME_MS 1000

static const char *now_str(void)
{
	static char buf[16]; /* ...HH:MM:SS.MMM */
	uint32_t now = k_uptime_get_32();
	unsigned int ms = now % MSEC_PER_SEC;
	unsigned int s;
	unsigned int min;
	unsigned int h;

	now /= MSEC_PER_SEC;
	s = now % 60U;
	now /= 60U;
	min = now % 60U;
	now /= 60U;
	h = now;

	snprintf(buf, sizeof(buf), "%u:%02u:%02u.%03u", h, min, s, ms);
	return buf;
}

void main(void)
{
	const struct device *const dht22 = DEVICE_DT_GET_ONE(aosong_dht);
	const struct device *const dev = led.port;

	if (!device_is_ready(dht22)) {
		printf("Device %s is not ready\n", dht22->name);
		return;
	}

	gpio_pin_configure(dev, DATA, GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure(dev, STORE, GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure(dev, REFRESH, GPIO_OUTPUT_INACTIVE);

	// reset
	gpio_pin_set(dev, REFRESH, 0);
	gpio_pin_set(dev, DATA, 0);
	gpio_pin_set(dev, STORE, 0);

	for (int i = 0; i < 8; i++) {
		// 0
		gpio_pin_set(dev, STORE, 0);
		gpio_pin_set(dev, DATA, 0);
		gpio_pin_set(dev, STORE, 1);
	}
	gpio_pin_set(dev, REFRESH, 1);

	// reset
	gpio_pin_set(dev, REFRESH, 0);
	gpio_pin_set(dev, DATA, 0);
	gpio_pin_set(dev, STORE, 0);

	k_msleep(SLEEP_TIME_MS);
	k_msleep(SLEEP_TIME_MS);

	while (true) {

		// bin			| hex 	| description
		// 0000 0110 	| 0x6 	| 1
		// 0101 1011 	| 0x5b 	| 2
		// 0100 1111	| 0x4f 	| 3
		// 0110 0110 	| 0x66	| 4
		// 0110 1101 	| 0x6d	| 5
		// 0111 1101 	| 0x7d	| 6
		// 0000 0111 	| 0x7	| 7
		// 0111 1111 	| 0x7f	| 8
		// 0110 1111 	| 0x6f	| 9
		// 0111 0110 	| 0x6f	| 9
		// 0000 0000 	| 0x0 	| space
		// 0110 1111 	| 0x76	| H
		// 0011 0001 	| 0x31	| t
		// 0100 0000 	| 0x40	| -
		// 1000 0000	| 0x80	| .

		// reset
		gpio_pin_set(dev, REFRESH, 0);
		gpio_pin_set(dev, STORE, 0);

		gpio_pin_set(dev, DATA, 0);
		gpio_pin_set(dev, STORE, 1);
		gpio_pin_set(dev, STORE, 0);

		gpio_pin_set(dev, DATA, 1);
		gpio_pin_set(dev, STORE, 1);
		gpio_pin_set(dev, STORE, 0);

		gpio_pin_set(dev, DATA, 1);
		gpio_pin_set(dev, STORE, 1);
		gpio_pin_set(dev, STORE, 0);

		gpio_pin_set(dev, DATA, 0);
		gpio_pin_set(dev, STORE, 1);
		gpio_pin_set(dev, STORE, 0);

		gpio_pin_set(dev, DATA, 1);
		gpio_pin_set(dev, STORE, 1);
		gpio_pin_set(dev, STORE, 0);

		gpio_pin_set(dev, DATA, 1);
		gpio_pin_set(dev, STORE, 1);
		gpio_pin_set(dev, STORE, 0);

		gpio_pin_set(dev, DATA, 1);
		gpio_pin_set(dev, STORE, 1);
		gpio_pin_set(dev, STORE, 0);

		gpio_pin_set(dev, DATA, 1);
		gpio_pin_set(dev, STORE, 1);
		gpio_pin_set(dev, STORE, 0);

		gpio_pin_set(dev, REFRESH, 1);

		// reset
		gpio_pin_set(dev, REFRESH, 0);
		gpio_pin_set(dev, STORE, 0);

		int rc = sensor_sample_fetch(dht22);

		if (rc != 0) {
			printf("Sensor fetch failed: %d\n", rc);
			break;
		}

		struct sensor_value temperature;
		struct sensor_value humidity;

		rc = sensor_channel_get(dht22, SENSOR_CHAN_AMBIENT_TEMP, &temperature);
		if (rc == 0) {
			rc = sensor_channel_get(dht22, SENSOR_CHAN_HUMIDITY, &humidity);
		}
		if (rc != 0) {
			printf("get failed: %d\n", rc);
			break;
		}

		printf("[%s]: %.1f Cel ; %.1f %%RH\n", now_str(),
		       sensor_value_to_double(&temperature), sensor_value_to_double(&humidity));
		// k_sleep(K_SECONDS(2));
		k_msleep(SLEEP_TIME_MS);
	}
}
