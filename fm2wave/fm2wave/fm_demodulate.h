#pragma once

#include <stdint.h>

#define SAMPLE_BLK_SIZE         (1024)
#define WAVE_AUDIO_SAMPLE_RATE  (48000)

void fm_init(int32_t bb_sample_rate, int32_t bb_size, FILE *outfile);
void fm_process(float *fm_baseband);
