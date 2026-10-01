#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "dsp/ChannelBlockade.h"
#include "dsp/SpectrumTap.h"

namespace IDs
{
    static const juce::String enabled { "enabled" };
}

class SnowsBlockadeAudioProcessor final : public juce::AudioProcessor
{
public:
    SnowsBlockadeAudioProcessor();

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override;

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    // Mono original and processed signals for the editor's spectrum display.
    blockade::SpectrumTap spectrumTap;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    std::array<blockade::ChannelBlockade, blockade::maxChannels> channels;
    juce::SmoothedValue<float> perturbationGain;

    std::atomic<float>* pEnabled = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SnowsBlockadeAudioProcessor)
};
