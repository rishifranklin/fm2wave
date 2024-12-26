#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>
#include <complex.h>
#include <assert.h>

#include "fm_demodulate.h"

#define PI 3.14159265358979323846

typedef _Fcomplex fcomplex_t;
typedef _Dcomplex dcomplex_t;

static uint32_t sample_rate = 0U;
static const uint32_t wav_sample_rate = 48000;
static uint32_t bb_len = 0;
static float_t *demod_buffer = NULL;
static fcomplex_t *iq_buffer = NULL;

static FILE *wave_file = NULL;

/* WAV file header structure */
typedef struct 
{
    char riff[4];               // "RIFF"
    uint32_t chunk_size;        // File size - 8 bytes
    char wave[4];               // "WAVE"
    char fmt[4];                // "fmt "
    uint32_t subchunk1_size;    // PCM = 16
    uint16_t audio_format;      // PCM = 1
    uint16_t num_channels;      // Mono = 1
    uint32_t sample_rate;       // 48000 Hz
    uint32_t byte_rate;         // SampleRate * NumChannels * BitsPerSample / 8
    uint16_t block_align;       // NumChannels * BitsPerSample / 8
    uint16_t bits_per_sample;   // 16 bits
    char data[4];               // "data"
    uint32_t data_size;         // NumSamples * NumChannels * BitsPerSample / 8
}wave_header_t;

void write_wav_header(FILE *wav_file, int32_t sample_rate, int32_t num_samples)
{
    wave_header_t header;

    assert(wav_file != NULL);

    memcpy(header.riff, "RIFF", 4);
    header.chunk_size = 36 + num_samples * sizeof(int16_t);
    memcpy(header.wave, "WAVE", 4);
    memcpy(header.fmt, "fmt ", 4);
    header.subchunk1_size = 16;
    header.audio_format = 1; /* PCM */
    header.num_channels = 1; /*Mono*/
    header.sample_rate = sample_rate;
    header.byte_rate = sample_rate * sizeof(int16_t);
    header.block_align = sizeof(int16_t);
    header.bits_per_sample = 16;
    memcpy(header.data, "data", 4);
    header.data_size = num_samples * sizeof(int16_t);

    fwrite(&header, sizeof(wave_header_t), 1, wav_file);
}

static float fm_demodulate(fcomplex_t current_sample, fcomplex_t previous_sample)
{
    float angle_diff = cargf(_FCmulcc(current_sample, conjf(previous_sample)));

    return angle_diff;
}

void fm_process(float_t *fm_baseband)
{
    fcomplex_t previous_sample = _FCOMPLEX_(0.0f, 0.0f);
    int32_t resample_factor = sample_rate / wav_sample_rate;
    int32_t resampled_samples = 0;
    int32_t total_samples = 0;
    int32_t copy_size = 0;

    assert(fm_baseband != NULL);

    int16_t *wav_buffer = (int16_t *)malloc((SAMPLE_BLK_SIZE / resample_factor + 1) * sizeof(int16_t));
    if (!wav_buffer)
    {
        perror("Error allocating memory for buffers");
        exit(EXIT_FAILURE);
    }

    for (int32_t i = 0; i < bb_len - SAMPLE_BLK_SIZE * 8; i = i + (SAMPLE_BLK_SIZE * 8))
    {
        /* Read a block of SAMPLE_BLK_SIZE */
        (void)memcpy((uint8_t *)iq_buffer + 0, (uint8_t *)fm_baseband + i, SAMPLE_BLK_SIZE * sizeof(fcomplex_t));

        /* demodulate the block */
        for (int32_t j = 0; j < SAMPLE_BLK_SIZE; j++)
        {
            demod_buffer[j] = fm_demodulate(iq_buffer[j], previous_sample);
            previous_sample = iq_buffer[j];
        }

        /* Resample for 48KHz audio */
        resampled_samples = 0;
        for (int32_t k = 0; k < SAMPLE_BLK_SIZE; k += resample_factor)
        {
            float_t sample = demod_buffer[k];

            /* Scale and clip the sample */
            if (sample > 1.0f) sample = 1.0f;
            if (sample < -1.0f) sample = -1.0f;

            wav_buffer[resampled_samples++] = (int16_t)(sample * 32767.0f);
            total_samples += resampled_samples;
        }

        fwrite(wav_buffer, sizeof(int16_t), resampled_samples, wave_file);
    }

    write_wav_header(wave_file, WAVE_AUDIO_SAMPLE_RATE, total_samples);

    free(iq_buffer);
    free(demod_buffer);
    free(wav_buffer);
}

void fm_init(int32_t bb_sample_rate, int32_t bb_size, FILE *outfile)
{
    sample_rate = bb_sample_rate;
    bb_len = bb_size;
    wave_file = outfile;

    assert(outfile != NULL);

    iq_buffer = (fcomplex_t *)malloc(SAMPLE_BLK_SIZE * sizeof(fcomplex_t));
    demod_buffer = (float_t *)malloc(SAMPLE_BLK_SIZE * sizeof(float_t));

    if (!iq_buffer || !demod_buffer)
    {
        perror("Error allocating memory for buffers");
        exit(EXIT_FAILURE);
    }

    write_wav_header(outfile, WAVE_AUDIO_SAMPLE_RATE, 0);
}
