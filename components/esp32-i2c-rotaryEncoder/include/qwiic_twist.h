#ifndef QWIIC_TWIST_H
#define QWIIC_TWIST_H

#include "smbus.h"


#include "esp_err.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "freertos/task.h"
#include "freertos/FreeRTOS.h"

//#include "esp_peripherals.h"
#include "esp_log.h"
#include <stdint.h>
#include <stdbool.h>

#include <math.h>

#define QWIIC_TWIST_ADDRESS 0x3F

#define QWIIC_TWIST_STATUS_CLICKED 2
#define QWIIC_TWIST_STATUS_PRESSED 1
#define QWIIC_TWIST_STATUS_MOVED 0

#define RGB_STEP 5.1


typedef uint8_t qwiic_twist_reg_t;


enum qwiic_twist_reg {
	QWIIC_TWIST_STATUS = 0x01,  //2 - button clicked, 1 - button pressed, 0 - encoder moved
	QWIIC_TWIST_VERSION = 0x02,
	QWIIC_TWIST_INT_ENABLE = 0x04,
	QWIIC_TWIST_ENCODER_COUNT_LSB = 0x05,
    QWIIC_TWIST_ENCODER_COUNT_MSB = 0x06,
    QWIIC_TWIST_ENCODER_DIFFERENCE_LSB = 0x07,
	QWIIC_TWIST_ENCODER_DIFFERENCE_MSB = 0x08,
	QWIIC_TWIST_TIME_SINCE_LAST_MOVEMENT_LSB = 0x09,
	QWIIC_TWIST_TIME_SINCE_LAST_MOVEMENT_MSB = 0x0A,
	QWIIC_TWIST_TIME_SINCE_LAST_BUTTON_LSB = 0x0B,
	QWIIC_TWIST_TIME_SINCE_LAST_BUTTON_MSB = 0x0C,
	QWIIC_TWIST_LED_BRIGHTNESS_RED = 0x0D,
	QWIIC_TWIST_LED_BRIGHTNESS_GREEN = 0x0E,
	QWIIC_TWIST_LED_BRIGHTNESS_BLUE = 0x0F,
	QWIIC_TWIST_TURN_INTERRUPT_TIMEOUT_LSB = 0x16
};


typedef struct
{
	smbus_info_t * smbus_info;                          ///< Pointer to associated SMBus info
	uint8_t i2c_addr;
	i2c_port_t port;
	SemaphoreHandle_t xMutex;

	// Tasks settings
	bool task_enabled;
	uint16_t task_time;
	
	void(*onButtonPressed)(void);   //Called when button of rotary encoder is pressed
	void(*onButtonClicked)(void);   //Called when button of rotary encoder is clicked
    void(*onMoved)(int16_t);        //Called when rotary encoder moved
	
	
} qwiic_twist_t;


esp_err_t qwiic_twist_init(qwiic_twist_t* config);

bool qwiic_twist_conntected(qwiic_twist_t* config);

esp_err_t qwiic_twist_start_task(qwiic_twist_t* config);

esp_err_t qwiic_twist_stop_task(qwiic_twist_t* config);

esp_err_t qwiic_twist_set_color(qwiic_twist_t* config, uint8_t r, uint8_t g, uint8_t b);
esp_err_t qwiic_twist_get_color(qwiic_twist_t* config, uint8_t *r, uint8_t *g, uint8_t *b);

// TODO seperate colors with get and set

esp_err_t qwiic_twist_get_count(qwiic_twist_t* config, int16_t *count);
esp_err_t qwiic_twist_set_count(qwiic_twist_t* config, int16_t count);

esp_err_t qwiic_twist_get_diff(qwiic_twist_t* config, int16_t *count, bool clearValue);

esp_err_t qwiic_twist_get_status(qwiic_twist_t* config, uint8_t* result);

esp_err_t qwiic_twist_is_moved(qwiic_twist_t* config, bool* result);

esp_err_t qwiic_twist_is_pressed(qwiic_twist_t* config, bool* result);

esp_err_t qwiic_twist_is_clicked(qwiic_twist_t* config, bool* result);

//Returns the number of milliseconds since the last encoder movement
esp_err_t qwiic_twist_timeSinceLastMovement(qwiic_twist_t* config, uint16_t* result, bool clearValue); 

//Returns the number of milliseconds since the last button event (press and release)
esp_err_t qwiic_twist_timeSinceLastPress(qwiic_twist_t* config, uint16_t* result, bool clearValue);    


esp_err_t qwiic_twist_get_version(qwiic_twist_t* config, uint16_t* version);

void changeRGB(qwiic_twist_t*, int R, int G, int B);

#endif
