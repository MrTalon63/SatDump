#include "constellation.h"
#include <cmath>
// #include <iostream>
#include <vector>

#define M_SQRT2 1.41421356237309504880
#ifndef M_PI
#define M_PI 3.14159265358979323846 /* pi */
#endif

#include "logger.h"
#include "core/exception.h"

namespace dsp
{
    complex_t constellation_t::polar(float r, int n, float i)
    {
        float a = i * 2 * M_PI / n;
        return complex_t(r * cosf(a), r * sinf(a));
    }

    constellation_t::constellation_t(constellation_type_t type, float g1, float g2) : const_type(type)
    {
        if (type == BPSK)
        {
            const_states = 2;
            const_bits = 1;

            constellation = new complex_t[const_states];

            constellation[0] = complex_t(-1, 0);
            constellation[1] = complex_t(1, 0);
        }
        else if (type == QPSK || type == OQPSK) // Distinction is NOT at constellation level
        {
            const_states = 4;
            const_bits = 2;
            const_amp = 3;

            constellation = new complex_t[const_states];

            // Default constellation, Gray-Coded
            constellation[0] = complex_t(-M_SQRT2, -M_SQRT2);
            constellation[1] = complex_t(M_SQRT2, -M_SQRT2);
            constellation[2] = complex_t(-M_SQRT2, M_SQRT2);
            constellation[3] = complex_t(M_SQRT2, M_SQRT2);
        }
        else if (type == PSK8)
        {
            const_states = 8;
            const_bits = 3;

            constellation = new complex_t[const_states];

            // Gray-coded
            float rcp_sqrt_2 = 0.70710678118654752440;
            constellation[0] = complex_t(0.0, -1.0);
            constellation[1] = complex_t(-rcp_sqrt_2, rcp_sqrt_2);
            constellation[2] = complex_t(rcp_sqrt_2, -rcp_sqrt_2);
            constellation[3] = complex_t(0.0, 1.0);
            constellation[4] = complex_t(-rcp_sqrt_2, -rcp_sqrt_2);
            constellation[5] = complex_t(-1.0, 0.0);
            constellation[6] = complex_t(1.0, 0.0);
            constellation[7] = complex_t(rcp_sqrt_2, rcp_sqrt_2);
        }
        else if (type == QAM16)
        {
            const_states = 16;
            const_bits = 4;

            constellation = new complex_t[const_states];

            // Normalize for average power of 1
            float scale = 1.0f / sqrtf(10.0f);

            // DVB-T style Gray-mapped 16-QAM
            for (int b = 0; b < 16; b++)
            {
                int b0 = (b >> 0) & 1;
                int b1 = (b >> 1) & 1;
                int b2 = (b >> 2) & 1;
                int b3 = (b >> 3) & 1;

                float i_val = (1 - 2 * b0) * (2 - (1 - 2 * b2));
                float q_val = (1 - 2 * b1) * (2 - (1 - 2 * b3));

                constellation[b] = complex_t(i_val * scale, q_val * scale);
            }
        }
        else if (type == APSK16)
        {
            const_states = 16;
            const_bits = 4;
            const_amp = 100;
            const_sca = 1; // 0.5;
            const_prescale = 0.53;

            constellation = new complex_t[const_states];

            float gamma1 = g1;

            if (!gamma1)
                gamma1 = 2.57;

            float r1 = sqrtf(4 / (1 + 3 * gamma1 * gamma1));
            float r2 = gamma1 * r1;
            r1 *= 0.5;
            r2 *= 0.5;

            constellation[15] = polar(r2, 12, 1.5) * const_amp;
            constellation[14] = polar(r2, 12, 10.5) * const_amp;
            constellation[13] = polar(r2, 12, 4.5) * const_amp;
            constellation[12] = polar(r2, 12, 7.5) * const_amp;
            constellation[11] = polar(r2, 12, 0.5) * const_amp;
            constellation[10] = polar(r2, 12, 11.5) * const_amp;
            constellation[9] = polar(r2, 12, 5.5) * const_amp;
            constellation[8] = polar(r2, 12, 6.5) * const_amp;
            constellation[7] = polar(r2, 12, 2.5) * const_amp;
            constellation[6] = polar(r2, 12, 9.5) * const_amp;
            constellation[5] = polar(r2, 12, 3.5) * const_amp;
            constellation[4] = polar(r2, 12, 8.5) * const_amp;
            constellation[3] = polar(r1, 4, 0.5) * const_amp;
            constellation[2] = polar(r1, 4, 3.5) * const_amp;
            constellation[1] = polar(r1, 4, 1.5) * const_amp;
            constellation[0] = polar(r1, 4, 2.5) * const_amp;
        }
        else if (type == APSK32)
        {
            const_states = 32;
            const_bits = 5;
            const_amp = 100;
            const_sca = 1; // 0.5;
            const_prescale = 0.54;

            constellation = new complex_t[const_states];

            float gamma1 = g1;
            float gamma2 = g2;

            if (!gamma1)
                gamma1 = 2.53;
            if (!gamma2)
                gamma2 = 4.30;

            float r1 = sqrtf(8 / (1 + 3 * gamma1 * gamma1 + 4 * gamma2 * gamma2));
            float r2 = gamma1 * r1;
            float r3 = gamma2 * r1;
            r1 *= 0.5;
            r2 *= 0.5;
            r3 *= 0.5;

            constellation[31] = polar(r2, 12, 1.5) * const_amp;
            constellation[30] = polar(r2, 12, 2.5) * const_amp;
            constellation[29] = polar(r2, 12, 10.5) * const_amp;
            constellation[28] = polar(r2, 12, 9.5) * const_amp;
            constellation[27] = polar(r2, 12, 4.5) * const_amp;
            constellation[26] = polar(r2, 12, 3.5) * const_amp;
            constellation[25] = polar(r2, 12, 7.5) * const_amp;
            constellation[24] = polar(r2, 12, 8.5) * const_amp;
            constellation[23] = polar(r3, 16, 1) * const_amp;
            constellation[22] = polar(r3, 16, 3) * const_amp;
            constellation[21] = polar(r3, 16, 14) * const_amp;
            constellation[20] = polar(r3, 16, 12) * const_amp;
            constellation[19] = polar(r3, 16, 6) * const_amp;
            constellation[18] = polar(r3, 16, 4) * const_amp;
            constellation[17] = polar(r3, 16, 9) * const_amp;
            constellation[16] = polar(r3, 16, 11) * const_amp;
            constellation[15] = polar(r2, 12, 0.5) * const_amp;
            constellation[14] = polar(r1, 4, 0.5) * const_amp;
            constellation[13] = polar(r2, 12, 11.5) * const_amp;
            constellation[12] = polar(r1, 4, 3.5) * const_amp;
            constellation[11] = polar(r2, 12, 5.5) * const_amp;
            constellation[10] = polar(r1, 4, 1.5) * const_amp;
            constellation[9] = polar(r2, 12, 6.5) * const_amp;
            constellation[8] = polar(r1, 4, 2.5) * const_amp;
            constellation[7] = polar(r3, 16, 0) * const_amp;
            constellation[6] = polar(r3, 16, 2) * const_amp;
            constellation[5] = polar(r3, 16, 15) * const_amp;
            constellation[4] = polar(r3, 16, 13) * const_amp;
            constellation[3] = polar(r3, 16, 7) * const_amp;
            constellation[2] = polar(r3, 16, 5) * const_amp;
            constellation[1] = polar(r3, 16, 8) * const_amp;
            constellation[0] = polar(r3, 16, 10) * const_amp;
        }
        else
        {
            throw satdump_exception("Undefined constellation type!");
        }
    }

    constellation_t::~constellation_t()
    {
        delete[] constellation;
    }

    complex_t constellation_t::mod(uint8_t symbol)
    {
        return (constellation[symbol] / const_amp) / const_prescale;
    };

    uint8_t constellation_t::demod(complex_t sample)
    {
        switch (const_type)
        {
        case BPSK:
            return sample.real > 0;
            break;

        case QPSK:
            return 2 * (sample.imag > 0) + (sample.real > 0);
            break;

        case OQPSK:
            return 2 * (sample.imag > 0) + (sample.real > 0);
            break;

        case PSK8:
        case QAM16:
        case APSK16:
        case APSK32:
        {
            complex_t scaled_sample = sample;
            if (const_amp != 1.0f)
                scaled_sample = scaled_sample * const_amp;
            if (const_prescale != 1.0f)
                scaled_sample = scaled_sample * const_prescale;

            uint8_t best_symbol = 0;
            float min_dist = std::numeric_limits<float>::max();
            for (int i = 0; i < const_states; i++)
            {
                float dist = std::abs(std::complex<float>(scaled_sample - constellation[i]));
                if (dist < min_dist)
                {
                    min_dist = dist;
                    best_symbol = i;
                }
            }
            return best_symbol;
        }
        default:
            return 0;
            break;
        }
    };

    uint8_t constellation_t::soft_demod(int8_t *sample)
    {
        switch (const_type)
        {
        case BPSK:
            return sample[0] > 0;
            break;

        case QPSK:
            return 2 * (sample[1] > 0) + (sample[0] > 0);
            break;

        case OQPSK:
            return 2 * (sample[1] > 0) + (sample[0] > 0);
            break;

        case PSK8:
            return 4 * (sample[2] > 0) + 2 * (sample[1] > 0) + (sample[0] > 0);
            break;

        case QAM16:
        case APSK16:
            return 8 * (sample[3] > 0) + 4 * (sample[2] > 0) + 2 * (sample[1] > 0) + (sample[0] > 0);
            break;

        case APSK32:
            return 16 * (sample[4] > 0) + 8 * (sample[3] > 0) + 4 * (sample[2] > 0) + 2 * (sample[1] > 0) + (sample[0] > 0);
            break;

        default:
            return 0;
            break;
        }
    };

    void constellation_t::soft_demod(int8_t *samples, int size, uint8_t *bits)
    {
        for (int i = 0; i < size / 2; i++)
            bits[i] = soft_demod(&samples[i * 2]);
    }

    void constellation_t::demod_soft_calc(complex_t sample, int8_t *bits, float *phase_error, float npwr)
    {
        int v;
        float tmp[16] = {0.0f};

        if (const_amp != 1)
            sample = sample * const_amp;
        if (const_prescale != 1)
            sample = sample * const_prescale;

        float min_dist = std::numeric_limits<float>::max();
        complex_t closest = 0;
        // int c_i = 0;

        for (int i = 0; i < const_states; i++)
        {
            // Calculate the distance between the sample and the current
            // constellation point.
            float dist = std::abs(std::complex<float>(sample - constellation[i]));

            if (dist < min_dist)
            {
                min_dist = dist;
                closest = constellation[i];
                // c_i = i;
            }

            // Calculate the probability factor from the distance and
            // the scaled noise power.
            float d = expf(-dist / npwr);

            v = i;

            for (int j = 0; j < const_bits; j++)
            {
                // Get the bit at the jth index
                int mask = 1 << j;
                int bit = (v & mask) >> j;

                // If the bit is a 0, add to the probability of a zero
                if (bit == 0)
                    tmp[2 * j + 0] += d;
                // else, add to the probability of a one
                else
                    tmp[2 * j + 1] += d;
            }
        }

        // logger->info(c_i);

        // Calculate the log-likelihood ratio for all bits based on the
        // probability of ones (tmp[2*i+1]) over the probability of a zero
        // (tmp[2*i+0]).
        if (bits != nullptr)
            for (int i = 0; i < const_bits; i++)
                bits[const_bits - 1 - i] = clamp((logf(tmp[2 * i + 1]) - logf(tmp[2 * i + 0])) * const_sca);

        // Calculate phase error
        if (phase_error != nullptr)
            *phase_error = (sample * closest.conj()).arg();
    }

    int8_t constellation_t::clamp(float x)
    {
        while (x < -127 || x > 127)
        {
            x *= 0.5;
            if (!std::isfinite(x))
                return x;
        }
        return x;
    }

    void constellation_t::make_lut(int resolution)
    {
        lut_resolution = resolution;
        lut.resize(resolution * resolution);

        for (int x = 0; x < resolution; x++)
        {
            for (int y = 0; y < resolution; y++)
            {
                float x_v = (float(x - resolution / 2) / float(resolution)) * 1.5f;
                float y_v = (float(y - resolution / 2) / float(resolution)) * 1.5f;

                SoftResult result;
                demod_soft_calc(complex_t(x_v, y_v), result.bits, &result.phase_error);

                lut[x * resolution + y] = result;
            }
        }
    }

    void constellation_t::demod_soft_lut(complex_t sample, int8_t *bits, float *phase_error)
    {
        if (const_bits != 5)
        {
            int x = (sample.real / 1.5) * lut_resolution + lut_resolution / 2;
#if 1
            if (x < 0)
                x = 0;
            if (x >= lut_resolution)
                x = lut_resolution - 1;
#endif

            int y = (sample.imag / 1.5) * lut_resolution + lut_resolution / 2;
#if 1
            if (y < 0)
                y = 0;
            if (y >= lut_resolution)
                y = lut_resolution - 1;
#endif

            SoftResult &v = lut[x * lut_resolution + y];

            if (bits != nullptr)
                for (int i = 0; i < const_bits; i++)
                    bits[i] = v.bits[i];

            if (phase_error != nullptr)
                *phase_error = v.phase_error;
        }
        else
        {
            demod_soft_calc(sample, bits, phase_error);
        }
    }
}