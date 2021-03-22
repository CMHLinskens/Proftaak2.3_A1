#ifndef GOERTZEL_H
#define GOERTZEL_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include <stdbool.h>

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


goertzel_data_t** goertzel_malloc(int numOfConfigurations);

esp_err_t goertzel_init_config(goertzel_data_t* config);
esp_err_t goertzel_init_configs(goertzel_data_t** configs, int numOfConfigurations);

esp_err_t goertzel_reset(goertzel_data_t* configs);
esp_err_t goertzel_resets(goertzel_data_t** configs, int numOfConfigurations);

esp_err_t goertzel_proces(goertzel_data_t** configs, int numOfConfigurations, int16_t* samples, int numOfSamples);


esp_err_t goertzel_free(goertzel_data_t** configs);

#ifdef __cplusplus
}
#endif

#endif  // GOERTZEL_H
