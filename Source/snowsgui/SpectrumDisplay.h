#pragma once

#include <juce_dsp/juce_dsp.h>
#include "Theme.h"

namespace snowsgui
{
    // Real-time spectrum on a log frequency axis. The input spectrum is a gray area;
    // wherever the output rises above it, the difference is drawn in white on top. With
    // `outputAsCurve` the output is instead a white curve over the gray area, so it
    // shows where the output falls below the input too.
    class SpectrumDisplay final : public juce::Component
    {
    public:
        static constexpr int fftOrder = 12, fftSize = 1 << fftOrder, numPoints = 180;

        bool outputAsCurve = false;

        // Analyses the newest samples. `fetch(float* input, float* output, size_t count)` must
        // fill both buffers with the latest `count` samples, oldest first. Both signals share
        // one window, so identical signals give identical spectra and nothing white is drawn.
        template <typename Fetch>
        void update(Fetch&& fetch, double sampleRate)
        {
            fetch(inputSamples.data(), outputSamples.data(), (size_t) fftSize);
            analyse(inputSamples.data(), sampleRate, inputDb);
            analyse(outputSamples.data(), sampleRate, outputDb);
            repaint();
        }

        void paint(juce::Graphics& g) override
        {
            const auto r = getLocalBounds().toFloat();
            g.setColour(juce::Colour(0xff161617));
            g.fillRoundedRectangle(r, 10.0f);
            innerShadow(g, r, 10.0f, 0.7f, 10.0f);

            juce::Path clip;
            clip.addRoundedRectangle(r, 10.0f);
            g.reduceClipRegion(clip);

            const auto plot = r.reduced(6.0f);
            const auto xAt = [&](int p) { return plot.getX() + plot.getWidth() * (float) p / (numPoints - 1); };
            const auto yAt = [&](float db)
            {
                return plot.getBottom() - juce::jlimit(0.0f, 1.0f, (db - floorDb) / (ceilDb - floorDb)) * plot.getHeight();
            };

            juce::Path input;
            input.startNewSubPath(plot.getX(), plot.getBottom());
            for (int p = 0; p < numPoints; ++p)
                input.lineTo(xAt(p), yAt(inputDb[(size_t) p]));
            input.lineTo(plot.getRight(), plot.getBottom());
            input.closeSubPath();
            g.setColour(juce::Colour(0xff454546));
            g.fillPath(input);

            if (outputAsCurve)
            {
                juce::Path curve;
                for (int p = 0; p < numPoints; ++p)
                {
                    if (p == 0)
                        curve.startNewSubPath(xAt(p), yAt(outputDb[(size_t) p]));
                    else
                        curve.lineTo(xAt(p), yAt(outputDb[(size_t) p]));
                }
                g.setColour(juce::Colours::white);
                g.strokePath(curve, juce::PathStrokeType(1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
                return;
            }

            juce::Path added;
            for (int p = 0; p < numPoints; ++p)
            {
                const float y = yAt(juce::jmax(inputDb[(size_t) p], outputDb[(size_t) p]));
                if (p == 0)
                    added.startNewSubPath(xAt(p), y);
                else
                    added.lineTo(xAt(p), y);
            }
            for (int p = numPoints - 1; p >= 0; --p)
                added.lineTo(xAt(p), yAt(inputDb[(size_t) p]));
            added.closeSubPath();
            g.setColour(juce::Colours::white);
            g.fillPath(added);
        }

    private:
        static constexpr float minHz = 20.0f, maxHz = 20000.0f;
        static constexpr float floorDb = -90.0f, ceilDb = 0.0f;
        static constexpr float tiltDbPerOctave = 4.5f;

        using Curve = std::array<float, numPoints>;

        // Turns one window of samples into a smoothed dB curve, one value per display point.
        void analyse(const float* samples, double sampleRate, Curve& curve)
        {
            std::copy(samples, samples + fftSize, fftData.begin());
            std::fill(fftData.begin() + fftSize, fftData.end(), 0.0f);
            window.multiplyWithWindowingTable(fftData.data(), fftSize);
            fft.performFrequencyOnlyForwardTransform(fftData.data());

            const float binsPerHz = (float) fftSize / (float) sampleRate;
            const float lastBin = (float) (fftSize / 2 - 1);
            const float ampScale = 4.0f / (float) fftSize; // a full-scale sine reads 0 dB
            const float halfStep = std::pow(maxHz / minHz, 0.5f / (numPoints - 1));

            for (int p = 0; p < numPoints; ++p)
            {
                const float hz = minHz * std::pow(maxHz / minHz, (float) p / (numPoints - 1));
                const float lo = hz / halfStep * binsPerHz, hi = hz * halfStep * binsPerHz;

                float power = 0.0f;
                if (lo < lastBin)
                {
                    if (hi - lo < 1.0f)
                    {
                        const float centre = 0.5f * (lo + hi);
                        const int b = (int) centre;
                        const float frac = centre - (float) b;
                        const float mag = fftData[(size_t) b] * (1.0f - frac) + fftData[(size_t) b + 1] * frac;
                        power = mag * mag;
                    }
                    else
                    {
                        const int first = (int) std::ceil(lo), last = juce::jmin((int) hi, (int) lastBin);
                        for (int b = first; b <= last; ++b)
                            power += fftData[(size_t) b] * fftData[(size_t) b];
                        power /= (float) juce::jmax(1, last - first + 1);
                    }
                }

                const float tilt = tiltDbPerOctave * std::log2(hz / 1000.0f);
                const float db = 10.0f * std::log10(power * ampScale * ampScale + 1.0e-12f) + tilt;
                auto& shown = curve[(size_t) p];
                shown += (db - shown) * (db > shown ? 0.6f : 0.15f);
            }
        }

        juce::dsp::FFT fft { fftOrder };
        juce::dsp::WindowingFunction<float> window { (size_t) fftSize, juce::dsp::WindowingFunction<float>::hann };

        std::array<float, fftSize> inputSamples {}, outputSamples {};
        std::array<float, 2 * fftSize> fftData {};
        Curve inputDb = filled(), outputDb = filled();

        static Curve filled()
        {
            Curve c;
            c.fill(floorDb);
            return c;
        }
    };
}
