#pragma once

#include <algorithm>
#include <cmath>
#include <vector>
#include "BlockadeConstants.h"

namespace blockade
{
    // Per-FFT-bin weight in [0, 1]: how strongly the most interested filter of a standard
    // triangular mel filterbank (spanning 0 Hz to Nyquist) responds to that bin.
    inline std::vector<float> melWeights(double sampleRate)
    {
        const auto hzToMel = [](double hz) { return 2595.0 * std::log10(1.0 + hz / 700.0); };
        const auto melToHz = [](double mel) { return 700.0 * (std::pow(10.0, mel / 2595.0) - 1.0); };

        const double maxMel = hzToMel(sampleRate / 2.0);
        std::vector<double> edgesHz((size_t) poisonMelBands + 2);
        for (size_t i = 0; i < edgesHz.size(); ++i)
            edgesHz[i] = melToHz(maxMel * (double) i / (double) (poisonMelBands + 1));

        constexpr double minWidthHz = 1.0e-8;
        const double binHz = sampleRate / frameSize;

        std::vector<float> weights((size_t) numBins, 0.0f);
        for (size_t band = 0; band < (size_t) poisonMelBands; ++band)
        {
            const double left = edgesHz[band], centre = edgesHz[band + 1], right = edgesHz[band + 2];
            for (int bin = 0; bin < numBins; ++bin)
            {
                const double hz = bin * binHz;
                const double rising = (hz - left) / std::max(centre - left, minWidthHz);
                const double falling = (right - hz) / std::max(right - centre, minWidthHz);
                weights[(size_t) bin] = std::max(weights[(size_t) bin], (float) std::min(rising, falling));
            }
        }

        const float peak = *std::max_element(weights.begin(), weights.end());
        for (auto& w : weights)
            w /= peak + 1.0e-12f;
        return weights;
    }
}
