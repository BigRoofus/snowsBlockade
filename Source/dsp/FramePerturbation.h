#pragma once

#include <array>
#include <cmath>
#include <complex>
#include <cstdint>
#include <vector>
#include <juce_dsp/juce_dsp.h>
#include "BlockadeConstants.h"
#include "MaskingModel.h"
#include "MelWeights.h"

namespace blockade
{
    // The Blockade algorithm for one audio frame. From the frame's spectrum it builds an additive
    // perturbation that stays under the psychoacoustic masking threshold: masking-shaped noise,
    // extra energy above the ultrasonic cutoff, an inverted copy of the strongest peaks (aimed at
    // landmark fingerprinters) and mel-band-shaped noise (aimed at MFCC front-ends).
    class FramePerturbation
    {
    public:
        void prepare(double sampleRate, std::uint32_t seed)
        {
            rng = seed != 0 ? seed : 1u;
            masking.prepare(sampleRate);
            melWeight = melWeights(sampleRate);

            window.resize((size_t) frameSize);
            for (int i = 0; i < frameSize; ++i)
                window[(size_t) i] = 0.5f - 0.5f * std::cos(juce::MathConstants<float>::twoPi * (float) i / (float) (frameSize - 1));

            const double binHz = sampleRate / frameSize;
            noiseGain.resize((size_t) numBins);
            for (int bin = 0; bin < numBins; ++bin)
                noiseGain[(size_t) bin] = std::sqrt(maskingStrength) * (bin * binHz >= ultrasonicCutoffHz ? ultrasonicGain : 1.0f);

            timeDomain.assign((size_t) frameSize, {});
            spectrum.assign((size_t) frameSize, {});
            perturbationSpectrum.assign((size_t) frameSize, {});
            power.assign((size_t) numBins, 0.0f);
            threshold.assign((size_t) numBins, 0.0f);
        }

        const std::vector<float>& hannWindow() const { return window; }

        // `frame` holds frameSize samples. Writes frameSize samples of Hann-windowed perturbation
        // (the perturbation alone, not the signal) to `perturbation`.
        void process(const float* frame, float* perturbation) noexcept
        {
            for (int i = 0; i < frameSize; ++i)
                timeDomain[(size_t) i] = frame[i] * window[(size_t) i];
            fft.perform(timeDomain.data(), spectrum.data(), false);

            for (int bin = 0; bin < numBins; ++bin)
                power[(size_t) bin] = std::norm(spectrum[(size_t) bin]);
            masking.computeThreshold(power.data(), threshold.data());

            for (int bin = 0; bin < numBins; ++bin)
            {
                const auto i = (size_t) bin;
                perturbationSpectrum[i] = threshold[i] * (noiseGain[i] * randomPhasor()
                                                          + melWeight[i] * poisonStrength * randomPhasor());
            }

            addPhaseInvertedPeaks();
            mirrorNegativeFrequencies();

            fft.perform(perturbationSpectrum.data(), timeDomain.data(), true);

            float peak = 0.0f;
            for (int i = 0; i < frameSize; ++i)
            {
                perturbation[i] = timeDomain[(size_t) i].real() * window[(size_t) i];
                peak = std::max(peak, std::abs(perturbation[i]));
            }

            // Hard backstop for a mistuned model: never let a frame's perturbation exceed the ceiling.
            if (peak > perturbationCeiling)
            {
                const float scale = perturbationCeiling / peak;
                for (int i = 0; i < frameSize; ++i)
                    perturbation[i] *= scale;
            }
        }

    private:
        std::complex<float> randomPhasor() noexcept
        {
            rng ^= rng << 13;
            rng ^= rng >> 17;
            rng ^= rng << 5;
            const float phase = juce::MathConstants<float>::twoPi * (float) (rng >> 8) / 16777216.0f;
            return { std::cos(phase), std::sin(phase) };
        }

        // Landmark fingerprinters key on the strongest time-frequency peaks. Each of the frame's
        // top peaks is added back inverted and shifted a few bins, blurring where it sits.
        void addPhaseInvertedPeaks() noexcept
        {
            std::array<int, phaseInvertTopK> topBins {};
            std::array<float, phaseInvertTopK> topPower {};
            topPower.fill(-1.0f);

            int weakest = 0;
            for (int bin = 0; bin < numBins; ++bin)
            {
                const float p = power[(size_t) bin];
                if (p <= topPower[(size_t) weakest])
                    continue;

                topBins[(size_t) weakest] = bin;
                topPower[(size_t) weakest] = p;
                weakest = (int) (std::min_element(topPower.begin(), topPower.end()) - topPower.begin());
            }

            for (int k = 0; k < phaseInvertTopK; ++k)
            {
                if (topPower[(size_t) k] <= 0.0f)
                    continue;

                const int source = topBins[(size_t) k];
                const auto target = (size_t) std::min(source + phaseInvertShiftBins, numBins - 1);
                const float magnitude = std::sqrt(topPower[(size_t) k]);
                const float allowed = std::min(magnitude, threshold[target] * phaseInvertGain);
                perturbationSpectrum[target] -= spectrum[(size_t) source] * (allowed / magnitude);
            }
        }

        // Completes a half spectrum into the conjugate-symmetric full spectrum of a real signal.
        void mirrorNegativeFrequencies() noexcept
        {
            perturbationSpectrum[0] = perturbationSpectrum[0].real();
            perturbationSpectrum[(size_t) frameSize / 2] = perturbationSpectrum[(size_t) frameSize / 2].real();
            for (int bin = 1; bin < frameSize / 2; ++bin)
                perturbationSpectrum[(size_t) (frameSize - bin)] = std::conj(perturbationSpectrum[(size_t) bin]);
        }

        juce::dsp::FFT fft { fftOrder };
        MaskingModel masking;
        std::uint32_t rng = 1;

        std::vector<float> window, noiseGain, melWeight, power, threshold;
        std::vector<std::complex<float>> timeDomain, spectrum, perturbationSpectrum;
    };
}
