#pragma once

#include <stdint.h>

typedef struct
{
	double cutoff_frequency;
	double transition_width;
	double ripple_db;
	double gain;
	double sample_rate;
	int32_t decimation_factor;
}fir_config_t;

void fir_init(fir_config_t* config);
void fir_filter_start(FILE* input, float_t* filtered_signal);
void fir_filter_test(float_t* filtered_signal, int32_t len, const char* filename);

