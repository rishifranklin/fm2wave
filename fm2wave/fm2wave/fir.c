#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <complex.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

#include "fir.h"

#define PI 3.141592653589793

typedef _Fcomplex fcomplex_t;
typedef _Dcomplex dcomplex_t;

static fir_config_t fir_config;

/* Function to calculate the Kaiser window beta parameter */
static double_t kaiser_beta(void)
{
    double_t beta_value = 0.0;

    if (fir_config.ripple_db > 50.0)
    {
        beta_value = 0.1102 * (fir_config.ripple_db - 8.7);
    }
    else if (fir_config.ripple_db >= 21)
    {
        beta_value = 0.5842 * pow(fir_config.ripple_db - 21, 0.4) + 0.07886 * (fir_config.ripple_db - 21);
    }
    else 
    {
        beta_value = 0.0;
    }

    return beta_value;
}

/* Function to calculate the zeroth-order modified Bessel function of the first kind (I0) */
static double_t besseli0(double_t x)
{
    double_t sum = 1.0;
    double_t y = x * x / 4.0;
    double_t t = y;
    int32_t n = 1;

    while (t > (1e-10 * sum))
    {
        sum += t;
        n++;
        t *= y / (n * n);
    }

    return sum;
}

/* Function to generate FIR filter coefficients using the Kaiser window */
static void fir_generate_coeff(double_t *coeffs, int32_t filter_len) 
{
    double_t beta = kaiser_beta();
    int32_t M = filter_len - 1;

    /* Normalized cutoff frequency */
    double_t fc = fir_config.cutoff_frequency / fir_config.sample_rate;

    assert(coeffs != NULL);

    for (int32_t n = 0; n <= M; n++)
    {
        double_t alpha = M / 2.0;
        double_t t = n - alpha;

        double_t sinc_val = (t == 0.0) ? 1.0 : sin(2 * PI * fc * t) / (2 * PI * fc * t);
        double_t window = besseli0(beta * sqrt(1 - pow((t / alpha), 2))) / besseli0(beta);

        coeffs[n] = sinc_val * window * fir_config.gain;
    }
}

/* FIR filter function with decimation for complex samples */
static void fir_filter_and_decimate(FILE *input_file, float_t *output_float, double_t *coeffs, int32_t filter_len)
{
    int32_t count = 0;

    assert(input_file != NULL);
    assert(output_float != NULL);
    assert(coeffs != NULL);

    fcomplex_t *buffer = (fcomplex_t *)calloc(filter_len, sizeof(fcomplex_t));
    if (buffer == NULL)
    {
        perror("Memory allocation fail.");
        exit(EXIT_FAILURE);
    }

    int32_t buffer_index = 0;
    float_t input_sample[2];
    dcomplex_t output_sample;
    int32_t sample_count = 0;

    while (fread(input_sample, sizeof(float_t), 2, input_file) == 2)
    {
        fcomplex_t sample = _FCOMPLEX_(input_sample[0], input_sample[1]);
        buffer[buffer_index] = sample;

        /* Apply the filter when enough samples have been collected */
        if (sample_count >= (filter_len - 1))
        {
            /* Decimate by processing every 'decimation_factor' samples */
            if ((sample_count % fir_config.decimation_factor) == 0)
            {
                output_sample = _DCOMPLEX_(0.0f, 0.0f);
                for (int32_t i = 0; i < filter_len; i++)
                {
                    int32_t index = (buffer_index + (filter_len - i)) % filter_len;
                    fcomplex_t temp = _FCmulcr(buffer[index], coeffs[i]);

                    output_sample._Val[0] += temp._Val[0];
                    output_sample._Val[1] += temp._Val[1];
                }

                output_float[count] = (float_t)creal(output_sample);
                output_float[count + 1] = (float_t)cimag(output_sample);
                count += 2;
            }
        }

        buffer_index = (buffer_index + 1) % filter_len;
        sample_count++;
    }

    free(buffer);
    printf("Filter Complete = %d samples processed\n", sample_count);
}

void fir_init(fir_config_t *config)
{
    printf("Initializing filter...\n");

    if (config != NULL) 
    {
        fir_config.cutoff_frequency = config->cutoff_frequency;
        fir_config.decimation_factor = config->decimation_factor;
        fir_config.gain = config->gain;
        fir_config.ripple_db = config->ripple_db;
        fir_config.sample_rate = config->sample_rate;
        fir_config.transition_width = config->transition_width;
    }
}

void fir_filter_start(FILE *input, float_t *filtered_signal)
{
    assert(input != NULL);
    assert(filtered_signal != NULL);

    /* Calculate the filter length using the Kaiser formula */
    double_t delta_f = fir_config.transition_width / fir_config.sample_rate;
    int32_t filter_len = (int32_t)(ceil((fir_config.ripple_db - 8.0) / (2.285 * 2 * PI * delta_f))) + 1;

    printf("Starting filter...\n");

    if (filter_len % 2 == 0) 
    {
        /* Ensure filter length is odd for symmetry */
        filter_len += 1; 
    }

    printf("Filter Length = %d\n", filter_len);
    double_t *coeffs = (double_t *)malloc(filter_len * sizeof(double_t));
    if (!coeffs) 
    {
        perror("Error allocating memory for coefficients");
        exit(EXIT_FAILURE);
    }

    fir_generate_coeff(coeffs, filter_len);
    fir_filter_and_decimate(input, filtered_signal, coeffs, filter_len);
}

void fir_filter_test(float_t *filtered_signal, int32_t len, const char *filename)
{
    assert(filtered_signal != NULL);
    assert(filename != NULL);

    FILE *outfile = fopen(filename, "wb");

    fwrite((uint8_t *)filtered_signal, 1, len, outfile);

    fclose(outfile);
}
