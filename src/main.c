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

#include "adxl345.h"

#include "remote.h"

// #define LOG_MODULE_NAME app
// LOG_MODULE_REGISTER(LOG_MODULE_NAME);
#define RUN_STATUS_LED DK_LED1
#define CONN_STATUS_LED DK_LED2
#define RUN_LED_BLINK_INTERVAL 250

static struct bt_conn *current_conn;
bool isNotify = false;
int counter = 0;

// static struct k_work my_work;

// static void my_work_fn(struct k_work *work)
// {
// 	LOG_ERR("Timer\n");
// }

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

	int ret;
	int err;
    int blink_status = 0;

	struct adxl345_data adxl345_data;

	printk("Hello World! %s\n", CONFIG_BOARD);

	configure_dk_buttons_leds();

	err = bluetooth_init(&bluetooth_callbacks, &remote_service_callbacks);
    if (err) {
        printk("Couldn't initialize Bluetooth. err: %d\n", err);
		return;
    }

	/* Get the binding of the I2C driver  */
	const struct device *dev_i2c = device_get_binding(I2C0);
	if (dev_i2c == NULL) {
		printk("Could not find  %s!\n",I2C0);
		return;
	}

	/* Setup the sensor */
	ret = adxl345_init(dev_i2c);
	if(ret != 0){
        printk("Failed to init ADXL345\n");
		return;
    }

	k_timer_init(&my_timer, timer_fn, NULL);
	k_timer_start(&my_timer, K_NO_WAIT, K_MSEC(250));	
	// k_work_init(&my_work, my_work_fn);

	while (1) {

		if(counter != 0){
			counter = 0;
			dk_set_led(RUN_STATUS_LED, (blink_status++)%2);			

			ret = readXYZ(dev_i2c, &adxl345_data);
			if(ret != 0){
			    printk("Failed to read adxl345 data\n");
			}
			
			if(isNotify){
				err = send_adxl345_notification(current_conn, (uint8_t*)&adxl345_data, sizeof(adxl345_data));
				if (err) {
					printk("Couldn't send notificaton. (err: %d)\n", err);
				}
			}
			printk("ACC X : %d, Y: %d, Z: %d \r\n", adxl345_data.x, adxl345_data.y, adxl345_data.z); 
		}
		// dk_set_led(RUN_STATUS_LED, (blink_status++)%2);			

		// ret = readXYZ(dev_i2c, &adxl345_data);
        // if(ret != 0){
        //     LOG_INF("Failed to read adxl345 data");
        // }
		
		// if(isNotify){
		// 	err = send_adxl345_notification(current_conn, (uint8_t*)&adxl345_data, sizeof(adxl345_data));
		// 	if (err) {
		// 		LOG_ERR("Couldn't send notificaton. (err: %d)", err);
		// 	}
		// }
		
		// //Print reading to console  
		// LOG_INF("ACC X : %d, Y: %d, Z: %d \r\n", adxl345_data.x, adxl345_data.y, adxl345_data.z);  

		// k_sleep(K_MSEC(RUN_LED_BLINK_INTERVAL));
	}	
}