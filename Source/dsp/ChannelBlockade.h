#pragma once

#include <algorithm>
#include <vector>
#include "FramePerturbation.h"

namespace blockade
{
    // Streams one channel through the Blockade algorithm: frames of the input are analysed every
    // hopSize samples and their perturbations are overlap-added. The perturbation for a sample is
    // only complete once every frame covering it has arrived, so the whole channel runs frameSize
    // samples late and the untouched signal is delayed to match.
    class ChannelBlockade
    {
    public:
        struct Output
        {
            float dry;       // the input, delayed by frameSize samples
            float perturbation;
        };

        void prepare(double sampleRate, std::uint32_t seed)
        {
            frame.prepare(sampleRate, seed);

            // Frames overlap, so each sample's perturbation is a sum of two windowed frames.
            // Dividing by the summed squared window (applied on analysis and synthesis) evens it out.
            constexpr float minWindowEnergy = 1.0e-8f;
            const auto& window = frame.hannWindow();
            overlapGain.resize((size_t) hopSize);
            for (int i = 0; i < hopSize; ++i)
            {
                const float a = window[(size_t) i], b = window[(size_t) (i + hopSize)];
                const float energy = a * a + b * b;
                overlapGain[(size_t) i] = energy > minWindowEnergy ? 1.0f / energy : 0.0f;
            }

            history.assign((size_t) frameSize, 0.0f);
            pending.assign((size_t) frameSize, 0.0f);
            frameIn.assign((size_t) frameSize, 0.0f);
            frameOut.assign((size_t) frameSize, 0.0f);
            position = 0;
        }

        // With `active` false no frames are analysed, so nothing is added and no CPU is spent.
        Output process(float input, bool active) noexcept
        {
            const auto slot = (size_t) position;

            // The slot still holds the input from frameSize samples ago, and the perturbation
            // that is now complete for that same sample. Two overlapped frames can each sit at the
            // ceiling and the overlap gain can exceed 1, so the sum is held to the ceiling too.
            const float perturbation = std::clamp(pending[slot] * overlapGain[slot % (size_t) hopSize],
                                                  -perturbationCeiling, perturbationCeiling);
            const Output output { history[slot], perturbation };
            pending[slot] = 0.0f;
            history[slot] = input;

            position = (position + 1) % frameSize;

            if (active && position % hopSize == 0)
                analyseLatestFrame();

            return output;
        }

    private:
        void analyseLatestFrame() noexcept
        {
            // `position` is one past the newest sample, so it is also the oldest one in the ring.
            for (int i = 0; i < frameSize; ++i)
                frameIn[(size_t) i] = history[(size_t) ((position + i) % frameSize)];

            frame.process(frameIn.data(), frameOut.data());

            for (int i = 0; i < frameSize; ++i)
                pending[(size_t) ((position + i) % frameSize)] += frameOut[(size_t) i];
        }

        FramePerturbation frame;
        std::vector<float> overlapGain, history, pending, frameIn, frameOut;
        int position = 0;
    };
}
