#ifndef MICROPHONE_H
#define MICROPHONE_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "board.h"
#include "audio_common.h"
#include "audio_pipeline.h"
#include "i2s_stream.h"
#include "raw_stream.h"
#include "filter_resample.h"
#include "goertzel.h"
#include <math.h>

void init_microphone(void* goertzel_callback);

#endif  // MICROPHONE_H