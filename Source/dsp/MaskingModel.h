#pragma once

#include <algorithm>
#include <cmath>
#include <vector>
#include "BlockadeConstants.h"

namespace blockade
{
    // Simplified psychoacoustic masking model (Bark scale, Schroeder spreading function) of the
    // kind MP3 encoders use to decide how much noise a signal can hide. Not the full ISO/MPEG
    // model. Maskers are grouped into half-Bark bands so a frame costs a few thousand operations
    // rather than a full bin-by-bin spreading matrix.
    class MaskingModel
    {
    public:
        void prepare(double sampleRate)
        {
            const double binHz = sampleRate / frameSize;

            std::vector<double> barks((size_t) numBins);
            for (int bin = 0; bin < numBins; ++bin)
                barks[(size_t) bin] = hzToBark(bin * binHz);

            numBands = (int) (barks.back() / bandWidthBark) + 1;

            bandOfBin.assign((size_t) numBins, 0);
            binScale.assign((size_t) numBins, 0.0f);
            athAmplitude.assign((size_t) numBins, 0.0f);

            for (int bin = 0; bin < numBins; ++bin)
            {
                const auto i = (size_t) bin;
                bandOfBin[i] = std::min((int) (barks[i] / bandWidthBark), numBands - 1);

                // The ear sums noise-like energy over a critical band, but every bin of the band is
                // filled at once, so each bin gets its share of the band's power budget.
                const double barkPerBin = (barks[(size_t) std::min(bin + 1, numBins - 1)] - barks[(size_t) std::max(bin - 1, 0)])
                                          / (double) (std::min(bin + 1, numBins - 1) - std::max(bin - 1, 0));
                const double binsPerBand = std::max(1.0 / std::max(barkPerBin, epsilon), 1.0);
                binScale[i] = (float) (std::sqrt(1.0 / binsPerBand) * frameSize / 2.0);

                athAmplitude[i] = (float) dbToAmplitude(absoluteThresholdDb(bin * binHz));
            }

            spreadDb.assign((size_t) (numBands * numBands), 0.0f);
            for (int target = 0; target < numBands; ++target)
                for (int masker = 0; masker < numBands; ++masker)
                    spreadDb[(size_t) (target * numBands + masker)] = (float) spreadingDb((target - masker) * bandWidthBark);

            bandPeakPower.assign((size_t) numBands, 0.0f);
            bandMaskerDb.assign((size_t) numBands, 0.0f);
            bandAllowedAmplitude.assign((size_t) numBands, 0.0f);
        }

        // `power` holds |X[k]|^2 of the Hann-windowed frame, one value per bin. Fills `threshold`
        // with the largest noise magnitude, in the same raw FFT units, each bin can carry unheard.
        void computeThreshold(const float* power, float* threshold) noexcept
        {
            std::fill(bandPeakPower.begin(), bandPeakPower.end(), 0.0f);
            for (int bin = 0; bin < numBins; ++bin)
            {
                auto& peak = bandPeakPower[(size_t) bandOfBin[(size_t) bin]];
                peak = std::max(peak, power[bin]);
            }

            // An FFT of a sinusoid of amplitude A peaks at A * frameSize / 2; undo that so the
            // level reflects the real signal amplitude, not the FFT size.
            constexpr float amplitudeNorm = 2.0f / (float) frameSize;
            for (int band = 0; band < numBands; ++band)
            {
                const float magnitude = std::sqrt(bandPeakPower[(size_t) band]) * amplitudeNorm;
                bandMaskerDb[(size_t) band] = 20.0f * std::log10(magnitude + (float) epsilon)
                                              + (float) calibrationDbSpl - (float) maskingOffsetDb;
            }

            for (int target = 0; target < numBands; ++target)
            {
                const float* spread = &spreadDb[(size_t) (target * numBands)];
                float maskedDb = bandMaskerDb[0] + spread[0];
                for (int masker = 1; masker < numBands; ++masker)
                    maskedDb = std::max(maskedDb, bandMaskerDb[(size_t) masker] + spread[masker]);
                bandAllowedAmplitude[(size_t) target] = (float) dbToAmplitude((double) maskedDb);
            }

            for (int bin = 0; bin < numBins; ++bin)
            {
                const auto i = (size_t) bin;
                threshold[bin] = std::max(bandAllowedAmplitude[(size_t) bandOfBin[i]], athAmplitude[i]) * binScale[i];
            }
        }

    private:
        // Full-scale digital amplitude (1.0) is taken to be 96 dB SPL, so FFT levels can be
        // compared against the SPL-calibrated masking formulas.
        static constexpr double calibrationDbSpl = 96.0;

        // A masker doesn't raise the threshold to its own level: noise becomes inaudible some
        // margin below it. Real models derive this from tonality; this is a conservative constant.
        static constexpr double maskingOffsetDb = 20.0;

        static constexpr double bandWidthBark = 0.5;
        static constexpr double epsilon = 1.0e-12;

        static double hzToBark(double hz)
        {
            return 13.0 * std::atan(0.00076 * hz) + 3.5 * std::atan(std::pow(hz / 7500.0, 2.0));
        }

        // Terhardt's absolute threshold of hearing in dB SPL. The formula is only valid up to
        // ~20 kHz, so frequencies beyond reuse the 20 kHz value (near-total inaudibility).
        static double absoluteThresholdDb(double hz)
        {
            const double khz = std::clamp(hz, 20.0, 20000.0) / 1000.0;
            return 3.64 * std::pow(khz, -0.8) - 6.5 * std::exp(-0.6 * (khz - 3.3) * (khz - 3.3)) + 1.0e-3 * std::pow(khz, 4.0);
        }

        // How much a masker attenuates the masking threshold `barkDistance` Bark away.
        static double spreadingDb(double barkDistance)
        {
            const double d = barkDistance + 0.474;
            return 15.81 + 7.5 * d - 17.5 * std::sqrt(1.0 + d * d);
        }

        // Digital amplitude for a level in dB SPL, capped at full scale.
        static double dbToAmplitude(double db)
        {
            return std::clamp(std::pow(10.0, (db - calibrationDbSpl) / 20.0), 0.0, 1.0);
        }

        int numBands = 0;
        std::vector<int> bandOfBin;
        std::vector<float> binScale, athAmplitude, spreadDb;
        std::vector<float> bandPeakPower, bandMaskerDb, bandAllowedAmplitude;
    };
}
