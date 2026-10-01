#include "PluginEditor.h"

namespace
{
    constexpr snowsgui::DesignSize design { 600, 420 };
}

SnowsBlockadeAudioProcessorEditor::SnowsBlockadeAudioProcessorEditor(SnowsBlockadeAudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p), blockadeSwitch("Blockade")
{
    logo = snowsgui::createLogo();

    addAndMakeVisible(content);
    content.addAndMakeVisible(spectrum);
    content.addAndMakeVisible(blockadeSwitch);

    blockadeSwitch.setTooltip("Blockade: on adds inaudible high-frequency information that disrupts audio identification "
                              "systems, off passes the audio through untouched. The plugin delays audio by a fixed "
                              "amount; hosts with delay compensation line it back up.");

    blockadeAttachment = std::make_unique<ButtonAttachment>(processorRef.apvts, IDs::enabled, blockadeSwitch);

    snowsgui::makeResizable(*this, design);

    startTimerHz(30);
}

void SnowsBlockadeAudioProcessorEditor::timerCallback()
{
    const double sampleRate = processorRef.getSampleRate();
    spectrum.update([this](float* original, float* processed, size_t count) { processorRef.spectrumTap.latest(original, processed, count); },
                    sampleRate > 0.0 ? sampleRate : 44100.0);
}

void SnowsBlockadeAudioProcessorEditor::paint(juce::Graphics& g)
{
    snowsgui::paintBackdrop(g, *this, design, "SnowsBlockade", "It Snows In Australia", logo.get());
}

void SnowsBlockadeAudioProcessorEditor::resized()
{
    snowsgui::fitContent(*this, content, design);

    spectrum.setBounds((design.width - 360) / 2, 84, 360, 162);
    blockadeSwitch.setBounds((design.width - 216) / 2, 250, 216, 156);
}
