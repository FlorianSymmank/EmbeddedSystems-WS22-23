#include <stdio.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>

// Define Pins
#define DATA	0
#define STORE	1
#define REFRESH 4

#define SLEEP_TIME_MS 500

#define LED0_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

#define SW0_NODE DT_ALIAS(sw0)
#if !DT_NODE_HAS_STATUS(SW0_NODE, okay)
#error "Unsupported board: sw0 devicetree alias is not defined"
#endif

static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET_OR(SW0_NODE, gpios, {0});
static struct gpio_callback button_cb_data;

enum Information_Displayed {
	TEMP = 0,
	HUMIDITY = 1,
	UPTIME = 2,
};

static enum Information_Displayed info = TEMP;

// delcare functions
int convert_to_display_bin(char character);
void clear_display(const struct device *dev);
void print_text_to_display(const struct device *dev, char source[]);
void print_char_to_display(const struct device *dev, int char_code);
void start_up_lighting(const struct device *dev);
void button_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins);
bool configure_gpio_pins(const struct device *dev);
bool configure_button();

/// @brief Get system uptime
/// @param
/// @return Formatted String 0:00:00.000 (Hours:Minutes:Seconds.Milliseconds)
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

/// @brief Get system uptime
/// @param
/// @return Formatted String 0:00:00.000 (Hours Minutes)
static const char *uptime_str(void)
{
	static char buf[10]; /* HH MM */
	uint32_t now = k_uptime_get_32();
	unsigned int min;
	unsigned int h;

	now /= MSEC_PER_SEC;
	now /= 60U;
	min = now % 60U;
	now /= 60U;
	h = now;

	snprintf(buf, sizeof(buf), "%02u %02u", h, min);
	return buf;
}

/// @brief Cycles through output modes
/// TEMP -> HUMIDITY -> UPTIME -> TEMP -> ...
/// @param dev Runtime Device Stucture
/// @param cb Callback Struct
/// @param pins Triggered Pin
void button_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	info = (info + 1) % 3;
}
/// @brief Converts a char into a binary sequence
/// @param char char to be converted
/// @return binary sequence
int convert_to_display_bin(char character)
{

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
	// 0b10110110	| ||. as default

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
		return 0b1110110;
	case 'C':
		return 0b00111001;
	case 't':
		return 0b00110001;
	case '-':
		return 0b01000000;
	case '.':
		return 0b10000000;
	default:
		return 0b10110110;
	}
}

/// @brief Clears the display
/// @param dev Runtime Device Stucture
void clear_display(const struct device *dev)
{
	print_text_to_display(dev, "        ");
}

/// @brief Outputs text on the connected display
/// @param dev Runtime Device Stucture
/// @param source Text to be displayed
void print_text_to_display(const struct device *dev, char source[])
{
	gpio_pin_set(dev, REFRESH, 0);
	int len = strlen(source);
	for (int i = 0; i < len; i++) {

		int bin = convert_to_display_bin(source[i]);
		// if next is a decimal point, combine with current character and skip dp to utilise
		// 8th segment
		if (i + 1 < len && source[i + 1] == '.') {
			bin |= convert_to_display_bin('.');
			i++;
		}

		print_char_to_display(dev, bin);
	}
	gpio_pin_set(dev, REFRESH, 1);
}

/// @brief sends binary sequence to Data pin
/// @param dev Runtime Device Stucture
/// @param char_code binary sequence to send
void print_char_to_display(const struct device *dev, int char_code)
{
	// send one bit at a time
	int mask = 0b00000001;
	for (int i = 7; i >= 0; i--) {
		int val = (char_code >> i) & mask;
		gpio_pin_set(dev, STORE, 0);
		gpio_pin_set(dev, DATA, val);
		gpio_pin_set(dev, STORE, 1);
	}
}

/// @brief Fancy startup sequence to check if setup correctly
/// @param dev Runtime Device Stucture
void start_up_lighting(const struct device *dev)
{
	clear_display(dev);
	k_msleep(SLEEP_TIME_MS);
	print_text_to_display(dev, "-8.-8.-8.-8.-");
	k_msleep(SLEEP_TIME_MS);
	print_text_to_display(dev, "8.-8.-8.-8.-8.");
	k_msleep(SLEEP_TIME_MS);
	print_text_to_display(dev, "-8.-8.-8.-8.-");
	k_msleep(SLEEP_TIME_MS);
	print_text_to_display(dev, "8.-8.-8.-8.-8.");
	k_msleep(SLEEP_TIME_MS);
	clear_display(dev);
	k_msleep(SLEEP_TIME_MS);
}

/// @brief set callback on user interaction
/// @return if successfully added
bool configure_button()
{
	int ret = gpio_pin_configure_dt(&button, GPIO_INPUT);
	if (ret != 0) {
		printk("Error %d: failed to configure %s pin %d\n", ret, button.port->name,
		       button.pin);
		return false;
	}

	ret = gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret != 0) {
		printk("Error %d: failed to configure interrupt on %s pin %d\n", ret,
		       button.port->name, button.pin);
		return false;
	}

	gpio_init_callback(&button_cb_data, button_pressed, BIT(button.pin));
	gpio_add_callback(button.port, &button_cb_data);
	printk("Set up button at %s pin %d\n", button.port->name, button.pin);
	return true;
}

/// @brief Sets up various gpio pins for further use
/// @param dev Runtime Device Stucture
/// @return if successfully configured
bool configure_gpio_pins(const struct device *dev)
{
	gpio_pin_configure(dev, DATA, GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure(dev, STORE, GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure(dev, REFRESH, GPIO_OUTPUT_INACTIVE);

	return true;
}

/// @brief Main Loop.
/// Reads sensordata and sends them to display
/// @param
void main(void)
{
	printf("[%s] Device started\n", now_str());

	const struct device *const dht22 = DEVICE_DT_GET_ONE(aosong_dht);
	const struct device *const dev = led.port;

	if (!device_is_ready(dht22)) {
		printf("Device %s is not ready\n", dht22->name);
		return;
	}

	configure_gpio_pins(dev);
	configure_button();
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

		char text[10];
		if (info == TEMP) {
			sprintf(text, "%.1f C", sensor_value_to_double(&temperature));
		} else if (info == HUMIDITY) {
			sprintf(text, "%.1f H", sensor_value_to_double(&humidity));
		} else {
			sprintf(text, "%s", uptime_str());
		}

		clear_display(dev);
		print_text_to_display(dev, text);

		// debug output
		printf("[%s]: %.1f Cel ; %.1f %%RH\n", now_str(),
		       sensor_value_to_double(&temperature), sensor_value_to_double(&humidity));

		k_msleep(SLEEP_TIME_MS);
	}
}
