#include "PluginProcessor.h"
#include "PluginEditor.h"

SnowsBlockadeAudioProcessor::SnowsBlockadeAudioProcessor()
    : AudioProcessor(BusesProperties()
                        .withInput("Input", juce::AudioChannelSet::stereo(), true)
                        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "PARAMETERS", createParameterLayout())
{
    pEnabled = apvts.getRawParameterValue(IDs::enabled);
    setLatencySamples(blockade::frameSize);
}

juce::AudioProcessorValueTreeState::ParameterLayout SnowsBlockadeAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterBool>(IDs::enabled, "Blockade", true));

    return { params.begin(), params.end() };
}

double SnowsBlockadeAudioProcessor::getTailLengthSeconds() const
{
    const double sampleRate = getSampleRate();
    return sampleRate > 0.0 ? blockade::frameSize / sampleRate : 0.0;
}

void SnowsBlockadeAudioProcessor::prepareToPlay(double sampleRate, int)
{
    // Each channel gets its own seed so their noise is uncorrelated.
    for (size_t ch = 0; ch < channels.size(); ++ch)
        channels[ch].prepare(sampleRate, 0x9e3779b9u * (std::uint32_t) (ch + 1));

    perturbationGain.reset(sampleRate, blockade::switchRampSeconds);
    perturbationGain.setCurrentAndTargetValue(pEnabled->load() > 0.5f ? 1.0f : 0.0f);
}

void SnowsBlockadeAudioProcessor::releaseResources() {}

bool SnowsBlockadeAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto set = layouts.getMainOutputChannelSet();
    if (set != juce::AudioChannelSet::mono() && set != juce::AudioChannelSet::stereo())
        return false;
    return layouts.getMainInputChannelSet() == set;
}

void SnowsBlockadeAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numChannels = juce::jmin(buffer.getNumChannels(), blockade::maxChannels);
    const int numSamples = buffer.getNumSamples();
    const bool enabled = pEnabled->load() > 0.5f;
    const float monoScale = 1.0f / (float) juce::jmax(1, numChannels);

    perturbationGain.setTargetValue(enabled ? 1.0f : 0.0f);

    std::array<float*, blockade::maxChannels> data {};
    for (int ch = 0; ch < numChannels; ++ch)
        data[(size_t) ch] = buffer.getWritePointer(ch);

    for (int i = 0; i < numSamples; ++i)
    {
        const float gain = perturbationGain.getNextValue();
        const bool active = enabled || gain > 0.0f;

        float dryMono = 0.0f, outMono = 0.0f;
        for (int ch = 0; ch < numChannels; ++ch)
        {
            const auto result = channels[(size_t) ch].process(data[(size_t) ch][i], active);
            const float out = result.dry + result.perturbation * gain;
            data[(size_t) ch][i] = out;
            dryMono += result.dry;
            outMono += out;
        }

        spectrumTap.push(dryMono * monoScale, outMono * monoScale);
    }
}

juce::AudioProcessorEditor* SnowsBlockadeAudioProcessor::createEditor()
{
    return new SnowsBlockadeAudioProcessorEditor(*this);
}

void SnowsBlockadeAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary(*xml, destData);
}

void SnowsBlockadeAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
        if (xml->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SnowsBlockadeAudioProcessor();
}
