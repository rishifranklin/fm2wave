#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <string.h>

#include "fir.h"
#include "fm_demodulate.h"

static float_t *filtered_signal = NULL;

int32_t main(int32_t argc, char *argv[])
{
    fir_config_t fir_cfg;
    int32_t input_file_sz = 0;
    int32_t filtered_signal_sz = 0;
    FILE *fp = NULL;
    FILE *wav = NULL;

    if (argc != 3) 
    {
        fprintf(stderr, "Usage: %s input.bin output.wav\n", argv[0]);
        return 1;
    }

    /* Setup filter configuration */
    fir_cfg.cutoff_frequency = 70e3;
    fir_cfg.decimation_factor = 5;
    fir_cfg.gain = 2.0;
    fir_cfg.ripple_db = 60.0;
    fir_cfg.sample_rate = 2.4e6;
    fir_cfg.transition_width = 5e3;

    fir_init(&fir_cfg);

    fp = fopen(argv[1], "rb");
    if (fp == NULL)
    {
        fprintf(stderr, "Failed to open IQ file.\n");
        return 1;
    }

    /* Signal filtering */
    fseek(fp, 0L, SEEK_END);
    input_file_sz = ftell(fp);
    fseek(fp, 0L, SEEK_SET);

    filtered_signal_sz = input_file_sz / fir_cfg.decimation_factor;
    filtered_signal_sz = ((int32_t)(filtered_signal_sz / SAMPLE_BLK_SIZE) + 1) * SAMPLE_BLK_SIZE;
    filtered_signal = (float_t *)malloc(filtered_signal_sz);

    if (filtered_signal == NULL)
    {
        fprintf(stderr, "Memory allocation failure.\n");
        fclose(fp);
        return 1;
    }

    fir_filter_start(fp, filtered_signal);

    /* FM demodulation */
    wav = fopen(argv[2], "wb");
    if (wav == NULL)
    {
        fprintf(stderr, "Failed to open Wave file.\n");
        return 1;
    }

    fm_init(480000, filtered_signal_sz, wav);
    fm_process(filtered_signal);

#ifdef TEST_FILTER
    fir_filter_test(filtered_signal, filtered_signal_sz, "output.bin");
#endif

    free(filtered_signal);
    fclose(fp);
    fclose(wav);

	return 0;
}