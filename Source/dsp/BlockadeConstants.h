#pragma once

namespace blockade
{
    // 4096/2048 gives ~10.7 Hz bins at 44.1 kHz (needed by the Bark-scale masking model)
    // with 50% overlap, which is what the Hann overlap-add reconstruction relies on.
    constexpr int fftOrder = 12;
    constexpr int frameSize = 1 << fftOrder;
    constexpr int hopSize = frameSize / 2;
    constexpr int numBins = frameSize / 2 + 1;

    constexpr int maxChannels = 2;

    // Fraction of the masking threshold that is filled with perturbation energy. Kept below 1
    // as a safety margin: the masking model is a simplified approximation.
    constexpr float maskingStrength = 0.7f;

    // Above this frequency the perturbation uses a fixed gain rather than a strict masking
    // derivation, since absolute-threshold modelling is unreliable that close to Nyquist.
    constexpr float ultrasonicCutoffHz = 18000.0f;
    constexpr float ultrasonicGain = 0.5f;

    // Hard cap on the peak of one frame's perturbation (fraction of full scale), applied
    // whatever the masking model computed.
    constexpr float perturbationCeiling = 0.02f;

    // Phase-inversion: an inverted, bin-shifted copy of each frame's strongest spectral peaks.
    constexpr int phaseInvertTopK = 8;
    constexpr int phaseInvertShiftBins = 3;
    constexpr float phaseInvertGain = 0.5f;

    // Data poisoning: noise shaped toward the bands of a triangular mel filterbank.
    constexpr int poisonMelBands = 40;
    constexpr float poisonStrength = 1.0f;

    // Time the perturbation takes to fade in and out when Blockade is switched.
    constexpr double switchRampSeconds = 0.01;
}
