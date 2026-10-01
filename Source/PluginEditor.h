#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"
#include "snowsgui/snowsgui.h"

class SnowsBlockadeAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                                private juce::Timer
{
public:
    explicit SnowsBlockadeAudioProcessorEditor(SnowsBlockadeAudioProcessor&);

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    void timerCallback() override;

    SnowsBlockadeAudioProcessor& processorRef;

    juce::TooltipWindow tooltipWindow { this, 500 };

    std::unique_ptr<juce::Drawable> logo;

    // Holds all controls at the fixed design size; resized() scales it to the window.
    juce::Component content;

    snowsgui::SpectrumDisplay spectrum;
    snowsgui::PillSwitch blockadeSwitch;

    std::unique_ptr<ButtonAttachment> blockadeAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SnowsBlockadeAudioProcessorEditor)
};
