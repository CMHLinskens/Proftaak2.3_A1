/* 
QWIIC_TWIST Library for I2C
Author: P.S.M.Goossens
*/

#include "qwiic_twist.h"
	
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_attr.h"
#include "audio-board.h"
#include <string.h>

static const char *TAG = "QWIIC_TWIST";

void qwiic_twist_task(void* pvParameters);

static int volume = 0;
static int r = 0;
static int g = 255;

/*
initialize the rotary encoder, more specific in the settings of the encoder. Like the time of the task etc
*/
esp_err_t qwiic_twist_init(qwiic_twist_t* config) {

    // Set up the SMBus
    config->smbus_info = smbus_malloc();
	config->i2c_addr = QWIIC_TWIST_ADDRESS;
    config->xMutex = xSemaphoreCreateMutex();
    config->task_enabled = true;
    config->task_time = 0;

    smbus_init(config->smbus_info, config->port, config->i2c_addr);
    smbus_set_timeout(config->smbus_info, 1000 / portTICK_RATE_MS);
	i2c_set_timeout(I2C_NUM_0, 20000);
	
	// Initialize Mutex
	//config->xMutex = xSemaphoreCreateMutex();
	return ESP_OK;
}


bool qwiic_twist_conntected(qwiic_twist_t* config) {
	return true;
}

/*
this method will change the color of the rotary encoder, by writing each value to a specific address
*/
esp_err_t qwiic_twist_set_color(qwiic_twist_t* config, uint8_t r, uint8_t g, uint8_t b) {
	
	//uint32_t data = (uint32_t)r << 0 | (uint32_t)g << 8 | (uint32_t)b << 16;
	//esp_err_t err = smbus_write_block(config->smbus_info, QWIIC_TWIST_LED_BRIGHTNESS_RED, (uint8_t*)&data, 3);
	
	xSemaphoreTake( config->xMutex, portMAX_DELAY );
	esp_err_t err = smbus_write_byte(config->smbus_info, QWIIC_TWIST_LED_BRIGHTNESS_RED, r);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "Error setting red color. Result %d", err);
		xSemaphoreGive( config->xMutex );
		return err;
	}
	err = smbus_write_byte(config->smbus_info, QWIIC_TWIST_LED_BRIGHTNESS_GREEN, g);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "Error setting green color. Result %d", err);
		xSemaphoreGive( config->xMutex );
		return err;
	}
	err = smbus_write_byte(config->smbus_info, QWIIC_TWIST_LED_BRIGHTNESS_BLUE, b);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "Error setting blue color. Result %d", err);
		xSemaphoreGive( config->xMutex );
		return err;
	}
	
	xSemaphoreGive( config->xMutex );
	
	return err;
}

void set_volume_color(qwiic_twist_t* config){
	volume = get_volume();

	if(volume < 0){
		ESP_LOGE(TAG, "Volume is not initiaised yet!");
	}

	if(volume == 50){
		r = 255;
		g = 255;

	}else if(volume < 50 ){
		r = RGB_STEP * volume;

	} else if(volume > 50){
		g = 255 - (RGB_STEP * volume);
	}
	
	if(volume  == 0){
		r = 0;
		g = 255;
	}
	else if(volume == 100){
		r = 255;
		g = 0;
	}
	
	qwiic_twist_set_color(config, r, g, 0);
}

/*
this method gets the color of the rotary encoder, by reading each value of a specific address
*/
esp_err_t qwiic_twist_get_color(qwiic_twist_t* config, uint8_t *r, uint8_t *g, uint8_t *b) {
	
	xSemaphoreTake( config->xMutex, portMAX_DELAY );

	esp_err_t err =  smbus_read_byte(config->smbus_info, QWIIC_TWIST_LED_BRIGHTNESS_RED, r);
	err =  smbus_read_byte(config->smbus_info, QWIIC_TWIST_LED_BRIGHTNESS_GREEN, g);
	err =  smbus_read_byte(config->smbus_info, QWIIC_TWIST_LED_BRIGHTNESS_BLUE, b);
	
	xSemaphoreGive( config->xMutex );
	
	return err;
}

/*
gets the current version of the rotary encoder by reading a specific byte
*/
esp_err_t qwiic_twist_get_version(qwiic_twist_t* config, uint16_t* version) {
	
	xSemaphoreTake( config->xMutex, portMAX_DELAY );
	
	esp_err_t err = smbus_read_byte(config->smbus_info, QWIIC_TWIST_VERSION, (uint8_t*)version);
	if (err != ESP_OK)
		ESP_LOGE(TAG, "Err reading version %d", err);
	
	xSemaphoreGive( config->xMutex );
	
	return err;
}

/*
gets the count by reading a specific byte
*/
esp_err_t qwiic_twist_get_count(qwiic_twist_t* config, int16_t *count) {
	
	xSemaphoreTake( config->xMutex, portMAX_DELAY );
	
	esp_err_t err = smbus_read_word(config->smbus_info, QWIIC_TWIST_ENCODER_COUNT_LSB, (uint16_t*)count);
	
	xSemaphoreGive( config->xMutex );
	
	return err;
}

/*
sets the count by writing a specific byte
*/
esp_err_t qwiic_twist_set_count(qwiic_twist_t* config, int16_t count) {
	
	xSemaphoreTake( config->xMutex, portMAX_DELAY );
	
	esp_err_t err =  smbus_write_word(config->smbus_info, QWIIC_TWIST_ENCODER_COUNT_LSB, (uint16_t)count);
	
	xSemaphoreGive( config->xMutex );
	
	return err;
}

esp_err_t qwiic_twist_get_diff(qwiic_twist_t* config, int16_t *count, bool clearValue) {
	
	xSemaphoreTake( config->xMutex, portMAX_DELAY );
	
	esp_err_t err =  smbus_read_word(config->smbus_info, QWIIC_TWIST_ENCODER_DIFFERENCE_LSB, (uint16_t*)count);
	
	if (clearValue == true) {
		err =  smbus_write_word(config->smbus_info, QWIIC_TWIST_ENCODER_DIFFERENCE_LSB, (uint16_t)0x00);
	}
	
	xSemaphoreGive( config->xMutex );
	
	return err;
}


/*
gets the status by reading a specific byte. 
*/
esp_err_t qwiic_twist_get_status(qwiic_twist_t* config, uint8_t* result) {
	
	xSemaphoreTake( config->xMutex, portMAX_DELAY );
	
	esp_err_t err = smbus_read_byte(config->smbus_info, QWIIC_TWIST_STATUS, result);
	
	err = smbus_write_byte(config->smbus_info, QWIIC_TWIST_STATUS, 0x00);
	
	xSemaphoreGive( config->xMutex );
	
	return err;
}


/*
checks if the rotary encoder is moved by reading a specific address
*/
esp_err_t qwiic_twist_is_moved(qwiic_twist_t* config, bool* result) {
	
	xSemaphoreTake( config->xMutex, portMAX_DELAY );
	
	uint8_t status = 0;
	esp_err_t err = smbus_read_byte(config->smbus_info, QWIIC_TWIST_STATUS, &status);
	
	*result = status & (1 << QWIIC_TWIST_STATUS_MOVED);
	
	err = smbus_write_byte(config->smbus_info, QWIIC_TWIST_STATUS, (status & ~(1 << QWIIC_TWIST_STATUS_MOVED)));  //We've read this status bit, now clear it
	
	xSemaphoreGive( config->xMutex );
	
	return err;
}

/*
checks if the rotary encoder is pressed by reading a specific address
*/
esp_err_t qwiic_twist_is_pressed(qwiic_twist_t* config, bool* result) {
	
	xSemaphoreTake( config->xMutex, portMAX_DELAY );
	
	uint8_t status = 0;
	esp_err_t err = smbus_read_byte(config->smbus_info, QWIIC_TWIST_STATUS, &status);
	
	*result = status & (1 << QWIIC_TWIST_STATUS_PRESSED);
	
	err = smbus_write_byte(config->smbus_info, QWIIC_TWIST_STATUS, (status & ~(1 << QWIIC_TWIST_STATUS_PRESSED)));  //We've read this status bit, now clear it
	
	xSemaphoreGive( config->xMutex );
	
	return err;
}

/*
checks if the rotary encoder is clicked by reading a specific address
*/
esp_err_t qwiic_twist_is_clicked(qwiic_twist_t* config, bool* result) {
	
	xSemaphoreTake( config->xMutex, portMAX_DELAY );
	
	uint8_t status = 0;
	esp_err_t err = smbus_read_byte(config->smbus_info, QWIIC_TWIST_STATUS, &status);
	
	*result = status & (1 << QWIIC_TWIST_STATUS_CLICKED);
	
	err = smbus_write_byte(config->smbus_info, QWIIC_TWIST_STATUS, (status & ~(1 << QWIIC_TWIST_STATUS_CLICKED)));  //We've read this status bit, now clear it
	
	xSemaphoreGive( config->xMutex );
	
	return err;
}

//Returns the number of milliseconds since the last encoder movement
esp_err_t qwiic_twist_timeSinceLastMovement(qwiic_twist_t* config, uint16_t* result, bool clearValue) {
	
	xSemaphoreTake( config->xMutex, portMAX_DELAY );
	
	esp_err_t err =  smbus_read_word(config->smbus_info, QWIIC_TWIST_TIME_SINCE_LAST_MOVEMENT_LSB, result);
	
	//Clear the current value if requested
	if (clearValue == true)
		err = smbus_write_word(config->smbus_info, QWIIC_TWIST_TIME_SINCE_LAST_MOVEMENT_LSB, (uint16_t)0x00);

	xSemaphoreGive( config->xMutex );
  
	return err;
}	

//Returns the number of milliseconds since the last button event (press and release)
esp_err_t qwiic_twist_timeSinceLastPress(qwiic_twist_t* config, uint16_t* result, bool clearValue) {
	
	xSemaphoreTake( config->xMutex, portMAX_DELAY );
	
	esp_err_t err =  smbus_read_word(config->smbus_info, QWIIC_TWIST_TIME_SINCE_LAST_BUTTON_LSB, result);
	
	//Clear the current value if requested
	if (clearValue == true)
		err = smbus_write_word(config->smbus_info, QWIIC_TWIST_TIME_SINCE_LAST_BUTTON_LSB, (uint16_t)0x00);

	xSemaphoreGive( config->xMutex );
	
	return err;
}    


/*
this task works like a event handler. when this task is active, this method checks the input of the rotary encoder constantly. 
When a specific input has been occurd, a method will be called.
*/
void qwiic_twist_task(void* pvParameters)
{
	qwiic_twist_t* config = (qwiic_twist_t*)pvParameters;
	uint8_t result = 0;
	esp_err_t err = 0;
	int16_t movement = 0;

	set_volume_color(config);
	
    while (config->task_enabled) {
		
		//xSemaphoreTake( config->xMutex, portMAX_DELAY );
		err = qwiic_twist_get_status(config, &result);
		if (err != ESP_OK) {
			ESP_LOGI(TAG, "Error in task: %d", err);
		}
		
		if(get_volume() != volume){
			set_volume_color(config);
		}

		// Check the result and fire callbacks
		// Click event
		if ((result & (1<<QWIIC_TWIST_STATUS_CLICKED)) > 0) {
			if (config->onButtonClicked != NULL) {
				config->onButtonClicked();
			}
		}
		
		// Pressed event
		if ((result & (1<<QWIIC_TWIST_STATUS_PRESSED)) > 0) {
			if (config->onButtonPressed != NULL) {
				config->onButtonPressed();
			}
		}
		
		// Moved event
		if ((result & (1<<QWIIC_TWIST_STATUS_MOVED)) > 0) {
			if (config->onMoved != NULL) {
				// Get the amount of movement
				
				qwiic_twist_get_diff(config, &movement, true);
				config->onMoved(movement);
			}
		}
		//xSemaphoreGive( config->xMutex );
		vTaskDelay(config->task_time / portTICK_RATE_MS);
    }
	
	ESP_LOGI(TAG, "task stopped");
    vTaskDelete(NULL);
}

/*
starts the task to check the input
*/
esp_err_t qwiic_twist_start_task(qwiic_twist_t* config) {
	config->task_enabled = true;
	if (config->task_time == 0) {
		config->task_time = 100;
	}
	ESP_LOGI(TAG, "Starting task");
	xTaskCreate(qwiic_twist_task, "qwiic_twist_task", 1024 * 2, (void*)config, 10, NULL);
			
	return ESP_OK;
}

/*
stops the task to check the input
*/
esp_err_t qwiic_twist_stop_task(qwiic_twist_t* config) {
	config->task_enabled = false;
	
	ESP_LOGI(TAG, "Stopping task");
	return ESP_OK;
}
