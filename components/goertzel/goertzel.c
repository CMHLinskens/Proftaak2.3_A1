#include "goertzel.h"
#include "esp_log.h"
#include <math.h>

static const char *TAG = "goertzel";


// Allocate number of Goertzel filter
goertzel_data_t** goertzel_malloc(int numOfConfigurations) {

	goertzel_data_t** configs;
	configs = (goertzel_data_t**)malloc(numOfConfigurations*sizeof(goertzel_data_t*));
	
	if (configs == NULL) {
		ESP_LOGE(TAG, "Cannot malloc goertzel configuration");
		return ESP_FAIL;
	}
	
	for (int i = 0; i < numOfConfigurations; i++) {
		configs[i] = (goertzel_data_t*)malloc(sizeof(goertzel_data_t));
		if (configs[i] == NULL) {
			ESP_LOGE(TAG, "Cannot malloc goertzel configuration");
			return ESP_FAIL;
		}

	}
	return configs;
	
}

// Initialize goertzel configuration per configuration
esp_err_t goertzel_init_config(goertzel_data_t* config) {
	
    float floatnumSamples = (float) config->samples;
    int k = (int) (0.5f + ((floatnumSamples * config->target_frequency) / config->sample_rate));
    config->omega = (2.0f * M_PI * k) / floatnumSamples;
    float cosine = cosf(config->omega);
    config->coefficient = 2.0f * cosine;
	config->active = true;
	config->scaling_factor = floatnumSamples / 2.0f;
	
	// Reset initial filter q
	goertzel_reset(config);

	return ESP_OK;
}


// Initialize goertzel configuration for multiple configurations
esp_err_t goertzel_init_configs(goertzel_data_t** configs, int numOfConfigurations) {
	for (int i = 0; i < numOfConfigurations; i++) {
		esp_err_t ret = goertzel_init_config(configs[i]);
		if (ret != ESP_OK) {
			ESP_LOGE(TAG, "Cannot create Goertzel settings");
			return ret;
		}
	}
	return ESP_OK;
}

// Reset goertzel filters for a single configuration
esp_err_t goertzel_reset(goertzel_data_t* config) {
	
	config->q0 = 0.0f;
	config->q1 = 0.0f;
	config->q2 = 0.0f;
	config->sample_counter = 0;
	return ESP_OK;
}

// Reset goertzel filters for multiple configurations
esp_err_t goertzel_resets(goertzel_data_t** configs, int numOfConfigurations) {
	for (int i = 0; i < numOfConfigurations; i++) {
		goertzel_reset(configs[i]);
	}
	return ESP_OK;
}

// Process all samples for all goertzel filters
esp_err_t goertzel_proces(goertzel_data_t** configs, int numOfConfigurations, int16_t* samples, int numOfSamples) {
	
	for (int idx = 0; idx < numOfSamples; idx++) {
		float data = samples[idx];
		for (int filter = 0; filter < numOfConfigurations; filter++) {
			
			// Goertzel part 1
			goertzel_data_t* currFilter = configs[filter];
			currFilter->q0 = currFilter->coefficient * currFilter->q1 - currFilter->q2 + data;
			currFilter->q2 = currFilter->q1;
			currFilter->q1 = currFilter->q0;
			
			
			// Check if we have the amount of samples
			currFilter->sample_counter++;
			if (currFilter->sample_counter == currFilter->samples) {
				// Process part 2 
				
				float real = (currFilter->q1 - currFilter->q2 * cosf(currFilter->omega)) / currFilter->scaling_factor;
				float imag = (currFilter->q2 * sinf(currFilter->omega)) / currFilter->scaling_factor;
				float magnitude = sqrtf(real*real + imag*imag);
					
				// Provide callback support
				if (currFilter->goertzel_cb != NULL) {
					currFilter->goertzel_cb(currFilter, magnitude);
				}
				
				// Reset filter
				goertzel_reset(currFilter);
			}
			
		}
	}
	return ESP_OK;
}

// Free all goertzel configurations
esp_err_t goertzel_free(goertzel_data_t** configs) {
	if (configs != NULL && (*configs != NULL)) {
        ESP_LOGD(TAG, "free goertzel_data_t %p", *configs);
        free(*configs);
        *configs = NULL;
    } else {
        ESP_LOGE(TAG, "free goertzel_data_t failed");
		return ESP_FAIL;
    }
	
	return ESP_OK;
}