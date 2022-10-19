#include <stdio.h>
#include <string.h>

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

// bin			| description
// 0b00000110 	| 1
// 0b01011011 	| 2
// 0b01001111	| 3
// 0b01100110 	| 4
// 0b01101101 	| 5
// 0b01111101 	| 6
// 0b00000111 	| 7
// 0b01111111 	| 8
// 0b01101111 	| 9
// 0b00111111 	| 0
// 0b00000000 	| space
// 0b01101111 	| H
// 0b00111001	| C
// 0b00110001 	| t
// 0b01000000 	| -
// 0b10000000	| .

int convert_to_display_bin(char character)
{
	switch (character) {
	case '1':
		return 0b00000110;
	case '2':
		return 0b01011011;
	case '3':
		return 0b01001111;
	case '4':
		return 0b01100110;
	case '5':
		return 0b01101101;
	case '6':
		return 0b01111101;
	case '7':
		return 0b00000111;
	case '8':
		return 0b01111111;
	case '9':
		return 0b01101111;
	case '0':
		return 0b00111111;
	case ' ':
		return 0b00000000;
	case 'H':
		return 0b01101111;
	case 'C':
		return 0b00111001;
	case 't':
		return 0b00110001;
	case '-':
		return 0b01000000;
	case '.':
		return 0b10000000;
	default:
		return 0b00111111;
	}
}

void clear_display(const struct device *dev)
{
	print_text_to_display(dev, "      ");
}

void print_text_to_display(const struct device *dev, char source[])
{
	gpio_pin_set(dev, REFRESH, 0);
	for (int i = 0; i < strlen(source); i++) {
		print_char_to_display(dev, convert_to_display_bin(source[i]));
	}
	gpio_pin_set(dev, REFRESH, 1);
}

void print_char_to_display(const struct device *dev, int char_code)
{
	int mask = 0b00000001;
	for (int i = 7; i >= 0; i--) {
		int val = (char_code >> i) & mask;
		gpio_pin_set(dev, STORE, 0);
		gpio_pin_set(dev, DATA, val);
		gpio_pin_set(dev, STORE, 1);
	}
}

void start_up_lighting(const struct device *dev)
{
	clear_display(dev);
	k_msleep(SLEEP_TIME_MS);
	print_text_to_display(dev, "-8-8-8-8-");
	k_msleep(SLEEP_TIME_MS);
	print_text_to_display(dev, "8-8-8-8-8");
	k_msleep(SLEEP_TIME_MS);
	print_text_to_display(dev, "-8-8-8-8-");
	k_msleep(SLEEP_TIME_MS);
	print_text_to_display(dev, "8-8-8-8-8");
	k_msleep(SLEEP_TIME_MS);
	clear_display(dev);
	k_msleep(SLEEP_TIME_MS);
}

void main(void)
{
	bool temp_mode = true;

	const struct device *const dht22 = DEVICE_DT_GET_ONE(aosong_dht);
	const struct device *const dev = led.port;

	if (!device_is_ready(dht22)) {
		printf("Device %s is not ready\n", dht22->name);
		return;
	}

	gpio_pin_configure(dev, DATA, GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure(dev, STORE, GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure(dev, REFRESH, GPIO_OUTPUT_INACTIVE);

	start_up_lighting(dev);

	while (true) {
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

		char text[7];
		if (temp_mode) {
			sprintf(text, "%.1f C", sensor_value_to_double(&temperature));
		} else {
			sprintf(text, "%.1f H", sensor_value_to_double(&humidity));
		}

		clear_display(dev); // push every bit out
		print_text_to_display(dev, text);

		printf("[%s]: %.1f Cel ; %.1f %%RH\n", now_str(),
		       sensor_value_to_double(&temperature), sensor_value_to_double(&humidity));

		k_msleep(SLEEP_TIME_MS);
	}
}
