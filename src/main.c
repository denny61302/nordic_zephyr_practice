/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <devicetree.h>

#include <logging/log.h>
#include <dk_buttons_and_leds.h>

/* STEP 3 - Include the header file of the I2C API */
#include <drivers/i2c.h>
/* STEP 4.1 - Include the header file of printk() */
#include <sys/printk.h>
/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS   1000

#include "adxl345.h"

#include "remote.h"

#define LOG_MODULE_NAME app
LOG_MODULE_REGISTER(LOG_MODULE_NAME);
#define RUN_STATUS_LED DK_LED1
#define CONN_STATUS_LED DK_LED2
#define RUN_LED_BLINK_INTERVAL 100

static struct bt_conn *current_conn;
bool isNotify = false;

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
		LOG_ERR("connection err: %d", err);
		return;
	}
	LOG_INF("Connected.");
	current_conn = bt_conn_ref(conn);
	dk_set_led_on(CONN_STATUS_LED);
}

void on_disconnected(struct bt_conn *conn, uint8_t reason)
{
	LOG_INF("Disconnected (reason: %d)", reason);
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
        LOG_INF("Notifications enabled");
    } else {
		isNotify = false;
        LOG_INF("Notifications disabled");
    }
}

void on_data_received(struct bt_conn *conn, const uint8_t *const data, uint16_t len)
{
    uint8_t temp_str[len+1];
    memcpy(temp_str, data, len);
    temp_str[len] = 0x00;

    LOG_INF("Received data on conn %p. Len: %d", (void *)conn, len);
    LOG_INF("Data: %s", log_strdup(temp_str));
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
        LOG_INF("Button %d pressed.", button_pressed);       
		set_button_value(button_pressed);
		err = send_button_notification(current_conn, &button_pressed, 1);
		if (err) {
            LOG_ERR("Couldn't send notificaton. (err: %d)", err);
        }
    }
}

/* Configurations */
static void configure_dk_buttons_leds(void)
{
    int err;
    err = dk_leds_init();
    if (err) {
        LOG_ERR("Couldn't init LEDS (err %d)", err);
    }
    err = dk_buttons_init(button_handler);
    if (err) {
        LOG_ERR("Couldn't init buttons (err %d)", err);
    }
}

void main(void)
{

	int ret;
	int err;
    int blink_status = 0;

	struct adxl345_data adxl345_data;

	LOG_INF("Hello World! %s\n", CONFIG_BOARD);

	configure_dk_buttons_leds();

	err = bluetooth_init(&bluetooth_callbacks, &remote_service_callbacks);
    if (err) {
        LOG_INF("Couldn't initialize Bluetooth. err: %d", err);
    }

	/* Get the binding of the I2C driver  */
	const struct device *dev_i2c = device_get_binding(I2C0);
	if (dev_i2c == NULL) {
		LOG_INF("Could not find  %s!\n\r",I2C0);
		return;
	}

	/* Setup the sensor */
	ret = adxl345_init(dev_i2c);
	if(ret != 0){
        LOG_INF("Failed to init ADXL345");
    }

	while (1) {

		dk_set_led(RUN_STATUS_LED, (blink_status++)%2);		

		ret = readXYZ(dev_i2c, &adxl345_data);
        if(ret != 0){
            LOG_INF("Failed to read adxl345 data");
        }
		
		if(isNotify){
			err = send_button_notification(current_conn, (uint8_t*)&adxl345_data, sizeof(adxl345_data));
			if (err) {
				LOG_ERR("Couldn't send notificaton. (err: %d)", err);
			}
		}
		
		//Print reading to console  
		LOG_INF("ACC X : %d, Y: %d, Z: %d \r\n", adxl345_data.x, adxl345_data.y, adxl345_data.z);  

		k_sleep(K_MSEC(RUN_LED_BLINK_INTERVAL));
	}	
}