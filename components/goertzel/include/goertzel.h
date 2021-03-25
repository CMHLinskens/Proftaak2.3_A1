#ifndef GOERTZEL_H
#define GOERTZEL_H

#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"

#ifdef __cplusplus
extern "C" {
#endif

struct goertzel_data_t;
// Callback
typedef void (*goertzel_cb_t)(struct goertzel_data_t* filter, float result);

/**
 * @brief Structure containing information related to the goertzel filter
 */
typedef struct
{
    int samples;                                        ///< Number of samples to process per time [N]
    int sample_rate;                          			///< Number of samples per second [Hz]
	int target_frequency;								///< Target frequency in [Hz]
	bool active;										///< Keep track if the configration is initialized
    float coefficient;                             		///< Calculated coefficient for Goertzel
	float scaling_factor;								///< Scaling factor for last goertzel phase.
	float omega;										///< Omega value of goertzel
    float q0;											///< Calculated Q0
	float q1;											///< Calculated Q1
	float q2;											///< Calculated Q2
	int sample_counter;									///< Number of samples calculated	
	goertzel_cb_t goertzel_cb;							///< Callback with results
} goertzel_data_t;

/**
 * @brief  Allocate number of Goertzel filters
 * @param  numOfConfigurations: The number of configurations
 * @return The goertzel configs
 */
goertzel_data_t** goertzel_malloc(int numOfConfigurations);

/**
 * @brief  Initialize goertzel configuration per configuration
 * @param  configs: Goertzel configuration
 * @return ESP_ERROR constant (ESP_OK and ESP_FAIL)
 */
esp_err_t goertzel_init_config(goertzel_data_t* config);

/**
 * @brief  Initialize goertzel configuration for multiple configurations
 * @param  configs: Goertzel configurations
 * @param  numOfConfigurations: The number of configurations
 * @return ESP_ERROR constant (ESP_OK and ESP_FAIL)
 */
esp_err_t goertzel_init_configs(goertzel_data_t** configs, int numOfConfigurations);

/**
 * @brief  Reset goertzel filter for a single configuration
 * @param  configs: Goertzel configurations
 * @return ESP_ERROR constant (ESP_OK and ESP_FAIL)
 */
esp_err_t goertzel_reset(goertzel_data_t* configs);

/**
 * @brief  Reset goertzel filters for multiple configurations
 * @param  configs: Goertzel configurations
 * @param  numOfConfigurations: The number of configurations
 * @return ESP_ERROR constant (ESP_OK and ESP_FAIL)
 */
esp_err_t goertzel_resets(goertzel_data_t** configs, int numOfConfigurations);

/**
 * @brief  Process all samples for all goertzel filters
 * @param  configs: Goertzel configurations
 * @param  numOfConfigurations: The number of configurations
 * @param  samples: The buffer with the raw data.
 * @param  numOfSamples: The size of the buffer.
 * @return ESP_ERROR constant (ESP_OK and ESP_FAIL)
 */
esp_err_t goertzel_proces(goertzel_data_t** configs, int numOfConfigurations, int16_t* samples, int numOfSamples);

/**
 * @brief  Free all goertzel configurations
 * @param  configs: Goertzel configurations
 * @return ESP_ERROR constant (ESP_OK and ESP_FAIL)
 */
esp_err_t goertzel_free(goertzel_data_t** configs);

#ifdef __cplusplus
}
#endif

#endif  // GOERTZEL_H
