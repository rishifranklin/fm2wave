# fm2wave
Converts an IQ FM radio station stream to a wave file.

Generating the IQ stream:
Open the attached grc file in GNU-Radio Companion and adjust according to your SDR device.
This version supports only the BladeRF xA4. Generate a small fragment by tuning into your
FM station. The IQ data will be written to the file input.bin.

Generating the Wave file:
Feed the file input.bin to the fm2wave executable. This version FIR filter is too slow
therefore larger the file more time it will take to generate the wave file.
