/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <devicetree.h>

// #include <logging/log.h>
#include <dk_buttons_and_leds.h>

// #include "adxl345.h"
#include <drivers/sensor.h>
#if !DT_HAS_COMPAT_STATUS_OKAY(adi_adxl345)
#error "No adi,adxl345 compatible node found in the device tree"
#endif

struct adxl345_data
{
    /* data */
    int16_t x;
    int16_t y;
    int16_t z;
};

#include "remote.h"

#define RUN_STATUS_LED DK_LED1
#define CONN_STATUS_LED DK_LED2
#define RUN_LED_BLINK_INTERVAL 250

static struct bt_conn *current_conn;
bool isNotify = false;
int counter = 0;

void timer_fn(struct k_timer *dummy)
{
	// k_work_submit(&my_work);
	counter++;
}

K_TIMER_DEFINE(my_timer, timer_fn, NULL);

/* Declarations */
void on_connected(struct bt_conn *conn, uint8_t err);
void on_disconnected(struct bt_conn *conn, uint8_t reason);
void on_notif_changed(enum bt_button_notifications_enabled status);
void on_data_received(struct bt_conn *conn, const uint8_t *const data, uint16_t len);

struct bt_conn_cb bluetooth_callbacks = {
	.connected 		= on_connected,
	.disconnected 	= on_disconnected,
};

struct bt_remote_service_cb remote_service_callbacks = {
	.notif_changed = on_notif_changed,
	.data_received = on_data_received,
};

/* Callback */
void on_connected(struct bt_conn *conn, uint8_t err)
{
	if(err) {
		printk("connection err: %d\n", err);
		return;
	}
	printk("Connected.\n");
	current_conn = bt_conn_ref(conn);
	dk_set_led_on(CONN_STATUS_LED);
}

void on_disconnected(struct bt_conn *conn, uint8_t reason)
{
	printk("Disconnected (reason: %d)\n", reason);
	dk_set_led_off(CONN_STATUS_LED);
	if(current_conn) {
		bt_conn_unref(current_conn);
		current_conn = NULL;
	}	
}

void on_notif_changed(enum bt_button_notifications_enabled status)
{
    if (status == BT_BUTTON_NOTIFICATIONS_ENABLED) {
		isNotify = true;
        printk("Notifications enabled\n");
    } else {
		isNotify = false;
        printk("Notifications disabled\n");
    }
}

void on_data_received(struct bt_conn *conn, const uint8_t *const data, uint16_t len)
{
    uint8_t temp_str[len+1];
    memcpy(temp_str, data, len);
    temp_str[len] = 0x00;

    printk("Received data on conn %p. Len: %d\n", (void *)conn, len);
    printk("Data: %s\n", temp_str);
}

void button_handler(uint32_t button_state, uint32_t has_changed)
{
    int err;
	int button_pressed = 0;
	if (has_changed & button_state)
	{
		switch (has_changed)
		{
			case DK_BTN1_MSK:
				button_pressed = 1;
				break;
			case DK_BTN2_MSK:
				button_pressed = 2;
				break;
			case DK_BTN3_MSK:
				button_pressed = 3;
				break;
			case DK_BTN4_MSK:
				button_pressed = 4;
				break;
			default:
				break;
		}
        printk("Button %d pressed.\n", button_pressed);       
		set_button_value(button_pressed);
		err = send_button_notification(current_conn, &button_pressed, 1);
		if (err) {
            printk("Couldn't send notificaton. (err: %d)\n", err);
        }
    }
}

/* Configurations */
static void configure_dk_buttons_leds(void)
{
    int err;
    err = dk_leds_init();
    if (err) {
        printk("Couldn't init LEDS (err %d)\n", err);
    }
    err = dk_buttons_init(button_handler);
    if (err) {
        printk("Couldn't init buttons (err %d)\n", err);
    }
}

void main(void)
{
	int err;
    int blink_status = 0;

	struct sensor_value accel[3], voltage;
	struct adxl345_data adxl345_data;

	printk("Hello World! %s\n", CONFIG_BOARD);

	configure_dk_buttons_leds();

	err = bluetooth_init(&bluetooth_callbacks, &remote_service_callbacks);
    if (err) {
        printk("Couldn't initialize Bluetooth. err: %d\n", err);
		return;
    }

	const struct device *sensor = DEVICE_DT_GET(DT_INST(0, adi_adxl345));

	if (sensor == NULL || !device_is_ready(sensor)) {
		printk("Could not get %s device\n", DT_LABEL(DT_INST(0, adi_adxl345)));
		// return;
	}

	const struct device *dev = DEVICE_DT_GET(DT_INST(0, ti_bq274xx));
	

	if (dev == NULL || !device_is_ready(dev)) {
		printk("Could not get %s device\n", DT_LABEL(DT_INST(0, ti_bq274xx)));
		// return;
	}

	k_timer_init(&my_timer, timer_fn, NULL);
	k_timer_start(&my_timer, K_NO_WAIT, K_MSEC(250));	

	while (1) {

		if(counter != 0){
			counter = 0;
			dk_set_led(RUN_STATUS_LED, (blink_status++)%2);		

			if (sensor_sample_fetch(sensor) < 0) {
				printk("sensor_sample_fetch failed\n");
			}		

			sensor_channel_get(sensor, SENSOR_CHAN_ACCEL_XYZ, accel);

			adxl345_data.x = (int16_t) sensor_value_to_double(&accel[0]);
			adxl345_data.y = (int16_t) sensor_value_to_double(&accel[1]);
			adxl345_data.z = (int16_t) sensor_value_to_double(&accel[2]);

			err = sensor_sample_fetch_chan(dev,
						  SENSOR_CHAN_GAUGE_VOLTAGE);
			if (err < 0) {
				printk("Unable to fetch the voltage\n");
				return;
			}

			err = sensor_channel_get(dev, SENSOR_CHAN_GAUGE_VOLTAGE,
							&voltage);
			if (err < 0) {
				printk("Unable to get the voltage value\n");
				return;
			}

			printk("Voltage: %d.%06dV\n", voltage.val1, voltage.val2);
			
			if(isNotify){
				err = send_adxl345_notification(current_conn, (uint8_t*)&adxl345_data, sizeof(adxl345_data));
				if (err) {
					printk("Couldn't send notificaton. (err: %d)\n", err);
				}
			}
			printk("ACC X : %d, Y: %d, Z: %d \r\n", adxl345_data.x, adxl345_data.y, adxl345_data.z); 
		}		
	}	
}


// #include <zephyr.h>
// #include "adxl345.h"
// #define APP_EVENT_QUEUE_SIZE 20

// enum app_event_type
// {
// 	APP_EVENT_TIMER,
// 	APP_EVENT_SENSOR_DATA,
// };

// struct app_event
// {
// 	enum app_event_type type;
// 	union 
// 	{
// 		int err;
// 		uint32_t value;
// 	};	
// };

// K_MSGQ_DEFINE(app_msgq, sizeof(struct app_event), APP_EVENT_QUEUE_SIZE, 4);

// void timer_expiry_fn(struct k_timer *dummy)
// {
// 	struct app_event evt = {
// 		.type = APP_EVENT_TIMER,
// 		.value = 1234,
// 	};

// 	k_msgq_put(&app_msgq, &evt, K_NO_WAIT);
// }

// K_TIMER_DEFINE(timer, timer_expiry_fn, NULL);

// void main(void)
// {
// 	struct app_event evt;

// 	printk("We are started\n");

// 	k_timer_start(&timer, K_SECONDS(1), K_SECONDS(1));

// 	while (true)
// 	{
// 		k_msgq_get(&app_msgq, &evt, K_FOREVER);

// 		printk("EVENT type: %i\n", evt.type);

// 		switch (evt.type)
// 		{
// 		case APP_EVENT_TIMER:
// 			printk("EVENT value: %i\n", evt.value);
// 			break;
		
// 		default:
// 			break;
// 		}
// 	}
	
// }